/*
 * kernel_path.c - Xbox device-path translation
 *
 * Translates Xbox device-style paths to host filesystem paths:
 *   \Device\CdRom0\  -> <game_dir>/
 *   D:\               -> <game_dir>/
 *   T:\               -> <save_dir>/TitleData/
 *   U:\               -> <save_dir>/UserData/
 *   Z:\               -> <save_dir>/Cache/
 *
 * The Win32 build emits UTF-16 paths (for CreateFileW); the Linux build
 * emits UTF-8 paths with '/' separators (for open()).
 */

#include "kernel.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <wctype.h>

/*
 * Helper: check if an ANSI string starts with a prefix (case-insensitive).
 * Returns the number of chars consumed from the prefix, or 0 if no match.
 * Platform-independent.
 */
static int match_prefix(const char* path, const char* prefix)
{
    int i = 0;
    while (prefix[i]) {
        if (tolower((unsigned char)path[i]) != tolower((unsigned char)prefix[i]))
            return 0;
        i++;
    }
    return i;
}

/* A device-path translation rule, shared by both backends. */
typedef struct {
    const char* prefix;     /* Xbox path prefix (backslash form)         */
    int         to_save;    /* 1 = under save_dir, 0 = under game_dir    */
    const char* sub_win;    /* sub-directory, Win32 backslash form       */
    const char* sub_posix;  /* sub-directory, POSIX slash form           */
} path_rule;

static const path_rule s_rules[] = {
    { "\\Device\\CdRom0\\",                   0, NULL,         NULL          },
    /* E:\TDATA and E:\UDATA are title settings and saves: they belong in
     * the save directory (TitleData/<title id>, UserData/<title id>), not
     * among the disc's files where a reinstall would take them away. */
    { "\\Device\\Harddisk0\\Partition1\\TDATA",  1, "\\TitleData","/TitleData"  },
    { "\\Device\\Harddisk0\\Partition1\\UDATA",  1, "\\UserData", "/UserData"   },
    { "\\Device\\Harddisk0\\Partition1\\",    0, NULL,         NULL          },
    /* The rest of the disk. Partition 0 is the whole raw device, 2 holds
     * system data, and 3-5 are the per-title caches behind X:, Y: and Z:.
     * Without these the path layer reported "Unrecognized Xbox path" and
     * NtOpenFile returned STATUS_OBJECT_PATH_NOT_FOUND; DSTEAL_JP probes
     * partition0 at startup and treats the failure as fatal.
     *
     * No trailing separator: the probe opens the device itself, with nothing
     * after it. Listed after Partition1 so that keeps its own mapping. */
    /* Partition2 is C:, the system partition. On a console that is where the
     * dashboard and its own assets live, and the dashboard opens them by
     * device path as well as through Y:, so a path *under* partition2 is an
     * ordinary asset read and belongs in the game dir -- the same place Y:
     * already goes. Only the bare device keeps the system-data image, which
     * is why this rule carries the trailing separator and is listed first. */
    { "\\Device\\Harddisk0\\Partition2\\",    0, NULL,         NULL          },
    { "C:\\",                                 0, NULL,         NULL          },
    { "\\??\\C:\\",                           0, NULL,         NULL          },
    { "\\Device\\Harddisk0\\Partition2",     1, "\\SystemData", "/SystemData" },
    { "\\Device\\Harddisk0\\Partition3",     1, "\\Cache",   "/Cache"      },
    { "\\Device\\Harddisk0\\Partition4",     1, "\\Cache",   "/Cache"      },
    { "\\Device\\Harddisk0\\Partition5",     1, "\\Cache",   "/Cache"      },
    { "D:\\",                                 0, NULL,         NULL          },
    { "d:\\",                                 0, NULL,         NULL          },
    /* Y: is the Xbox dashboard partition; the dashboard opens its assets
     * (e.g. "Y:\default.xip") from there. Map it to the game dir. */
    { "Y:\\",                                 0, NULL,         NULL          },
    { "y:\\",                                 0, NULL,         NULL          },
    { "T:\\",                                 1, "\\TitleData","/TitleData"  },
    { "U:\\",                                 1, "\\UserData", "/UserData"   },
    { "Z:\\",                                 1, "\\Cache",    "/Cache"      },
    { "\\??\\D:\\",                           0, NULL,         NULL          },
    { "\\??\\Y:\\",                           0, NULL,         NULL          },
    { "\\??\\y:\\",                           0, NULL,         NULL          },
    { "\\??\\T:\\",                           1, "\\TitleData","/TitleData"  },
};
#define PATH_RULE_COUNT ((int)(sizeof(s_rules) / sizeof(s_rules[0])))

/*
 * Rewrite a path through a drive letter the title mapped for itself.
 *
 * Titles do their own mounting. Wreckless calls IoCreateSymbolicLink at guest
 * 0x000E9B1D to link "\??\Z:" to "\Device\Harddisk0\Partition1\", then loads
 * every asset through z:\. kernel_io.c has recorded that link since it was
 * written, but nothing ever read the table back, so the static rule below --
 * Z: is the cache partition, which is true of the console in general and wrong
 * for this title -- sent every asset open to the save directory and it failed
 * with ERROR_FILE_NOT_FOUND.
 *
 * Accepts "Z:\rest" and "\??\Z:\rest"; the table is keyed on the "\??\Z:" form
 * the kernel actually registers.
 *
 * Deliberately conservative: the rewritten path is returned only when it
 * matches a static rule. A link whose target this layer has no rule for would
 * otherwise turn a path that translated adequately into one that does not
 * translate at all.
 *
 * Returns 1 and fills `out` on success, 0 to leave the path alone.
 */
static int resolve_symlink(const char* xbox_path, char* out, size_t out_size)
{
    char        link[8];
    const char* rest;
    const char* target;
    size_t      tlen, rlen;
    int         i;

    if (!xbox_path || !out || out_size == 0)
        return 0;

    if (match_prefix(xbox_path, "\\??\\") && xbox_path[4] && xbox_path[5] == ':')
        xbox_path += 4;
    if (!xbox_path[0] || xbox_path[1] != ':')
        return 0;
    rest = xbox_path + 2;

    link[0] = '\\';
    link[1] = '?';
    link[2] = '?';
    link[3] = '\\';
    link[4] = (char)toupper((unsigned char)xbox_path[0]);
    link[5] = ':';
    link[6] = '\0';

    target = xbox_LookupSymbolicLink(link);
    if (!target || !target[0])
        return 0;

    while (*rest == '\\' || *rest == '/')
        rest++;

    tlen = strlen(target);
    rlen = strlen(rest);
    if (tlen + rlen + 2 > out_size)
        return 0;

    memcpy(out, target, tlen);
    if (tlen && target[tlen - 1] != '\\' && target[tlen - 1] != '/')
        out[tlen++] = '\\';
    memcpy(out + tlen, rest, rlen);
    out[tlen + rlen] = '\0';

    for (i = 0; i < PATH_RULE_COUNT; i++) {
        if (match_prefix(out, s_rules[i].prefix))
            return 1;           /* the target is somewhere we can place */
    }
    return 0;
}

/* ======================================================================== */
/* Shared by both backends, for the same reason as xbox_LastFileError: the
 * bridge calls it unconditionally, and a _WIN32-only definition breaks every
 * POSIX link. The Win32 backend fills this during translation; the POSIX one
 * does not yet, so it reads empty there. */
static XBOX_THREAD_LOCAL wchar_t s_last_host_path_shared[MAX_PATH];

const wchar_t *xbox_LastHostPath(void)
{
    return s_last_host_path_shared;
}

#if defined(_WIN32)
/* ======================================================================== */

#include <shlobj.h>
#include <winioctl.h>

static WCHAR s_game_dir[MAX_PATH];
static WCHAR s_save_dir[MAX_PATH];
static BOOL  s_initialized = FALSE;
static void mods_restore_cache(void);

/*
 * The raw disk device, \Device\Harddisk0\Partition0.
 *
 * A title opens it to read the Xbox partition table at sector 4 (offset 0x800)
 * and find the cache partitions behind X:, Y: and Z: -- XAPI's utility-drive
 * mount does exactly that, and DSTEAL_JP calls HalReturnToFirmware when the
 * read fails. A directory cannot answer a 512-byte read at a file offset, so
 * the device is backed by an image file instead.
 *
 * The table is the standard retail geometry. This is emulating a device that
 * has to be there, not fabricating anything the title owns.
 */
#define XBOX_DISK_IMAGE_NAME   L"Partition0.img"
#define XBOX_PART_TABLE_OFFSET 0x800
#define XBOX_PART_IN_USE       0x80000000u

static void xbox_write_partition_table(const WCHAR *path)
{
    /* name[16], flags, lba_start, lba_size, reserved -- 32 bytes each */
    static const struct { const char *name; ULONG start, size; } parts[] = {
        { "XBOX_PART_X",  0x00000400, 0x00177000 },  /* X: cache      */
        { "XBOX_PART_Y",  0x00177400, 0x00177000 },  /* Y: cache      */
        { "XBOX_PART_Z",  0x002EE400, 0x00177000 },  /* Z: cache      */
        { "XBOX_PART_C",  0x00465400, 0x000FA000 },  /* C: system     */
        { "XBOX_PART_E",  0x0055F400, 0x00465400 },  /* E: game/save  */
    };
    unsigned char sector[512];
    HANDLE h;
    DWORD written;
    LARGE_INTEGER off;
    size_t i;

    h = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                    OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;

    memset(sector, 0, sizeof(sector));
    memcpy(sector, "****PARTINFO****", 16);
    for (i = 0; i < sizeof(parts) / sizeof(parts[0]); i++) {
        unsigned char *e = sector + 48 + i * 32;   /* 16 magic + 32 reserved */
        size_t n = strlen(parts[i].name);
        memset(e, ' ', 16);
        memcpy(e, parts[i].name, n < 16 ? n : 16);
        *(ULONG *)(e + 16) = XBOX_PART_IN_USE;
        *(ULONG *)(e + 20) = parts[i].start;
        *(ULONG *)(e + 24) = parts[i].size;
        *(ULONG *)(e + 28) = 0;
    }

    off.QuadPart = XBOX_PART_TABLE_OFFSET;
    if (SetFilePointerEx(h, off, NULL, FILE_BEGIN))
        WriteFile(h, sector, sizeof(sector), &written, NULL);
    CloseHandle(h);
}

/*
 * Map a partition device onto an image file.
 *
 * Matches "\Device\Harddisk0\PartitionN" with nothing below it, which is how a
 * device is opened. Anything with a path under it -- Partition1\TDATA and the
 * like -- is an ordinary filesystem access and falls through to the rules
 * table, which puts it in a directory.
 *
 * Partition0 is the whole disk and carries the partition table written above.
 * The rest start empty, which is what an unformatted partition looks like, so
 * a title that wants a filesystem there formats one.
 */
static BOOL xbox_partition_device_path(const char *xbox_path, WCHAR *out, DWORD n)
{
    static const char *prefix = "\\Device\\Harddisk0\\Partition";
    int len = match_prefix(xbox_path, prefix);
    int digit;
    const char *rest;

    if (!len)
        return FALSE;
    digit = xbox_path[len];
    if (digit < '0' || digit > '9')
        return FALSE;

    rest = xbox_path + len + 1;
    /* Nothing below it. A trailing separator names the root directory of the
     * partition's filesystem, not the volume -- XAPI opens
     * "\\Device\\Harddisk0\\partition1\\" as a directory to check E: is
     * there, and routing that to the image failed it and sent the title to the
     * dashboard with XLD_ERROR_INVALID_HARD_DISK (Ninja Gaiden Black). Only
     * Partition0, the whole disk, has no filesystem to have a root. */
    if ((*rest == '\\' || *rest == '/') && digit == '0')
        rest++;
    if (*rest != '\0')
        return FALSE;

    swprintf_s(out, n, L"%s\\Partition%c.img", s_save_dir, (WCHAR)digit);
    return TRUE;
}

void xbox_path_init(const char* game_dir, const char* save_dir)
{
    WCHAR save_base[MAX_PATH];

    /* The fallbacks used to name Burnout 3 specifically, so any other title
     * that passed NULL silently pointed its game dir and its saves at another
     * game's folders. Generic now -- a caller that wants a title-specific
     * location should pass one. */
    if (game_dir) {
        MultiByteToWideChar(CP_UTF8, 0, game_dir, -1, s_game_dir, MAX_PATH);
    } else {
        /* Neutral default: the CWD's "game" subdirectory, which is the
         * layout the game projects use (see templates/new-game). */
        GetCurrentDirectoryW(MAX_PATH, s_game_dir);
    }

    if (save_dir) {
        MultiByteToWideChar(CP_UTF8, 0, save_dir, -1, s_save_dir, MAX_PATH);
    } else {
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, save_base))) {
            swprintf_s(s_save_dir, MAX_PATH, L"%s\\xboxrecomp", save_base);
        } else {
            GetCurrentDirectoryW(MAX_PATH, s_save_dir);
            wcscat_s(s_save_dir, MAX_PATH, L"\\SaveData");
        }
    }

    size_t len = wcslen(s_game_dir);
    if (len > 0 && (s_game_dir[len - 1] == L'\\' || s_game_dir[len - 1] == L'/'))   /* (a picker's D:/) */
        s_game_dir[len - 1] = L'\0';

    len = wcslen(s_save_dir);
    if (len > 0 && s_save_dir[len - 1] == L'\\')
        s_save_dir[len - 1] = L'\0';

    /* Create the save-side directories. T:/U:/Z: map into subdirectories of
     * save_dir, and a title that opens a file there with a create disposition
     * fails if the parent does not exist -- which reads as "cannot create save
     * file" and sends the title down its init-failure path. Halo asserts
     * exactly that at saved games/game_state_xbox.c:97 and then unwinds,
     * clearing global_d3d_device on the way out, so a missing directory
     * surfaces as a graphics failure.
     *
     * Cheap and idempotent: SHCreateDirectoryExW builds intermediates and is
     * happy if they already exist. */
    {
        static const WCHAR *subs[] = { L"TitleData", L"UserData", L"Cache",
                                       L"SystemData" };
        WCHAR image[MAX_PATH];
        WCHAR dir[MAX_PATH];
        SHCreateDirectoryExW(NULL, s_save_dir, NULL);
        for (int i = 0; i < (int)(sizeof(subs) / sizeof(subs[0])); i++) {
            swprintf_s(dir, MAX_PATH, L"%s\\%s", s_save_dir, subs[i]);
            SHCreateDirectoryExW(NULL, dir, NULL);
        }
        swprintf_s(image, MAX_PATH, L"%s\\%s", s_save_dir, XBOX_DISK_IMAGE_NAME);
        xbox_write_partition_table(image);
        /* The other partition devices, sized to the same geometry the table
         * above describes, so a title that asks the device how big it is gets
         * an answer consistent with the table it just read. Marked sparse
         * first: the cache partitions are 750 MB each and none of that is
         * touched until something writes to it. */
        {
            static const ULONGLONG part_sectors[6] = {
                0,             /* 0: whole disk, sized below      */
                0x00465400ull, /* 1: E: game and saves            */
                0x000FA000ull, /* 2: C: system                    */
                0x00177000ull, /* 3: X: cache                     */
                0x00177000ull, /* 4: Y: cache                     */
                0x00177000ull, /* 5: Z: cache                     */
            };
            for (int p = 1; p <= 5; p++) {
                HANDLE h;
                DWORD ret;
                LARGE_INTEGER end;

                swprintf_s(image, MAX_PATH, L"%s\\Partition%d.img",
                           s_save_dir, p);
                h = CreateFileW(image, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                                OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (h == INVALID_HANDLE_VALUE)
                    continue;
                DeviceIoControl(h, FSCTL_SET_SPARSE, NULL, 0, NULL, 0,
                                &ret, NULL);
                end.QuadPart = (LONGLONG)(part_sectors[p] * 512ull);
                if (SetFilePointerEx(h, end, NULL, FILE_BEGIN))
                    SetEndOfFile(h);
                CloseHandle(h);
            }
        }
    }

    s_initialized = TRUE;
    xbox_log(XBOX_LOG_INFO, XBOX_LOG_PATH, "Path init: game=%S, save=%S", s_game_dir, s_save_dir);
    mods_restore_cache();
}

/* The host path the last translation produced.
 *
 * A caller that wants to act on the file a title just opened -- playing an FMV
 * the host can decode itself, say -- has the guest path but not the host one,
 * and re-deriving it would duplicate every rule above. */
/* Thread-local: several threads open files at once, and a plain static let one
 * thread's translation overwrite another's between the translate and the read.
 * That showed up as the FMV trigger firing on roughly two runs in three. */

static void xbox_remember_host_path(const wchar_t *p)
{
    if (p) wcsncpy_s(s_last_host_path_shared, MAX_PATH, p, _TRUNCATE);
}



BOOL xbox_translate_path(const char* xbox_path, xbox_host_char* host_path_buf, DWORD buf_size)
{
    const char*  remainder = NULL;
    const WCHAR* base_dir  = NULL;
    const char*  sub_dir   = NULL;
    int          skip;

    if (!xbox_path || !host_path_buf || buf_size == 0)
        return FALSE;

    if (!s_initialized)
        xbox_path_init(NULL, NULL);

    {
        char linked[512];
        if (resolve_symlink(xbox_path, linked, sizeof(linked)))
            return xbox_translate_path(linked, host_path_buf, buf_size);
    }

    if (xbox_partition_device_path(xbox_path, host_path_buf, buf_size)) {
        fprintf(stderr, "  [PATH] %s -> partition image\n", xbox_path);
        fflush(stderr);
        return TRUE;
    }

    for (int i = 0; i < PATH_RULE_COUNT; i++) {
        skip = match_prefix(xbox_path, s_rules[i].prefix);
        if (skip) {
            remainder = xbox_path + skip;
            base_dir  = s_rules[i].to_save ? s_save_dir : s_game_dir;
            sub_dir   = s_rules[i].sub_win;
            goto translate;
        }
    }

    xbox_log(XBOX_LOG_WARN, XBOX_LOG_PATH, "Unrecognized Xbox path: %s", xbox_path);
    MultiByteToWideChar(CP_ACP, 0, xbox_path, -1, host_path_buf, buf_size);
    return TRUE;

translate:
    fprintf(stderr, "  [PATH] %s\n", xbox_path);
    fflush(stderr);
    {
        WCHAR remainder_wide[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, remainder, -1, remainder_wide, MAX_PATH);

        for (WCHAR* p = remainder_wide; *p; p++) {
            if (*p == L'/') *p = L'\\';
        }

        if (sub_dir) {
            WCHAR sub_wide[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, sub_dir, -1, sub_wide, MAX_PATH);
            swprintf_s(host_path_buf, buf_size, L"%s%s\\%s", base_dir, sub_wide, remainder_wide);

            WCHAR dir_path[MAX_PATH];
            swprintf_s(dir_path, MAX_PATH, L"%s%s", base_dir, sub_wide);
            CreateDirectoryW(s_save_dir, NULL);
            CreateDirectoryW(dir_path, NULL);
        } else {
            swprintf_s(host_path_buf, buf_size, L"%s\\%s", base_dir, remainder_wide);
        }

        /* An empty remainder means the title opened the device itself, so
         * the join above leaves a trailing separator -- which CreateFileW
         * rejects even with FILE_FLAG_BACKUP_SEMANTICS, turning an existing
         * directory into STATUS_OBJECT_PATH_NOT_FOUND. */
        {
            size_t n = wcslen(host_path_buf);
            while (n > 1 && (host_path_buf[n - 1] == L'\\'
                             || host_path_buf[n - 1] == L'/'))
                host_path_buf[--n] = L'\0';
        }

        XBOX_TRACE(XBOX_LOG_PATH, "%s -> %S", xbox_path, host_path_buf);
        xbox_remember_host_path(host_path_buf);
        return TRUE;
    }
}

/* ======================================================================== */

/* ---- mods (#179) -------------------------------------------------------
 *
 * A game file the player put under <save dir>\mods with the same path -- the disc's (chr_ryu.afs,
 * movie\cre.sfd), which the title also copies into its hard disk cache (Z:, the save dir's Cache, then read
 * from there) -- is read instead. Data only: nothing of a mod's runs. Only an existing file opened to read
 * (NtCreateFile), its size in a directory search (NtQueryDirectoryFile, how CRI's file layer sizes a file),
 * and the quick menu's own icon reads (src/quick_ui.c) take it; writes (the cache's copies) never do. The
 * mods folder is looked for once. */
static int s_mods = -1;
static WCHAR s_game_final[MAX_PATH], s_cache_final[MAX_PATH];   /* the two as Windows resolves them (links, drives) */

/* p without the \\?\ (\\?\UNC\) that Windows' own resolved paths carry, into out */
static void plain_path(const WCHAR* p, WCHAR* out, size_t n)
{
    if (!wcsncmp(p, L"\\\\?\\UNC\\", 8))
        swprintf_s(out, n, L"\\\\%s", p + 8);
    else
        wcsncpy_s(out, n, !wcsncmp(p, L"\\\\?\\", 4) ? p + 4 : p, _TRUNCATE);
}

static void final_path(const WCHAR* p, WCHAR* out)
{
    WCHAR b[MAX_PATH];
    DWORD n;
    HANDLE h = CreateFileW(p, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS, NULL);
    out[0] = 0;
    if (h == INVALID_HANDLE_VALUE)
        return;
    n = GetFinalPathNameByHandleW(h, b, MAX_PATH, FILE_NAME_NORMALIZED);
    CloseHandle(h);
    if (n && n < MAX_PATH)
        plain_path(b, out, MAX_PATH);
}

/* The game files taken from mods so far (their cache copies are the mod's): <save>\mods-cache.txt, one path a
 * line, so that a mod deleted later has the game's own file put back over its copy (mods_restore_cache). */
#define MODS_NOTED_MAX 1024
static WCHAR s_noted[MODS_NOTED_MAX][MAX_PATH];
static int s_noted_n;
static SRWLOCK s_noted_lock = SRWLOCK_INIT;

static void mods_note(const WCHAR* rel)
{
    WCHAR list[MAX_PATH];
    char line[MAX_PATH * 3];
    FILE* f;
    AcquireSRWLockExclusive(&s_noted_lock);
    for (int i = 0; i < s_noted_n; i++)
        if (!_wcsicmp(s_noted[i], rel)) {
            ReleaseSRWLockExclusive(&s_noted_lock);
            return;
        }
    if (s_noted_n >= MODS_NOTED_MAX) {            /* (full: its lines would only repeat) */
        ReleaseSRWLockExclusive(&s_noted_lock);
        return;
    }
    wcsncpy_s(s_noted[s_noted_n++], MAX_PATH, rel, _TRUNCATE);
    swprintf_s(list, MAX_PATH, L"%s\\mods-cache.txt", s_save_dir);
    if (WideCharToMultiByte(CP_UTF8, 0, rel, -1, line, sizeof line, NULL, NULL) > 0 && (f = _wfopen(list, L"ab"))) {
        fprintf(f, "%s\n", line);
        fclose(f);
    }
    ReleaseSRWLockExclusive(&s_noted_lock);
}

static BOOL is_file(const WCHAR* p)
{
    DWORD a = GetFileAttributesW(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

/* At start: each noted file whose mod is gone gets the game's own file back over its cache copy (the game keeps
 * a cached file, under the same name and size, without checking it again). */
static void mods_restore_cache(void)
{
    WCHAR list[MAX_PATH], tmp[MAX_PATH], rel[MAX_PATH], a[MAX_PATH * 2], b[MAX_PATH * 2];
    char line[MAX_PATH * 3];
    FILE *f, *out;
    int kept = 0;
    swprintf_s(list, MAX_PATH, L"%s\\mods-cache.txt", s_save_dir);
    swprintf_s(tmp, MAX_PATH, L"%s\\mods-cache.new", s_save_dir);
    if (!(f = _wfopen(list, L"rb")))
        return;
    out = _wfopen(tmp, L"wb");
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0] || !MultiByteToWideChar(CP_UTF8, 0, line, -1, rel, MAX_PATH))
            continue;
        swprintf_s(a, MAX_PATH * 2, L"%s\\mods\\%s", s_save_dir, rel);
        if (is_file(a)) {                          /* (still a mod: kept) */
            if (kept < MODS_NOTED_MAX)
                wcsncpy_s(s_noted[kept], MAX_PATH, rel, _TRUNCATE);
            kept++;
            if (out)
                fprintf(out, "%s\n", line);
            continue;
        }
        swprintf_s(a, MAX_PATH * 2, L"%s\\%s", s_game_dir, rel);
        swprintf_s(b, MAX_PATH * 2, L"%s\\Cache\\%s", s_save_dir, rel);
        if (is_file(a) && is_file(b) && CopyFileW(a, b, FALSE))
            fprintf(stderr, "[MODS] %S: the game's own file is back in the cache\n", rel);
    }
    fclose(f);
    s_noted_n = kept < MODS_NOTED_MAX ? kept : MODS_NOTED_MAX;
    if (out)
        fclose(out);
    if (!out || !kept) {
        DeleteFileW(tmp);
        if (!kept)
            DeleteFileW(list);
        return;
    }
    MoveFileExW(tmp, list, MOVEFILE_REPLACE_EXISTING);
}

int xbox_mods_present(void)
{
    if (s_mods < 0) {
        WCHAR dir[MAX_PATH];
        if (!s_initialized)
            xbox_path_init(NULL, NULL);
        swprintf_s(dir, MAX_PATH, L"%s\\mods", s_save_dir);
        DWORD a = GetFileAttributesW(dir);
        if (a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY)) {
            final_path(s_game_dir, s_game_final);
            swprintf_s(dir, MAX_PATH, L"%s\\Cache", s_save_dir);
            final_path(dir, s_cache_final);
            swprintf_s(dir, MAX_PATH, L"%s\\mods", s_save_dir);
            fprintf(stderr, "[MODS] files in %S replace the game's\n", dir);
            s_mods = 1;
        } else {
            s_mods = 0;
        }
    }
    return s_mods;
}

/* host starts with base followed by a separator, slashes either way and case-blind (the dirs are kept with '/',
 * Windows hands paths back with '\\'): the rest, else NULL */
static const WCHAR* path_under(const WCHAR* host, const WCHAR* base)
{
    if (!*base)
        return NULL;
    for (; *base; host++, base++) {
        WCHAR h = *host == L'/' ? L'\\' : towlower(*host), b = *base == L'/' ? L'\\' : towlower(*base);
        if (h != b)
            return NULL;
    }
    return *host == L'\\' || *host == L'/' ? host + 1 : NULL;
}

/* The mod for a translated path in the game dir or the cache, into out: TRUE when there is one (and the game's
 * own file is there too, so that an open and a directory search agree) */
BOOL xbox_mod_path(const WCHAR* host, WCHAR* out, DWORD n)
{
    const WCHAR* rel;
    WCHAR h[MAX_PATH * 2], cache[MAX_PATH], mod[MAX_PATH * 2];
    BOOL game = FALSE;
    if (!host || !xbox_mods_present())
        return FALSE;
    plain_path(host, h, MAX_PATH * 2);
    swprintf_s(cache, MAX_PATH, L"%s\\Cache", s_save_dir);
    /* (the cache first: a save dir may be inside the game's) */
    if (!(rel = path_under(h, cache)) && !(rel = path_under(h, s_cache_final))) {
        if ((rel = path_under(h, s_game_dir)) || (rel = path_under(h, s_game_final)))
            game = TRUE;
    }
    while (rel && (*rel == L'\\' || *rel == L'/'))
        rel++;
    if (!rel || !*rel || wcslen(s_save_dir) + 6 + wcslen(rel) + 1 > n)
        return FALSE;                              /* (a path too long for the buffer: the game's own) */
    swprintf_s(mod, MAX_PATH * 2, L"%s\\mods\\%s", s_save_dir, rel);
    if (!is_file(mod) || !is_file(h))
        return FALSE;
    if (game)
        mods_note(rel);
    wcsncpy_s(out, n, mod, _TRUNCATE);   /* (out may be host: h is a copy) */
    return TRUE;
}

/* The same for a path in the game dir given as rel ("spr.afs"), UTF-8: src/quick_ui.c */
int xbox_mod_file(const char* rel, char* out, size_t n)
{
    WCHAR host[MAX_PATH], mod[MAX_PATH], wrel[MAX_PATH];
    if (!rel || !xbox_mods_present() || !MultiByteToWideChar(CP_UTF8, 0, rel, -1, wrel, MAX_PATH))
        return 0;
    if (wcslen(s_game_dir) + 1 + wcslen(wrel) + 1 > MAX_PATH)
        return 0;
    swprintf_s(host, MAX_PATH, L"%s\\%s", s_game_dir, wrel);
    if (!xbox_mod_path(host, mod, MAX_PATH))
        return 0;
    return WideCharToMultiByte(CP_UTF8, 0, mod, -1, out, (int)n, NULL, NULL) > 0;
}

/* host is a player's mod (a delete through a handle opened on one leaves it alone) */
int xbox_mod_owned(const WCHAR* host)
{
    WCHAR dir[MAX_PATH], h[MAX_PATH * 2];
    if (!host || s_mods != 1)
        return 0;
    plain_path(host, h, MAX_PATH * 2);
    swprintf_s(dir, MAX_PATH, L"%s\\mods", s_save_dir);
    return path_under(h, dir) != NULL;
}

#else /* !_WIN32  -- POSIX / Linux */
/* ======================================================================== */

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <strings.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

static char s_game_dir[MAX_PATH];
static char s_save_dir[MAX_PATH];
static BOOL s_initialized = FALSE;
static void mods_restore_cache(void);

/* Strip a single trailing '/' (but never the root '/'). */
static void strip_trailing_slash(char* s)
{
    size_t len = strlen(s);
    if (len > 1 && s[len - 1] == '/')
        s[len - 1] = '\0';
}

/* The raw disk device, \\Device\\Harddisk0\\Partition0, backed by an image
 * holding the retail partition table at sector 4 -- see the Windows half
 * above for why it has to exist. Same geometry, same bytes. */
#define XBOX_DISK_IMAGE_NAME_A "Partition0.img"

static void xbox_write_partition_table_posix(const char *path)
{
    static const struct { const char *name; uint32_t start, size; } parts[] = {
        { "XBOX_PART_X",  0x00000400, 0x00177000 },
        { "XBOX_PART_Y",  0x00177400, 0x00177000 },
        { "XBOX_PART_Z",  0x002EE400, 0x00177000 },
        { "XBOX_PART_C",  0x00465400, 0x000FA000 },
        { "XBOX_PART_E",  0x0055F400, 0x00465400 },
    };
    unsigned char sector[512];
    size_t i;
    int fd = open(path, O_WRONLY | O_CREAT, 0644);

    if (fd < 0)
        return;
    memset(sector, 0, sizeof(sector));
    memcpy(sector, "****PARTINFO****", 16);
    for (i = 0; i < sizeof(parts) / sizeof(parts[0]); i++) {
        unsigned char *e = sector + 48 + i * 32;
        size_t n = strlen(parts[i].name);
        uint32_t v[4] = { 0x80000000u, parts[i].start, parts[i].size, 0 };
        memset(e, ' ', 16);
        memcpy(e, parts[i].name, n < 16 ? n : 16);
        memcpy(e + 16, v, sizeof(v));
    }
    if (pwrite(fd, sector, sizeof(sector), 0x800) != (ssize_t)sizeof(sector))
        xbox_log(XBOX_LOG_WARN, XBOX_LOG_PATH, "partition table %s: %s",
                 path, strerror(errno));
    close(fd);
}

/* "\\Device\\Harddisk0\\PartitionN" with nothing below it is the device
 * itself, served from PartitionN.img; anything under it is a file path. */
static BOOL xbox_partition_device_path_posix(const char *xbox_path, char *out,
                                             DWORD n)
{
    static const char *prefix = "\\Device\\Harddisk0\\Partition";
    int len = match_prefix(xbox_path, prefix);
    const char *rest;
    int digit;

    if (!len)
        return FALSE;
    digit = xbox_path[len];
    if (digit < '0' || digit > '9')
        return FALSE;
    rest = xbox_path + len + 1;
    /* A trailing separator is the filesystem root (see the Windows half). */
    if ((*rest == '\\' || *rest == '/') && digit == '0')
        rest++;
    if (*rest != '\0')
        return FALSE;
    snprintf(out, n, "%s/Partition%c.img", s_save_dir, digit);
    return TRUE;
}

/* Recursively create a directory and all missing parents. */
static void mkdir_p(const char* path)
{
    char tmp[MAX_PATH];
    size_t len = strlen(path);
    if (len == 0 || len >= sizeof(tmp))
        return;
    memcpy(tmp, path, len + 1);

    for (char* p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
                xbox_log(XBOX_LOG_WARN, XBOX_LOG_PATH, "mkdir %s: %s", tmp, strerror(errno));
            *p = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
        xbox_log(XBOX_LOG_WARN, XBOX_LOG_PATH, "mkdir %s: %s", tmp, strerror(errno));
}

void xbox_path_init(const char* game_dir, const char* save_dir)
{
    if (game_dir) {
        snprintf(s_game_dir, sizeof(s_game_dir), "%s", game_dir);
    } else {
        /* Neutral default: CWD/"game", the layout game projects use. */
        char cwd[MAX_PATH];
        if (!getcwd(cwd, sizeof(cwd)))
            snprintf(cwd, sizeof(cwd), ".");
        snprintf(s_game_dir, sizeof(s_game_dir), "%s/game", cwd);
    }

    if (save_dir) {
        snprintf(s_save_dir, sizeof(s_save_dir), "%s", save_dir);
    } else {
        /* XDG base-directory spec: $XDG_DATA_HOME or ~/.local/share */
        const char* xdg = getenv("XDG_DATA_HOME");
        if (xdg && xdg[0]) {
            snprintf(s_save_dir, sizeof(s_save_dir), "%s/xboxrecomp", xdg);
        } else {
            const char* home = getenv("HOME");
            snprintf(s_save_dir, sizeof(s_save_dir), "%s/.local/share/xboxrecomp",
                     (home && home[0]) ? home : ".");
        }
    }

    strip_trailing_slash(s_game_dir);
    strip_trailing_slash(s_save_dir);

    /* Save-side directories and the partition device images, as on Windows:
     * a title opening a save file with a create disposition fails if the
     * parent is missing, and XAPI's utility-drive mount reads the raw disk. */
    {
        static const char *subs[] = { "TitleData", "UserData", "Cache",
                                      "SystemData" };
        static const unsigned long long part_sectors[6] = {
            0, 0x00465400ull, 0x000FA000ull,
            0x00177000ull, 0x00177000ull, 0x00177000ull,
        };
        char path[MAX_PATH];

        mkdir_p(s_save_dir);
        for (int i = 0; i < (int)(sizeof(subs) / sizeof(subs[0])); i++) {
            snprintf(path, sizeof(path), "%s/%s", s_save_dir, subs[i]);
            mkdir_p(path);
        }
        snprintf(path, sizeof(path), "%s/%s", s_save_dir, XBOX_DISK_IMAGE_NAME_A);
        xbox_write_partition_table_posix(path);
        /* ftruncate leaves the images sparse: 750 MB cache partitions cost
         * nothing until written. Only ever grows them, so a formatted
         * partition from an earlier run survives. */
        for (int p = 1; p <= 5; p++) {
            struct stat st;
            int fd;
            off_t want = (off_t)(part_sectors[p] * 512ull);

            snprintf(path, sizeof(path), "%s/Partition%d.img", s_save_dir, p);
            fd = open(path, O_RDWR | O_CREAT, 0644);
            if (fd < 0)
                continue;
            if (fstat(fd, &st) == 0 && st.st_size < want)
                if (ftruncate(fd, want) != 0)
                    xbox_log(XBOX_LOG_WARN, XBOX_LOG_PATH, "size %s: %s",
                             path, strerror(errno));
            close(fd);
        }
    }

    s_initialized = TRUE;
    xbox_log(XBOX_LOG_INFO, XBOX_LOG_PATH, "Path init: game=%s, save=%s",
             s_game_dir, s_save_dir);
    mods_restore_cache();
}

/* The Xbox's file names ignore case, a POSIX file system's do not: a title asking for "media\font.ttf"
 * on a disc whose folder is "Media" (Ninja Gaiden Black's arcade program, #18). Each component past
 * base_len that does not exist as spelled is replaced by the entry of its folder that matches it
 * ignoring case; a path with no such entry (a file about to be created) is left as it was. */
static void fix_case(char *path, size_t base_len)
{
    struct stat st;
    if (stat(path, &st) == 0)
        return;
    char *c = path + base_len;
    while (*c == '/')
        c++;
    while (*c) {
        char *end = strchr(c, '/');
        char saved = end ? *end : 0;
        if (end)
            *end = 0;
        if (stat(path, &st) != 0) {
            char *slash = c - 1;            /* the folder is path up to it */
            *slash = 0;
            DIR *d = opendir(*path ? path : "/");
            *slash = '/';
            struct dirent *e;
            int found = 0;
            while (d && (e = readdir(d))) {
                if (!strcasecmp(e->d_name, c) && strlen(e->d_name) == strlen(c)) {
                    memcpy(c, e->d_name, strlen(c));
                    found = 1;
                    break;
                }
            }
            if (d)
                closedir(d);
            if (!found) {
                if (end)
                    *end = saved;
                return;
            }
        }
        if (!end)
            return;
        *end = saved;
        c = end + 1;
    }
}

BOOL xbox_translate_path(const char* xbox_path, xbox_host_char* host_path_buf, DWORD buf_size)
{
    const char* remainder = NULL;
    const char* base_dir  = NULL;
    const char* sub_dir   = NULL;
    int         skip;

    if (!xbox_path || !host_path_buf || buf_size == 0)
        return FALSE;

    if (!s_initialized)
        xbox_path_init(NULL, NULL);

    {
        char linked[512];
        if (resolve_symlink(xbox_path, linked, sizeof(linked)))
            return xbox_translate_path(linked, host_path_buf, buf_size);
    }

    if (xbox_partition_device_path_posix(xbox_path, host_path_buf, buf_size)) {
        fprintf(stderr, "  [PATH] %s -> partition image\n", xbox_path);
        return TRUE;
    }

    for (int i = 0; i < PATH_RULE_COUNT; i++) {
        skip = match_prefix(xbox_path, s_rules[i].prefix);
        if (skip) {
            remainder = xbox_path + skip;
            base_dir  = s_rules[i].to_save ? s_save_dir : s_game_dir;
            sub_dir   = s_rules[i].sub_posix;
            goto translate;
        }
    }

    /* Unrecognized path: pass through, just normalize separators. */
    xbox_log(XBOX_LOG_WARN, XBOX_LOG_PATH, "Unrecognized Xbox path: %s", xbox_path);
    snprintf(host_path_buf, buf_size, "%s", xbox_path);
    for (char* p = host_path_buf; *p; p++)
        if (*p == '\\') *p = '/';
    return TRUE;

translate:
    /* The same line the Windows half prints: NtCreateFile's [FILE] status
     * line reads as the answer to it. */
    fprintf(stderr, "  [PATH] %s\n", xbox_path);
    {
        char remainder_posix[MAX_PATH];
        snprintf(remainder_posix, sizeof(remainder_posix), "%s", remainder);

        /* Xbox paths use backslashes -> POSIX slashes. */
        for (char* p = remainder_posix; *p; p++)
            if (*p == '\\') *p = '/';

        if (sub_dir) {
            snprintf(host_path_buf, buf_size, "%s%s/%s",
                     base_dir, sub_dir, remainder_posix);

            /* Ensure the save directory tree exists. */
            char dir_path[MAX_PATH];
            snprintf(dir_path, sizeof(dir_path), "%s%s", base_dir, sub_dir);
            mkdir_p(dir_path);
        } else {
            snprintf(host_path_buf, buf_size, "%s/%s", base_dir, remainder_posix);
        }

        {
            size_t n = strlen(host_path_buf);
            while (n > 1 && host_path_buf[n - 1] == '/')
                host_path_buf[--n] = '\0';
        }
        fix_case(host_path_buf, strlen(base_dir));

        XBOX_TRACE(XBOX_LOG_PATH, "%s -> %s", xbox_path, host_path_buf);
        return TRUE;
    }
}


/* ---- mods (#179): as the Windows half's ---- */
static int s_mods = -1;
#define MODS_NOTED_MAX 1024
static char s_noted[MODS_NOTED_MAX][MAX_PATH];
static int s_noted_n;
static pthread_mutex_t s_noted_lock = PTHREAD_MUTEX_INITIALIZER;

static int is_file(const char* p)
{
    struct stat st;
    return stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

static void mods_note(const char* rel)
{
    char list[MAX_PATH];
    FILE* f;
    pthread_mutex_lock(&s_noted_lock);
    for (int i = 0; i < s_noted_n; i++)
        if (!strcasecmp(s_noted[i], rel)) {
            pthread_mutex_unlock(&s_noted_lock);
            return;
        }
    if (s_noted_n >= MODS_NOTED_MAX) {            /* (full: its lines would only repeat) */
        pthread_mutex_unlock(&s_noted_lock);
        return;
    }
    snprintf(s_noted[s_noted_n++], MAX_PATH, "%s", rel);
    snprintf(list, sizeof list, "%s/mods-cache.txt", s_save_dir);
    if ((f = fopen(list, "ab"))) {
        fprintf(f, "%s\n", rel);
        fclose(f);
    }
    pthread_mutex_unlock(&s_noted_lock);
}

static int copy_file(const char* from, const char* to)
{
    char tmp[MAX_PATH * 2 + 8], buf[65536];
    ssize_t r;
    int in, out, ok = 1;
    snprintf(tmp, sizeof tmp, "%s.ngbtmp", to);
    if ((in = open(from, O_RDONLY)) < 0)
        return 0;
    if ((out = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        close(in);
        return 0;
    }
    while ((r = read(in, buf, sizeof buf)) > 0)
        if (write(out, buf, (size_t)r) != r) { ok = 0; break; }
    if (r < 0) ok = 0;
    close(in);
    if (close(out) != 0) ok = 0;
    if (ok && rename(tmp, to) == 0)
        return 1;
    unlink(tmp);
    return 0;
}

static void mods_restore_cache(void)
{
    char list[MAX_PATH], tmp[MAX_PATH + 8], line[MAX_PATH], a[MAX_PATH * 2], b[MAX_PATH * 2];
    FILE *f, *out;
    int kept = 0;
    snprintf(list, sizeof list, "%s/mods-cache.txt", s_save_dir);
    snprintf(tmp, sizeof tmp, "%s/mods-cache.new", s_save_dir);
    if (!(f = fopen(list, "rb")))
        return;
    out = fopen(tmp, "wb");
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!line[0])
            continue;
        snprintf(a, sizeof a, "%s/mods/%s", s_save_dir, line);
        fix_case(a, strlen(s_save_dir));
        if (is_file(a)) {                          /* (still a mod: kept) */
            if (kept < MODS_NOTED_MAX)
                snprintf(s_noted[kept], MAX_PATH, "%s", line);
            kept++;
            if (out)
                fprintf(out, "%s\n", line);
            continue;
        }
        snprintf(a, sizeof a, "%s/%s", s_game_dir, line);
        fix_case(a, strlen(s_game_dir));
        snprintf(b, sizeof b, "%s/Cache/%s", s_save_dir, line);
        fix_case(b, strlen(s_save_dir));
        if (is_file(a) && is_file(b) && copy_file(a, b))
            fprintf(stderr, "[MODS] %s: the game's own file is back in the cache\n", line);
    }
    fclose(f);
    s_noted_n = kept < MODS_NOTED_MAX ? kept : MODS_NOTED_MAX;
    if (out)
        fclose(out);
    if (!out || !kept) {
        unlink(tmp);
        if (!kept)
            unlink(list);
        return;
    }
    rename(tmp, list);
}

int xbox_mods_present(void)
{
    if (s_mods < 0) {
        char dir[MAX_PATH];
        struct stat st;
        if (!s_initialized)
            xbox_path_init(NULL, NULL);
        snprintf(dir, sizeof dir, "%s/mods", s_save_dir);
        fix_case(dir, strlen(s_save_dir));
        s_mods = stat(dir, &st) == 0 && S_ISDIR(st.st_mode);
        if (s_mods)
            fprintf(stderr, "[MODS] files in %s replace the game's\n", dir);
    }
    return s_mods;
}

BOOL xbox_mod_path(const char* host, char* out, DWORD n)
{
    const char* rel = NULL;
    char cache[MAX_PATH], mod[MAX_PATH * 2];
    size_t g, c;
    int game = 0;
    if (!host || !xbox_mods_present())
        return FALSE;
    g = strlen(s_game_dir);
    snprintf(cache, sizeof cache, "%s/Cache", s_save_dir);
    c = strlen(cache);
    if (!strncmp(host, cache, c) && host[c] == '/')   /* (the cache first: a save dir may be inside the game's) */
        rel = host + c + 1;
    else if (g && !strncmp(host, s_game_dir, g) && host[g] == '/') {
        rel = host + g + 1;
        game = 1;
    }
    while (rel && *rel == '/')
        rel++;
    if (!rel || !*rel || snprintf(mod, sizeof mod, "%s/mods/%s", s_save_dir, rel) >= (int)n)
        return FALSE;
    fix_case(mod, strlen(s_save_dir));
    if (!is_file(mod) || !is_file(host))   /* (the game's own there too: an open and a search agree) */
        return FALSE;
    if (game)
        mods_note(rel);
    snprintf(out, n, "%s", mod);   /* (out may be host: rel is not used after) */
    return TRUE;
}

int xbox_mod_file(const char* rel, char* out, size_t n)
{
    char host[MAX_PATH];
    if (!rel || !xbox_mods_present() || snprintf(host, sizeof host, "%s/%s", s_game_dir, rel) >= (int)sizeof host)
        return 0;
    fix_case(host, strlen(s_game_dir));
    return xbox_mod_path(host, out, (DWORD)n);
}

int xbox_mod_owned(const char* host)
{
    char dir[MAX_PATH];
    size_t d;
    if (!host || s_mods != 1)
        return 0;
    snprintf(dir, sizeof dir, "%s/mods/", s_save_dir);
    d = strlen(dir);
    return !strncasecmp(host, dir, d);
}

#endif /* _WIN32 */
