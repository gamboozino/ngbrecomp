package io.github.gamboozino.ngbrecomp;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Process;
import android.util.Log;

import java.io.File;

/* NgbActivity.relaunch's other half, in a process of its own (the manifest's ":relaunch"), see-through: the game's
 * process ended, then the activity started again with the next library, and this process ended too. The old process
 * goes first: the system would start the activity in it while it is still there. The start is made once this activity
 * is in front (onResume), the app's own activity on top, which is what Android 10 and later want of a start not
 * made from the background (Android 14 and 15 are stricter about those). */
public class NgbRelaunch extends Activity {
    static final String EXTRA_NEXT = "io.github.gamboozino.ngbrecomp.NEXT";
    static final String EXTRA_PID = "io.github.gamboozino.ngbrecomp.PID";
    private boolean mDone;

    @Override
    protected void onResume() {
        super.onResume();
        if (mDone)
            return;
        mDone = true;
        int pid = getIntent().getIntExtra(EXTRA_PID, -1);
        Intent next = getIntent().getParcelableExtra(EXTRA_NEXT);
        if (pid > 0 && pid != Process.myPid()) {
            Process.killProcess(pid);
            for (int i = 0; i < 200 && new File("/proc/" + pid).exists(); i++) {
                try {
                    Thread.sleep(10);
                } catch (InterruptedException e) {
                    break;
                }
            }
        }
        try {
            if (next != null)
                startActivity(next);
            Log.i("NGB", "[LAUNCH] relaunch: " + (next != null ? next.getStringExtra(NgbActivity.EXTRA_LIB) : "nothing")
                             + " started");
        } catch (Exception e) {
            Log.e("NGB", "[LAUNCH] relaunch: could not start the game again", e);
        }
        finish();
        overridePendingTransition(0, 0);
        Runtime.getRuntime().exit(0);
    }
}
