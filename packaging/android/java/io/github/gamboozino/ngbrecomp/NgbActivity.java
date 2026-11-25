package io.github.gamboozino.ngbrecomp;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.provider.DocumentsContract;
import android.system.Os;

import java.io.File;
import java.nio.charset.StandardCharsets;

import org.libsdl.app.SDLActivity;

/* SDL's activity, loading the game (libmain.so, src/android_main.c) after SDL itself -- or the arcade's program
 * the game launched (libtrilogy.so, libtomoe.so: src/launch.c); and what the first start needs to install the game
 * from the player's disc image (src/android_install.c calls these): the system's file picker, which gives the app
 * the one file chosen (no storage permission), and the SD card. */
public class NgbActivity extends SDLActivity {
    private static final int PICK_DISC = 0x4E47;
    static final String EXTRA_LIB = "io.github.gamboozino.ngbrecomp.LIB";
    static final String EXTRA_ENV = "io.github.gamboozino.ngbrecomp.ENV";
    private volatile String mPicked;
    private volatile boolean mPickDone;

    private String mLib = "main";

    /* Started by relaunch: the library to run, and the variables the program that launched it left ("NAME=value",
     * each ended by a 0 byte, UTF-8), set before SDL loads the library and starts it. Only at that start: the activity
     * made again later (the app ended in the background and brought back) gets its first intent again, and would
     * otherwise boot as a return from the arcade or into the arcade; so not with a saved state, and taken out of the
     * intent once used. */
    @Override
    protected void onCreate(Bundle state) {
        Intent intent = getIntent();
        String lib = state == null && intent != null ? intent.getStringExtra(EXTRA_LIB) : null;
        byte[] env = state == null && intent != null ? intent.getByteArrayExtra(EXTRA_ENV) : null;
        if (lib != null && lib.matches("main|trilogy|tomoe"))
            mLib = lib;
        for (int i = 0, start = 0; env != null && i < env.length; i++) {
            if (env[i] != 0)
                continue;
            String kv = new String(env, start, i - start, StandardCharsets.UTF_8);
            start = i + 1;
            int eq = kv.indexOf('=');
            if (eq > 0) {
                try {
                    Os.setenv(kv.substring(0, eq), kv.substring(eq + 1), true);
                } catch (Exception e) {
                }
            }
        }
        if (intent != null && (intent.hasExtra(EXTRA_LIB) || intent.hasExtra(EXTRA_ENV))) {
            Intent plain = new Intent(intent);
            plain.removeExtra(EXTRA_LIB);
            plain.removeExtra(EXTRA_ENV);
            setIntent(plain);
        }
        super.onCreate(state);
    }

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", mLib };
    }

    /* Another of the app's programs in place of this one (src/launch.c, as an exec on the desktop): the process
     * ended and the activity started again in a new one, running lib with env. A small activity in a process of
     * its own does it (NgbRelaunch): this one cannot start itself again from the process being ended. */
    public void relaunch(String lib, byte[] env) {
        Intent next = new Intent(this, NgbActivity.class);
        next.putExtra(EXTRA_LIB, lib);
        next.putExtra(EXTRA_ENV, env);
        next.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        Intent i = new Intent(this, NgbRelaunch.class);
        i.putExtra(NgbRelaunch.EXTRA_NEXT, next);
        i.putExtra(NgbRelaunch.EXTRA_PID, Process.myPid());
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(i);
    }

    /* The system's file picker, for the disc image: pickDone() turns true when it closes, pickedPath() is then
     * the file's path, or null */
    public void pickDisc() {
        mPicked = null;
        mPickDone = false;
        runOnUiThread(() -> {
            Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType("*/*");
            try {
                startActivityForResult(i, PICK_DISC);
            } catch (Exception e) {
                mPickDone = true;
            }
        });
    }

    public boolean pickDone() {
        return mPickDone;
    }

    public String pickedPath() {
        return mPicked;
    }

    @Override
    protected void onActivityResult(int request, int result, Intent data) {
        if (request == PICK_DISC) {   /* (off the UI thread: a cloud file is fetched whole when opened) */
            final Uri uri = result == RESULT_OK && data != null ? data.getData() : null;
            new Thread(() -> {
                mPicked = uri != null ? pathOf(uri) : null;
                mPickDone = true;
            }).start();
            return;
        }
        super.onActivityResult(request, result, data);
    }

    /* The file by its path where the app can read it there; else (most often, with no storage permission) an open
     * descriptor's, /proc/self/fd/N, which the installer takes for a disc image */
    private String pathOf(Uri uri) {
        try {
            if (DocumentsContract.isDocumentUri(this, uri)) {
                String id = DocumentsContract.getDocumentId(uri);
                if ("com.android.externalstorage.documents".equals(uri.getAuthority()) && id.indexOf(':') > 0) {
                    String volume = id.substring(0, id.indexOf(':')), rel = id.substring(id.indexOf(':') + 1);
                    File f = new File("primary".equalsIgnoreCase(volume)
                                          ? Environment.getExternalStorageDirectory().getPath()
                                          : "/storage/" + volume, rel);
                    if (f.canRead())
                        return f.getPath();
                }
                if (id.startsWith("raw:") && new File(id.substring(4)).canRead())
                    return id.substring(4);
                if (id.startsWith("home:")) {   /* (the Documents folder) */
                    File f = new File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOCUMENTS),
                                      id.substring(5));
                    if (f.canRead())
                        return f.getPath();
                }
            }
            ParcelFileDescriptor p = getContentResolver().openFileDescriptor(uri, "r");
            return p == null ? null : "/proc/self/fd/" + p.detachFd();
        } catch (Exception e) {
            return null;
        }
    }

    /* The app's own folder on a removable SD card, if one is in (no permission needed there), or null */
    public String sdCardFolder() {
        File[] dirs = getExternalFilesDirs(null);
        for (int i = 1; dirs != null && i < dirs.length; i++)
            if (dirs[i] != null && Environment.isExternalStorageRemovable(dirs[i]) &&
                Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(dirs[i])))
                return dirs[i].getPath();
        return null;
    }
}
