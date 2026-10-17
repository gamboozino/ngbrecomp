/*
 * Finding the game's files, and installing them from the player's disc.
 *
 * The program carries no game data. It runs from the disc's files: the
 * directory named on the command line or in NGB_GAME_DIR, else the one it
 * installed earlier (<save dir>/game), else "game" beside the executable.
 * Given a disc image instead -- an Xbox ISO (a plain XDVDFS image or a full
 * Redump dump) or the .7z/.zip it came in -- it unpacks the game partition
 * into <save dir>/game once and runs from there.
 *
 * The code was recompiled from one particular default.xbe, so the installed
 * one is checked against it: a different release or a modified disc would
 * run this code against data it was not built for.
 */
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#if defined(_WIN32)
#include <direct.h>
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shlobj.h>
#define mkdir(path, mode) _mkdir(path)
#define fseeko _fseeki64
#define off_t int64_t
#else
#include <signal.h>
#include <sys/wait.h>
#endif

#include "install.h"
#include "port.h"

static void restart_portable(const char *src);

/* The retail USA default.xbe the code was recompiled from. */
#define NGB_XBE_SIZE  4571136u
#define NGB_XBE_FNV   0x3602ba6a6079c8cdull

#define SECTOR 2048u
static const char XDVDFS_MAGIC[20] = "MICROSOFT*XBOX*MEDIA";
/* Where the game partition starts: plain XISO, a few rebuilt-image layouts,
 * and a Redump full-disc dump (after the ~387 MB video partition). */
static const uint64_t BASES[] = { 0, 0xFD90, 0x30600, 0xFD90000, 0x18300000 };

static char s_err[1024];
static ngb_progress_fn s_progress;
static void *s_progress_ud;
static volatile int s_cancel;

/* The reason the last call failed, as the player should read it. */
static void fail(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s_err, sizeof s_err, fmt, ap);
    va_end(ap);
    fprintf(stderr, "%s\n", s_err);
}

const char *ngb_install_error(void) { return s_err; }

void ngb_install_set_progress(ngb_progress_fn fn, void *ud)
{
    s_progress = fn;
    s_progress_ud = ud;
}

#if defined(_WIN32)
static HANDLE volatile s_child;  /* an unpacker running */
#else
static volatile pid_t s_child;
#endif

void ngb_install_cancel(void)
{
    s_cancel = 1;
#if defined(_WIN32)
    if (s_child)
        TerminateProcess(s_child, 1);
#else
    if (s_child > 0)
        kill(s_child, SIGTERM);
#endif
}

static void progress(uint64_t done, uint64_t total)
{
    if (s_progress)
        s_progress(done, total, s_progress_ud);
}

static int is_dir(const char *p)
{
    struct stat st;
    return p && stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

static int is_file(const char *p)
{
    struct stat st;
    return p && stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

static int has_xbe(const char *dir)
{
    char p[4096];
    snprintf(p, sizeof p, "%s/default.xbe", dir);
    return is_file(p);
}

static int mkdirs(const char *path)
{
    char tmp[4096];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char *q = tmp + 1; *q; q++) {
        if (*q != '/' && *q != '\\')
            continue;
        *q = 0;
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST && !is_dir(tmp))
            return -1;
        *q = '/';
    }
    return mkdir(tmp, 0755) == 0 || errno == EEXIST ? 0 : -1;
}

/* 0 when dir's default.xbe is the one this build was made from. */
/* The retail XBE's sections, in order: their sizes and FNV-1a hashes. A disc image made with a
 * "media patch" (so a modded console runs it from a burned DVD) has a changed header and
 * certificate, but the game itself -- these sections -- is the same, and so is this build. */
static const struct { const char *name; uint32_t size; uint64_t fnv; } s_xbe_sections[] = {
    { ".text",     3576960, 0x4f583bc03974eb5dull }, { "D3DX",          552, 0x802e93d06050ddc2ull },
    { "DSOUND",     116108, 0x1c717f4fb13d38d9ull }, { "XONLINE",    100604, 0xf7f32cba9715d707ull },
    { "XNET",        73112, 0x5dd132e95dd1b4ecull }, { "PSFD00",      11192, 0xd033e2d3a400c0d7ull },
    { "XGRPH",        4388, 0xdb9d2b60f04aec39ull }, { "D3D",         68272, 0x54c712be5b41551bull },
    { "XPP",         32308, 0x10b3b979adbff38eull }, { ".rdata",     216700, 0x722ca73485b1ee65ull },
    { ".data",      283752, 0x2990cb7806d3c3d8ull }, { "DOLBY",       29036, 0x08826a2af878f54full },
    { "XON_RD",       5388, 0xa7c8c48ee45f6184ull }, { "$$XTIMAGE",   10240, 0xe4449f60879ad7d0ull },
    { "$$XSIMAGE",    4096, 0x0eb831e2c2d793a4ull }, { ".XTLID",       1808, 0x04d0097419f217b7ull },
};

static uint64_t fnv1a(const unsigned char *p, size_t n, uint64_t h)
{
    for (size_t i = 0; i < n; i++)
        h = (h ^ p[i]) * 0x100000001b3ull;
    return h;
}

static uint32_t le32(const unsigned char *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

int ngb_check_xbe(const char *dir)
{
    char p[4096];
    unsigned char *d;
    long size;
    FILE *f;
    int ok = 1, nsec = (int)(sizeof s_xbe_sections / sizeof s_xbe_sections[0]);

    snprintf(p, sizeof p, "%s/default.xbe", dir);
    f = fopen(p, "rb");
    if (!f)
        return -1;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    d = size > 0 && size < (64l << 20) ? malloc((size_t)size) : NULL;
    if (!d || fread(d, 1, (size_t)size, f) != (size_t)size) {
        fclose(f);
        free(d);
        return 1;
    }
    fclose(f);
    if ((uint64_t)size == NGB_XBE_SIZE && fnv1a(d, (size_t)size, 0xcbf29ce484222325ull) == NGB_XBE_FNV) {
        free(d);
        return 0;                                   /* the very file */
    }
    /* Otherwise the game's sections, wherever the header puts them. */
    if (size < 0x180 || memcmp(d, "XBEH", 4) != 0 || le32(d + 0x11C) != (uint32_t)nsec) {
        fprintf(stderr, "%s: not this game's XBE (%ld bytes)\n", p, size);
        free(d);
        return 1;
    }
    {
        uint32_t base = le32(d + 0x104), table = le32(d + 0x120) - base;
        for (int i = 0; i < nsec; i++) {
            const unsigned char *h = d + table + (size_t)i * 56;
            uint32_t raw, rsz;
            uint64_t got;
            if (table + (size_t)(i + 1) * 56 > (size_t)size) {
                ok = 0;
                break;
            }
            raw = le32(h + 12);
            rsz = le32(h + 16);
            if (rsz != s_xbe_sections[i].size || (uint64_t)raw + rsz > (uint64_t)size) {
                fprintf(stderr, "%s: section %s is %u bytes, not %u\n", p, s_xbe_sections[i].name, rsz,
                        s_xbe_sections[i].size);
                ok = 0;
                continue;
            }
            got = fnv1a(d + raw, rsz, 0xcbf29ce484222325ull);
            if (got != s_xbe_sections[i].fnv) {
                fprintf(stderr, "%s: section %s differs (%016llx)\n", p, s_xbe_sections[i].name,
                        (unsigned long long)got);
                ok = 0;
            }
        }
    }
    free(d);
    if (ok)
        fprintf(stderr, "%s: a modified header (a media patch?), the game's sections as in the retail "
                        "release: accepted\n", p);
    return ok ? 0 : 1;
}

/* ---- XDVDFS ------------------------------------------------------------ */

typedef struct {
    FILE *f;
    uint64_t base;
    uint64_t files, bytes;
    uint64_t total;     /* bytes to extract, from a first walk */
    int count_only;     /* that walk: sum the sizes, write nothing */
} Image;

static int read_at(Image *im, uint64_t sector, void *buf, size_t len)
{
    if (fseeko(im->f, (off_t)(im->base + sector * SECTOR), SEEK_SET) != 0)
        return -1;
    return fread(buf, 1, len, im->f) == len ? 0 : -1;
}

static int copy_out(Image *im, uint32_t sector, uint32_t size, const char *dest)
{
    static unsigned char buf[1 << 20];
    FILE *out = fopen(dest, "wb");
    uint32_t left = size;

    if (!out) {
        fail("Cannot write %s: %s", dest, strerror(errno));
        return -1;
    }
    if (fseeko(im->f, (off_t)(im->base + (uint64_t)sector * SECTOR), SEEK_SET) != 0) {
        fclose(out);
        fail("Cannot read the disc image at sector %u (for %s): %s", sector, dest, strerror(errno));
        return -1;
    }
    while (left) {
        size_t chunk = left < sizeof buf ? left : sizeof buf;
        if (s_cancel) {
            fclose(out);
            fail("Cancelled.");
            return -1;
        }
        if (fread(buf, 1, chunk, im->f) != chunk) {
            fclose(out);
            fail("The disc image is cut short (a damaged or incomplete download?).");
            return -1;
        }
        if (fwrite(buf, 1, chunk, out) != chunk) {
            fclose(out);
            fail("Cannot write %s: %s", dest, strerror(errno));
            return -1;
        }
        left -= (uint32_t)chunk;
        progress(im->bytes + (size - left), im->total);
    }
    im->files++;
    im->bytes += size;
    if (im->files % 25 == 0)
        fprintf(stderr, "  %llu files, %llu MB\n", (unsigned long long)im->files,
                (unsigned long long)(im->bytes >> 20));
    if (fclose(out) != 0) {
        fail("Cannot write %s: %s", dest, strerror(errno));
        return -1;
    }
    return 0;
}

/* Every entry of a directory. Entries form a binary tree, but they also lie
 * contiguously within each sector, so a linear walk with a resync at every
 * sector boundary reads them all without trusting the tree links. */
static int extract_dir(Image *im, uint32_t sector, uint32_t size, const char *dest)
{
    unsigned char *data;
    int rc = 0;

    if (size == 0)
        return 0;
    data = malloc(size);
    if (!data || read_at(im, sector, data, size) != 0) {
        free(data);
        fail("Cannot read the disc image's folder list at sector %u (%u bytes, for %s): "
             "the image may be cut short or damaged.", sector, size, dest);
        return -1;
    }
    for (uint32_t base = 0; base < size && rc == 0; base += SECTOR) {
        uint32_t off = 0, end = size - base < SECTOR ? size - base : SECTOR;
        while (off + 0x0E <= end && rc == 0) {
            const unsigned char *e = data + base + off;
            uint16_t l = (uint16_t)(e[0] | e[1] << 8), r = (uint16_t)(e[2] | e[3] << 8);
            uint32_t sec = e[4] | e[5] << 8 | e[6] << 16 | (uint32_t)e[7] << 24;
            uint32_t sz = e[8] | e[9] << 8 | e[10] << 16 | (uint32_t)e[11] << 24;
            unsigned attr = e[12], nlen = e[13];
            char name[256], path[4096];

            if ((l == 0xFFFF && r == 0xFFFF) || nlen == 0 || off + 0x0E + nlen > end)
                break;
            memcpy(name, e + 0x0E, nlen);
            name[nlen] = 0;
            if (strpbrk(name, "/\\:") || !strcmp(name, ".") || !strcmp(name, "..")) {
                fail("The disc image has an unexpected file name in %s: \"%s\"", dest, name);
                rc = -1;
                break;
            }
            snprintf(path, sizeof path, "%s/%s", dest, name);
            if (attr & 0x10) {
                if (!im->count_only && mkdirs(path) != 0) {
                    fail("Cannot create the folder %s: %s", path, strerror(errno));
                    rc = -1;
                } else {
                    rc = extract_dir(im, sec, sz, path);
                }
            } else if (im->count_only) {
                im->total += sz;
            } else {
                rc = copy_out(im, sec, sz, path);
            }
            off += (0x0E + nlen + 3) & ~3u;
        }
    }
    free(data);
    return rc;
}

static int extract_iso(const char *iso, const char *dest)
{
    Image im = { 0 };
    unsigned char desc[SECTOR];
    int rc = -1, found = 0;

    im.f = fopen(iso, "rb");
    if (!im.f) {
        fail("Cannot open %s: %s", iso, strerror(errno));
        return -1;
    }
    for (size_t i = 0; i < sizeof BASES / sizeof BASES[0]; i++) {
        im.base = BASES[i];
        if (read_at(&im, 32, desc, sizeof desc) == 0 &&
            !memcmp(desc, XDVDFS_MAGIC, sizeof XDVDFS_MAGIC)) {
            uint32_t root = desc[20] | desc[21] << 8 | desc[22] << 16 | (uint32_t)desc[23] << 24;
            uint32_t rsize = desc[24] | desc[25] << 8 | desc[26] << 16 | (uint32_t)desc[27] << 24;
            found = 1;
            fprintf(stderr, "Extracting the game from %s ...\n", iso);
            im.count_only = 1;
            rc = extract_dir(&im, root, rsize, dest);
            im.count_only = 0;
            if (rc == 0)
                rc = mkdirs(dest) == 0 ? extract_dir(&im, root, rsize, dest) : -1;
            if (rc != 0 && !s_err[0])
                fail("The disc image could not be read.");
            if (rc == 0)
                fprintf(stderr, "  done: %llu files, %llu MB\n",
                        (unsigned long long)im.files, (unsigned long long)(im.bytes >> 20));
            break;
        }
    }
    if (!found)
        fail("%s is not an Xbox disc image.", iso);
    fclose(im.f);
    return rc;
}

/* ---- archives ---------------------------------------------------------- */

#if defined(_WIN32)
/* argv[0] with its arguments, quoted for the command line, run without a
 * console window; its exit status, or -1 when it cannot start. */
static int run(char *const argv[])
{
    char cmd[8192];
    size_t len = 0;
    STARTUPINFOA si = { sizeof si };
    PROCESS_INFORMATION pi;
    DWORD status = 1;

    if (s_cancel)
        return -1;
    for (int i = 0; argv[i]; i++)
        len += (size_t)snprintf(cmd + len, len < sizeof cmd ? sizeof cmd - len : 0,
                                "%s\"%s\"", i ? " " : "", argv[i]);
    if (len >= sizeof cmd)
        return -1;
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
        return -1;
    CloseHandle(pi.hThread);
    s_child = pi.hProcess;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &status);
    s_child = NULL;
    CloseHandle(pi.hProcess);
    return (int)status;
}
#else
static int run(char *const argv[])
{
    pid_t pid;
    int status, rc;
    if (s_cancel)
        return -1;
    pid = fork();
    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }
    s_child = pid;
    rc = pid < 0 || waitpid(pid, &status, 0) < 0 ? -1
         : WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    s_child = 0;
    return rc;
}
#endif

static int find_iso(const char *dir, char *out, size_t n)
{
    DIR *d = opendir(dir);
    struct dirent *e;
    int found = 0;
    if (!d)
        return 0;
    while (!found && (e = readdir(d))) {
        const char *dot = strrchr(e->d_name, '.');
        if (dot && (!strcasecmp(dot, ".iso") || !strcasecmp(dot, ".xiso"))) {
            snprintf(out, n, "%s/%s", dir, e->d_name);
            found = 1;
        }
    }
    closedir(d);
    return found;
}

/* Unpack an archive with whatever the system has: bsdtar (libarchive, on
 * SteamOS and most desktops; Windows' own tar.exe is one) reads .7z and .zip,
 * else 7z/7za (on Windows also 7-Zip where its installer puts it). */
static int unpack_archive(const char *archive, const char *tmp)
{
#if defined(_WIN32)
    char *bsdtar[] = { "tar", "-xf", (char *)archive, "-C", (char *)tmp, NULL };
    char sevenzip[MAX_PATH + 32];
    const char *pf = getenv("ProgramFiles");
    char *installed[] = { sevenzip, "x", "-y", NULL, (char *)archive, NULL };
#else
    char *bsdtar[] = { "bsdtar", "-xf", (char *)archive, "-C", (char *)tmp, NULL };
#endif
    char opt[4200];
    char *sevenz[] = { "7z", "x", "-y", opt, (char *)archive, NULL };
    char *sevenza[] = { "7za", "x", "-y", opt, (char *)archive, NULL };

    snprintf(opt, sizeof opt, "-o%s", tmp);
    fprintf(stderr, "Unpacking %s (this needs about 8 GB free for a while) ...\n", archive);
    if (run(bsdtar) == 0 || run(sevenz) == 0 || run(sevenza) == 0)
        return 0;
#if defined(_WIN32)
    snprintf(sevenzip, sizeof sevenzip, "%s\\7-Zip\\7z.exe", pf && *pf ? pf : "C:\\Program Files");
    installed[3] = opt;
    if (!s_cancel && run(installed) == 0)
        return 0;
#endif
    if (s_cancel) {
        fail("Cancelled.");
        return -1;
    }
    fail("Cannot unpack %s: install bsdtar (libarchive) or 7-Zip, "
         "or extract the .iso from it yourself and choose that.", archive);
    return -1;
}

static void remove_tree(const char *path)
{
    DIR *d = opendir(path);
    struct dirent *e;
    if (d) {
        while ((e = readdir(d))) {
            char p[4096];
            if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, ".."))
                continue;
            snprintf(p, sizeof p, "%s/%s", path, e->d_name);
            if (is_dir(p))
                remove_tree(p);
            else
                unlink(p);
        }
        closedir(d);
    }
    rmdir(path);
}

/* ---- the picker -------------------------------------------------------- */

#if defined(_WIN32)
static void to_slashes(char *p)
{
    for (; *p; p++)
        if (*p == '\\')
            *p = '/';
}

/* The first start: what the game needs, then the choice -- a disc image, a folder of
 * extracted files, or quitting (0 quit, 1 image, 2 folder). A task dialog (common
 * controls 6, packaging/windows/ngb.manifest); a message box where there is none. */
static int win_welcome(const char *installed, int *portable)
{
    typedef HRESULT(WINAPI * TaskDialogIndirectFn)(const TASKDIALOGCONFIG *, int *, int *, BOOL *);
    char path[1024], text[2048];
    wchar_t wtext[2048];
    const char *lad = getenv("LOCALAPPDATA");
    size_t ln = lad ? strlen(lad) : 0;
    snprintf(path, sizeof path, "%s", installed);
    for (char *q = path; *q; q++)
        if (*q == '/')
            *q = '\\';
    if (ln && strncasecmp(path, lad, ln) == 0 && (path[ln] == '\\' || !path[ln])) {   /* short */
        char rest[1024];
        snprintf(rest, sizeof rest, "%s", path + ln);
        snprintf(path, sizeof path, "%%LOCALAPPDATA%%%s", rest);
    }
    snprintf(text, sizeof text,
             "The game itself is not included. To play, choose your own copy of the "
             "Xbox game (the retail USA release):\n\n"
             "\u2022  a disc image: .iso, including a full Redump dump, or the .7z / .zip it came in\n"
             "\u2022  or a folder of extracted files, with default.xbe\n\n"
             "A disc image is installed once (about 6 GB) into\n%s\n"
             "and the game starts from there from then on.", path);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, 2048);

    HMODULE cc = LoadLibraryA("comctl32.dll");
    TaskDialogIndirectFn td = cc ? (TaskDialogIndirectFn)(void *)GetProcAddress(cc, "TaskDialogIndirect") : NULL;
    if (td) {
        TASKDIALOG_BUTTON buttons[] = {
            { 101, L"Choose disc image..." },
            { 102, L"Choose folder..." },
        };
        TASKDIALOGCONFIG c = { sizeof c };
        int pressed = 0;
        c.hInstance = GetModuleHandleA(NULL);
        c.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
        c.cxWidth = 300;   /* dialog units: the lines unbroken */
        c.dwCommonButtons = TDCBF_CLOSE_BUTTON;
        c.pszWindowTitle = L"Ninja Gaiden Black - Setup";
        c.pszMainIcon = MAKEINTRESOURCEW(1);
        c.pszMainInstruction = L"Ninja Gaiden Black";
        c.pszContent = wtext;
        c.cButtons = 2;
        c.pButtons = buttons;
        c.nDefaultButton = 101;
        BOOL checked = FALSE;
        if (!getenv("NGB_PORTABLE"))   /* not already portable (issue #4) */
            c.pszVerificationText = L"Keep the game, saves and settings next to the program (portable)";
        if (SUCCEEDED(td(&c, &pressed, NULL, &checked))) {
            *portable = checked != FALSE;
            return pressed == 101 ? 1 : pressed == 102 ? 2 : 0;
        }
    }
    strncat(text, "\n\nYes: choose a disc image.   No: choose a folder.", sizeof text - strlen(text) - 1);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, 2048);
    switch (MessageBoxW(NULL, wtext, L"Ninja Gaiden Black - Setup", MB_YESNOCANCEL | MB_ICONINFORMATION)) {
    case IDYES:
        return 1;
    case IDNO:
        return 2;
    }
    return 0;
}

/* A folder of extracted files, with the shell's folder dialog. */
static int pick_folder(char *out, size_t n)
{
    BROWSEINFOA bi = { 0 };
    LPITEMIDLIST id;
    char path[MAX_PATH];
    int ok = 0;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    bi.lpszTitle = "Choose the folder with the game's files (default.xbe)";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;
    id = SHBrowseForFolderA(&bi);
    if (id) {
        if (SHGetPathFromIDListA(id, path)) {
            snprintf(out, n, "%s", path);
            to_slashes(out);
            ok = 1;
        }
        CoTaskMemFree(id);
    }
    CoUninitialize();
    return ok;
}

/* Ask with the standard Open dialog. The path comes back with '/' separators. */
static int pick_disc(char *out, size_t n)
{
    OPENFILENAMEA ofn = { sizeof ofn };
    out[0] = 0;
    ofn.lpstrFilter = "Xbox disc image (.iso, .7z, .zip) or default.xbe\0"
                      "*.iso;*.xiso;*.7z;*.zip;default.xbe\0All files\0*.*\0";
    ofn.lpstrFile = out;
    ofn.nMaxFile = (DWORD)n;
    ofn.lpstrTitle = "Ninja Gaiden Black: choose your disc image";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    if (!GetOpenFileNameA(&ofn))
        return 0;
    to_slashes(out);
    return 1;
}
#else
/* Ask with a desktop file dialog when there is one (zenity, kdialog). */
static int pick_disc(char *out, size_t n)
{
    const char *cmds[] = {
        "zenity --file-selection --title='Ninja Gaiden Black: choose your disc image "
        "(.iso, .7z, .zip) or its default.xbe' 2>/dev/null",
        "kdialog --title 'Ninja Gaiden Black: choose your disc image' "
        "--getopenfilename \"$HOME\" '*.iso *.xiso *.7z *.zip default.xbe' 2>/dev/null",
    };
    for (size_t i = 0; i < sizeof cmds / sizeof cmds[0]; i++) {
        FILE *p = popen(cmds[i], "r");
        if (!p)
            continue;
        if (fgets(out, (int)n, p)) {
            out[strcspn(out, "\r\n")] = 0;
            if (pclose(p) == 0 && *out)
                return 1;
        } else {
            pclose(p);
        }
    }
    return 0;
}
#endif

static int install_image(const char *src, const char *dest);

/* Install from src (an image, an archive, or a directory/default.xbe) into
 * dest: extracted beside it first and moved there whole, so an install cut
 * short never looks like a game. 0 on success, 1 for a folder to run from
 * where it is. */
static int install_from(const char *src, const char *dest)
{
    char part[4200];
    const char *dot = strrchr(src, '.');
    int rc;

    if (!(dot && (!strcasecmp(dot, ".iso") || !strcasecmp(dot, ".xiso") ||
                  !strcasecmp(dot, ".7z") || !strcasecmp(dot, ".zip"))))
        return install_image(src, dest);
    snprintf(part, sizeof part, "%s.partial", dest);
    remove_tree(part);
    s_cancel = 0;
    rc = install_image(src, part);
    if (rc == 0 && ngb_check_xbe(part) == 1 && !getenv("NGB_ANY_XBE")) {
        fail("This is not the retail USA Ninja Gaiden Black (NTSC) this "
             "build was made from.");
        rc = -1;
    }
    if (rc == 0) {
        remove_tree(dest);
        if (rename(part, dest) != 0) {
            fail("Cannot move the game into %s: %s", dest, strerror(errno));
            rc = -1;
        }
    }
    if (rc != 0)
        remove_tree(part);
    return rc;
}

static int install_image(const char *src, const char *dest)
{
    char dir[4096];
    const char *dot = strrchr(src, '.');

    /* A default.xbe or a folder of extracted files: run from there. */
    if (is_dir(src) && has_xbe(src))
        return 1;
    snprintf(dir, sizeof dir, "%s", src);
    if (dot && !strcasecmp(dot, ".xbe")) {
        char *slash = strrchr(dir, '/');
        if (slash) *slash = 0;
        if (has_xbe(dir))
            return 1;
        fail("There is no default.xbe in %s.", dir);
        return -1;
    }
    if (dot && (!strcasecmp(dot, ".iso") || !strcasecmp(dot, ".xiso")))
        return extract_iso(src, dest);
    if (dot && (!strcasecmp(dot, ".7z") || !strcasecmp(dot, ".zip"))) {
        char tmp[4200], iso[4400];
        int rc = -1;
        snprintf(tmp, sizeof tmp, "%s.unpack", dest);
        remove_tree(tmp);
        progress(0, 0);   /* no measure of an unpacker's progress */
        if (mkdirs(tmp) == 0 && unpack_archive(src, tmp) == 0) {
            if (find_iso(tmp, iso, sizeof iso))
                rc = extract_iso(iso, dest);
            else
                fail("There is no .iso inside %s.", src);
        }
        remove_tree(tmp);
        return rc;
    }
    fail("%s is not a disc image, an archive or a game folder.", src);
    return -1;
}

int ngb_install(const char *src, const char *installed, char *out, size_t n)
{
    s_err[0] = 0;
    if (is_dir(src) && has_xbe(src)) {
        snprintf(out, n, "%s", src);
    } else {
        int rc = install_from(src, installed);
        if (rc < 0)
            return -1;
        if (rc == 1) {   /* a folder or default.xbe: use it in place */
            snprintf(out, n, "%s", src);
            if (!is_dir(src)) {
                char *slash = strrchr(out, '/');
                if (slash) *slash = 0;
            }
        } else {
            snprintf(out, n, "%s", installed);
        }
    }

    switch (ngb_check_xbe(out)) {
    case 0:
        return 0;
    case 1:
        if (getenv("NGB_ANY_XBE"))
            return 0;
        fail("%s/default.xbe is not the retail USA Ninja Gaiden Black (NTSC) "
             "this build was made from.", out);
        return -1;
    default:
        fail("There is no default.xbe in %s.", out);
        return -1;
    }
}

#if !defined(_WIN32)
/* The setup window (ngb-setup, Qt) beside the executable: it asks for the
 * disc, installs it into installed and prints the game folder. 1 when it
 * did, 0 when the player closed it, -1 when it could not run. */
static int run_setup(const char *exe_dir, const char *installed,
                     char *out, size_t n)
{
    char cmd[8400];
    FILE *p;
    int status;

    snprintf(cmd, sizeof cmd, "%sngb-setup", exe_dir);
    if (access(cmd, X_OK) != 0)
        return -1;
    /* Its own Qt's plugins only (a bundled Qt), not the desktop's. */
    snprintf(cmd, sizeof cmd, "env -u QT_PLUGIN_PATH '%sngb-setup' --install-to '%s'%s",
             exe_dir, installed, getenv("NGB_PORTABLE") ? "" : " --portable");
    if (strchr(exe_dir, '\'') || strchr(installed, '\''))
        return -1;
    p = popen(cmd, "r");
    if (!p)
        return -1;
    out[0] = 0;
    if (fgets(out, (int)n, p))
        out[strcspn(out, "\r\n")] = 0;
    status = pclose(p);
    if (!WIFEXITED(status) || WEXITSTATUS(status) > 1)
        return -1;      /* no display, missing libraries: the fallbacks */
    if (WEXITSTATUS(status) == 0 && !strncmp(out, "portable:", 9))
    if (WEXITSTATUS(status) == 0 && !strncmp(out, "portable:", 9)) {
        restart_portable(out + 9);   /* portable mode chosen (the setup made portable.txt) */
        out[0] = 0;
    }
    return WEXITSTATUS(status) == 0 && out[0] ? 1 : 0;
}
#endif

#if defined(_WIN32)
/* A small window with a progress bar while ngb_install runs on a worker
 * thread; closing it cancels. A failure is told in a message box. */
typedef struct {
    const char *src, *installed;
    char *out;
    size_t n;
    int rc;
    HWND wnd, bar;
} WinJob;

#define WM_NGB_PROGRESS (WM_APP + 1)   /* wParam: per mille, or -1 for no measure */
#define WM_NGB_DONE     (WM_APP + 2)

static void win_progress(uint64_t done, uint64_t total, void *ud)
{
    WinJob *job = ud;
    static int last = -2;
    int pm = total ? (int)(done * 1000 / total) : -1;
    if (pm != last) {
        last = pm;
        PostMessageA(job->wnd, WM_NGB_PROGRESS, (WPARAM)pm, 0);
    }
}

static DWORD WINAPI win_worker(LPVOID arg)
{
    WinJob *job = arg;
    job->rc = ngb_install(job->src, job->installed, job->out, job->n);
    PostMessageA(job->wnd, WM_NGB_DONE, 0, 0);
    return 0;
}

static LRESULT CALLBACK win_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    WinJob *job = (WinJob *)GetWindowLongPtrA(h, GWLP_USERDATA);
    switch (msg) {
    case WM_NGB_PROGRESS:
        if ((int)wp < 0) {
            SetWindowLongA(job->bar, GWL_STYLE, GetWindowLongA(job->bar, GWL_STYLE) | PBS_MARQUEE);
            SendMessageA(job->bar, PBM_SETMARQUEE, TRUE, 30);
        } else {
            SendMessageA(job->bar, PBM_SETMARQUEE, FALSE, 0);
            SetWindowLongA(job->bar, GWL_STYLE, GetWindowLongA(job->bar, GWL_STYLE) & ~PBS_MARQUEE);
            SendMessageA(job->bar, PBM_SETPOS, wp, 0);
        }
        return 0;
    case WM_NGB_DONE:
        DestroyWindow(h);
        return 0;
    case WM_CLOSE:
        ngb_install_cancel();   /* the worker fails and posts WM_NGB_DONE */
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(h, msg, wp, lp);
}

static int win_install(const char *src, const char *installed, char *out, size_t n)
{
    WinJob job = { src, installed, out, n, -1, NULL, NULL };
    INITCOMMONCONTROLSEX icc = { sizeof icc, ICC_PROGRESS_CLASS };
    WNDCLASSA wc = { 0 };
    HINSTANCE inst = GetModuleHandleA(NULL);
    HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND text;
    HANDLE th;
    MSG m;
    RECT r = { 0, 0, 440, 96 };

    InitCommonControlsEx(&icc);
    wc.lpfnWndProc = win_proc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "NgbInstall";
    RegisterClassA(&wc);
    AdjustWindowRect(&r, WS_CAPTION | WS_SYSMENU, FALSE);
    job.wnd = CreateWindowA("NgbInstall", "Ninja Gaiden Black", WS_CAPTION | WS_SYSMENU,
                            CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                            NULL, NULL, inst, NULL);
    if (!job.wnd)
        return ngb_install(src, installed, out, n);
    SetWindowLongPtrA(job.wnd, GWLP_USERDATA, (LONG_PTR)&job);
    text = CreateWindowA("STATIC", "Installing the game from your disc image. This takes a minute or two.",
                         WS_CHILD | WS_VISIBLE, 16, 16, 408, 20, job.wnd, NULL, inst, NULL);
    SendMessageA(text, WM_SETFONT, (WPARAM)font, TRUE);
    job.bar = CreateWindowA(PROGRESS_CLASSA, NULL, WS_CHILD | WS_VISIBLE,
                            16, 48, 408, 22, job.wnd, NULL, inst, NULL);
    SendMessageA(job.bar, PBM_SETRANGE, 0, MAKELPARAM(0, 1000));
    ShowWindow(job.wnd, SW_SHOW);

    ngb_install_set_progress(win_progress, &job);
    th = CreateThread(NULL, 0, win_worker, &job, 0, NULL);
    if (!th) {
        DestroyWindow(job.wnd);
        ngb_install_set_progress(NULL, NULL);
        return ngb_install(src, installed, out, n);
    }
    while (GetMessageA(&m, NULL, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
    WaitForSingleObject(th, INFINITE);
    CloseHandle(th);
    ngb_install_set_progress(NULL, NULL);
    return job.rc;
}

static void win_tell(const char *msg)
{
    MessageBoxA(NULL, msg, "Ninja Gaiden Black", MB_OK | MB_ICONERROR);
}
#endif

int ngb_portable_base(char *dir, size_t n)
{
    const char *appimage = getenv("APPIMAGE");
    char *slash;

    if (appimage && *appimage)
        snprintf(dir, n, "%s", appimage);
    else if (ngb_exe_path(dir, n) <= 0)
        return 0;
    if (!(slash = strrchr(dir, '/')))
        return 0;
    *slash = 0;
#ifdef __APPLE__
    const char *bundle = strstr(dir, ".app/Contents/MacOS");
    if (bundle && !bundle[strlen(".app/Contents/MacOS")]) {
        dir[bundle - dir + 4] = 0;
        if (!(slash = strrchr(dir, '/')))
            return 0;
        *slash = 0;
    }
#endif
    return 1;
}

int ngb_portable_enable(void)
{
    char dir[4096], p[4200];
    FILE *f;

    if (!ngb_portable_base(dir, sizeof dir))
        return 0;
    snprintf(p, sizeof p, "%s/portable.txt", dir);
    if (!(f = fopen(p, "w")))
        return 0;   /* not writable there (Program Files, a read-only mount) */
    fprintf(f, "This file makes Ninja Gaiden Black keep its game files, saves and settings in the "
               "\"userdata\" folder here. Delete it to use your profile's folders again.\n");
    fclose(f);
    return 1;
}

/* Portable mode just chosen in the setup: the program starts again -- its folders are picked at the very
 * start (src/main.c) -- with the disc image or folder chosen, so it is not asked for twice. */
static void restart_portable(const char *src)
{
#if defined(_WIN32)
    wchar_t self[MAX_PATH], wsrc[4096], cmd[8400];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    if (!GetModuleFileNameW(NULL, self, MAX_PATH) || !MultiByteToWideChar(CP_ACP, 0, src, -1, wsrc, 4096))
        return;
    _snwprintf(cmd, 8400, L"\"%ls\" \"%ls\"", self, wsrc);
    if (CreateProcessW(self, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        TerminateProcess(GetCurrentProcess(), 0);
    }
#else
    char self[4096];
    const char *appimage = getenv("APPIMAGE");
    char *argv[2];

    if (appimage && *appimage)
        snprintf(self, sizeof self, "%s", appimage);
    else if (ngb_exe_path(self, sizeof self) <= 0)
        return;
    setenv("NGB_SETUP_AUTO", src, 1);   /* the setup window installs it straight away (src/setup) */
    argv[0] = self;
    argv[1] = NULL;
    execv(self, argv);
#endif
    fprintf(stderr, "Could not start again for portable mode; carrying on as before.\n");
}

/* A folder of extracted files the player picked runs where it is -- and was asked for again at the
 * next start, since only the installed copy and a "game" folder beside the program were looked for
 * (issue #14). Its path is kept in the save folder, and found there first. */
static void location_file(const char *save_dir, char *p, size_t n)
{
    snprintf(p, n, "%s/game-location.txt", save_dir);
}

static int remembered_game(const char *save_dir, char *out, size_t n)
{
    char p[4200], line[4096];
    FILE *f;

    location_file(save_dir, p, sizeof p);
    if (!(f = fopen(p, "r")))
        return 0;
    if (!fgets(line, sizeof line, f))
        line[0] = 0;
    fclose(f);
    line[strcspn(line, "\r\n")] = 0;
    if (!line[0] || !has_xbe(line))
        return 0;   /* moved or deleted: asked for again */
    snprintf(out, n, "%s", line);
    return 1;
}

static void remember_game(const char *save_dir, const char *installed, const char *dir)
{
    char p[4200];
    FILE *f;

    if (!strcmp(dir, installed))
        return;   /* installed: found there anyway */
    location_file(save_dir, p, sizeof p);
    if ((f = fopen(p, "w"))) {
        fprintf(f, "%s\n", dir);
        fclose(f);
    }
}

int ngb_find_game(const char *arg, const char *save_dir, const char *exe_dir,
                  char *out, size_t n)
{
    char installed[4096], beside[4096], picked[4096];
    const char *env = getenv("NGB_GAME_DIR");
    const char *src = arg && *arg ? arg : NULL;

    snprintf(installed, sizeof installed, "%s/game", save_dir);
    snprintf(beside, sizeof beside, "%sgame", exe_dir);

    int from_env = 0;

    if (!src && env && *env) {
        src = env;
        from_env = 1;
    }
    if (!src) {
        if (has_xbe(installed)) { snprintf(out, n, "%s", installed); goto check; }
        if (has_xbe(beside))    { snprintf(out, n, "%s", beside);    goto check; }
        if (remembered_game(save_dir, out, n)) goto check;
#if !defined(_WIN32)
        switch (run_setup(exe_dir, installed, out, n)) {
        case 1:
            remember_game(save_dir, installed, out);   /* a folder picked: found there next time */
            return 0;   /* installed and checked there */
        case 0:
            return -1;  /* closed */
        }
        if (pick_disc(picked, sizeof picked))
            src = picked;
#else
        int portable = 0;
        for (;;) {   /* the welcome again when a picker is closed */
            int how = win_welcome(installed, &portable);
            if (how == 0)
                return -1;  /* quit */
            if (how == 1 ? pick_disc(picked, sizeof picked) : pick_folder(picked, sizeof picked))
                break;
        }
        src = picked;
        if (portable) {
            if (ngb_portable_enable())
                restart_portable(src);   /* does not come back when it works */
            else
                win_tell("Portable mode needs a folder the program can write to (not Program Files). "
                         "The game is installed in your profile instead.");
        }
#endif
        if (!src) {
            fprintf(stderr,
                "No game files found. Run it once with your disc image:\n"
                "  NinjaGaidenBlack /path/to/disc.iso   (or the .7z, or a folder "
                "of extracted files)\n"
                "It is installed into %s and found there from then on.\n", installed);
            return -1;
        }
    }
#if defined(_WIN32)
    if (win_install(src, installed, out, n) == 0) {
        if (!from_env)
            remember_game(save_dir, installed, out);
        return 0;
    }
    {
        char msg[1200];
        snprintf(msg, sizeof msg, "The game could not be installed.\n\n%s", ngb_install_error());
        win_tell(msg);
    }
    return -1;
#else
    if (ngb_install(src, installed, out, n) != 0)
        return -1;
    if (!from_env)
        remember_game(save_dir, installed, out);
    return 0;
#endif

check:
    switch (ngb_check_xbe(out)) {
    case 0:
        return 0;
    case 1:
        fprintf(stderr, "%s/default.xbe is not the retail USA Ninja Gaiden Black "
                        "this build was made from.\n", out);
        if (getenv("NGB_ANY_XBE"))
            return 0;
#if defined(_WIN32)
        win_tell("The installed game is not the retail USA Ninja Gaiden Black (NTSC) "
                 "this build was made from.");
#endif
        return -1;
    default:
        fprintf(stderr, "No default.xbe in %s\n", out);
        return -1;
    }
}
