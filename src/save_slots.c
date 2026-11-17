/*
 * save_slots.c -- one save a slot number, before the game starts.
 *
 * The game lists its saves by the number each one's name starts with ("2. CHAPTER 7 004:51" is No. 2, 1-30) and
 * shows one save a number: of two with the same number only one is listed, which one depends on the order the
 * folder lists them, and saving over it replaces only that one. Two come from saves copied in by hand and, on Linux
 * before 0.9.10, from saving over a slot, which left the old save beside the new one: the save folder is named by
 * a hash of the whole name (XAPI's, sub_00328CCB), so a new name made a new folder. Here, as the game starts, of
 * each number's saves the newest (save000.dat's time) stays and the others go to <save dir>/saves-not-shown as they
 * are, where a player can bring one back by hand (in place of the one that stayed). Nothing is deleted or changed;
 * the title's save folder is copied to <save dir>/save-backup/<date and time> before the first move.
 * NGB_SAVE_SLOTS=0 leaves the saves as they are.
 */
#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#if defined(_WIN32)
#include <sys/utime.h>
#else
#include <utime.h>
#endif

#include "port.h"

#define TITLE_ID "5443000d"
#define SLOTS 30
#define MAX_SAVES 512

typedef struct {
    char folder[64];
    int number;    /* 1..30, 0 none the game counts */
    time_t when;   /* save000.dat's time: the newest of a number stays */
} Save;

static int same_ci(const char *a, const char *b)   /* the kernel's paths are blind to case (kernel_path.c) */
{
    for (; *a && *b; a++, b++) {
        char x = *a >= 'A' && *a <= 'Z' ? (char)(*a + 32) : *a, y = *b >= 'A' && *b <= 'Z' ? (char)(*b + 32) : *b;
        if (x != y)
            return 0;
    }
    return !*a && !*b;
}

static int is_dir(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int is_file(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static int exists(const char *path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static uint8_t *read_all(const char *path, size_t *n)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    uint8_t *buf = NULL;
    size_t cap = 0, len = 0, got;
    do {
        if (len == cap) {
            uint8_t *more = realloc(buf, cap ? cap * 2 : 4096);
            if (!more) { free(buf); fclose(f); return NULL; }
            buf = more;
            cap = cap ? cap * 2 : 4096;
        }
        got = fread(buf + len, 1, cap - len, f);
        len += got;
    } while (got);
    if (ferror(f)) {   /* a read that failed is no end of the file */
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *n = len;
    return buf;
}

/* A save's slot number: its SaveMeta.xbx's Name= (UTF-16 after a byte-order mark), read as the game's wcstol does
 * (spaces, then digits); 0 when it has none or it is past 30 */
static int read_save(const char *root, const char *folder, Save *s)
{
    char path[4096];
    size_t n = 0;
    snprintf(path, sizeof path, "%s/%s/SaveMeta.xbx", root, folder);
    uint8_t *b = read_all(path, &n);
    if (!b)
        return 0;
    const uint16_t *u = (const uint16_t *)b;
    size_t units = n / 2, at = units;
    for (size_t i = 0; i + 5 <= units; i++)
        if ((!i || u[i - 1] == '\n' || u[i - 1] == 0xFEFF) && u[i] == 'N' && u[i + 1] == 'a' && u[i + 2] == 'm' &&
            u[i + 3] == 'e' && u[i + 4] == '=') {
            at = i + 5;
            break;
        }
    memset(s, 0, sizeof *s);
    snprintf(s->folder, sizeof s->folder, "%s", folder);
    int v = 0, digits = 0;
    while (at < units && u[at] == ' ')
        at++;
    while (at < units && u[at] >= '0' && u[at] <= '9' && v < 1000)
        v = v * 10 + (u[at++] - '0'), digits++;
    s->number = digits && v >= 1 && v <= SLOTS ? v : 0;
    free(b);
    struct stat st;
    snprintf(path, sizeof path, "%s/%s/save000.dat", root, folder);
    s->when = stat(path, &st) == 0 ? st.st_mtime : 0;
    return 1;
}

static int copy_file(const char *from, const char *to)
{
    size_t n = 0;
    uint8_t *b = read_all(from, &n);
    if (!b)
        return 0;
    FILE *f = fopen(to, "wb");
    int ok = f && fwrite(b, 1, n, f) == n;
    if (f && fclose(f) != 0)
        ok = 0;
    free(b);
    struct stat st;
    if (ok && stat(from, &st) == 0) {   /* its time kept: which save is newest decides which one stays */
        struct utimbuf t = { st.st_atime, st.st_mtime };
        utime(to, &t);
    }
    return ok;
}

/* A backup folder this start made, taken away again (one that failed or that nothing came of): its files, the
 * saves' folders in it and itself, nothing else */
static void remove_backup(const char *where)
{
    DIR *d = opendir(where);
    struct dirent *e;
    while (d && (e = readdir(d))) {
        char p[4096];
        if (e->d_name[0] == '.')
            continue;
        snprintf(p, sizeof p, "%s/%s", where, e->d_name);
        if (is_dir(p)) {
            DIR *s = opendir(p);
            struct dirent *f;
            while (s && (f = readdir(s))) {
                char q[4096];
                if (f->d_name[0] == '.')
                    continue;
                snprintf(q, sizeof q, "%s/%s", p, f->d_name);
                remove(q);
            }
            if (s)
                closedir(s);
            rmdir(p);
        } else {
            remove(p);
        }
    }
    if (d)
        closedir(d);
    rmdir(where);
}

/* The title's save folder as it is, files and the saves' folders, into <save dir>/save-backup/<date and time>;
 * a backup that fails is taken away again */
static int backup(const char *save_dir, const char *root, char *where, size_t wn)
{
    char dir[4096], from[4096], to[4096];
    time_t now = time(NULL);
    struct tm tm;
#if defined(_WIN32)
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    snprintf(dir, sizeof dir, "%s/save-backup", save_dir);
    ngb_mkdir(dir);
    for (int k = 1;; k++) {   /* a new folder, never one made before (two starts in a second) */
        snprintf(where, wn, "%s/save-backup/%04d-%02d-%02d_%02d%02d%02d%s%.0d", save_dir, tm.tm_year + 1900,
                 tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, k > 1 ? "-" : "", k > 1 ? k : 0);
        if (!exists(where) && ngb_mkdir(where) == 0)
            break;
        if (k == 99)
            return 0;
    }
    DIR *d = opendir(root);
    if (!d) {
        remove_backup(where);
        return 0;
    }
    struct dirent *e;
    int ok = 1;
    while (ok && (e = readdir(d))) {
        if (e->d_name[0] == '.')
            continue;
        snprintf(from, sizeof from, "%s/%s", root, e->d_name);
        snprintf(to, sizeof to, "%s/%s", where, e->d_name);
        if (!is_dir(from)) {
            ok = is_file(from) && copy_file(from, to);
            continue;
        }
        if (ngb_mkdir(to) != 0) {
            ok = 0;
            break;
        }
        DIR *s = opendir(from);
        struct dirent *f;
        if (!s)
            ok = 0;   /* a save not copied: no backup */
        while (ok && s && (f = readdir(s))) {
            char a[4096], b[4096];
            if (f->d_name[0] == '.')
                continue;
            snprintf(a, sizeof a, "%s/%s", from, f->d_name);
            snprintf(b, sizeof b, "%s/%s", to, f->d_name);
            ok = is_file(a) && copy_file(a, b);   /* (a save holds files only: anything else, no backup) */
        }
        if (s)
            closedir(s);
    }
    closedir(d);
    if (!ok)
        remove_backup(where);
    return ok;
}

static int newer(const void *a, const void *b)
{
    const Save *x = a, *y = b;
    return x->when < y->when ? 1 : x->when > y->when ? -1 : strcmp(x->folder, y->folder);
}

void ngb_save_slots_fix(const char *save_dir)
{
    const char *e = getenv("NGB_SAVE_SLOTS");
    if ((e && *e == '0') || !save_dir || !*save_dir)
        return;
    /* the title's folder under UserData, whatever its case (the kernel finds it so: kernel_path.c) */
    char users[4096], root[4096] = "", aside[4096];
    snprintf(users, sizeof users, "%s/UserData", save_dir);
    snprintf(aside, sizeof aside, "%s/saves-not-shown", save_dir);
    DIR *d = opendir(users);
    if (!d)
        return;
    struct dirent *de;
    while ((de = readdir(d)))
        if (same_ci(de->d_name, TITLE_ID)) {
            snprintf(root, sizeof root, "%s/%s", users, de->d_name);
            break;
        }
    closedir(d);
    if (!*root || !(d = opendir(root)))
        return;
    static Save saves[MAX_SAVES];
    int n = 0;
    while ((de = readdir(d)) && n < MAX_SAVES) {
        char path[4096];
        if (de->d_name[0] == '.' || strlen(de->d_name) >= sizeof saves[0].folder)
            continue;
        snprintf(path, sizeof path, "%s/%s", root, de->d_name);
        if (is_dir(path) && read_save(root, de->d_name, &saves[n]))
            n++;
    }
    closedir(d);
    qsort(saves, (size_t)n, sizeof saves[0], newer);   /* newest first */

    /* the older saves of a number, where one of that folder's name is not aside already */
    static int move[MAX_SAVES];
    int seen[SLOTS + 1] = {0}, moves = 0;
    for (int i = 0; i < n; i++) {
        char path[4096];
        move[i] = 0;
        if (!saves[i].number)
            continue;
        if (!seen[saves[i].number]) {
            seen[saves[i].number] = 1;
            continue;
        }
        snprintf(path, sizeof path, "%s/%s", aside, saves[i].folder);
        if (!exists(path))
            move[i] = 1, moves++;
    }
    if (!moves)
        return;

    ngb_mkdir(aside);
    if (!is_dir(aside)) {   /* nowhere to move them: no backup made for nothing, at every start */
        fprintf(stderr, "[SAVES] %d saves share a slot number with a newer one, but %s cannot be made\n", moves, aside);
        return;
    }
    char where[4096];
    if (!backup(save_dir, root, where, sizeof where)) {
        fprintf(stderr, "[SAVES] %d saves share a slot number with a newer one, but the backup failed: left as they "
                        "are\n", moves);
        return;
    }
    int moved = 0;
    for (int i = 0; i < n; i++) {
        char from[4096], to[4096];
        if (!move[i])
            continue;
        snprintf(from, sizeof from, "%s/%s", root, saves[i].folder);
        snprintf(to, sizeof to, "%s/%s", aside, saves[i].folder);
        if (!exists(to) && rename(from, to) == 0) {
            moved++;
            fprintf(stderr, "[SAVES] %s (No. %d, older than the save the game shows as No. %d) moved to %s\n",
                    saves[i].folder, saves[i].number, saves[i].number, aside);
        } else {
            fprintf(stderr, "[SAVES] %s (No. %d) could not be moved to %s\n", saves[i].folder, saves[i].number,
                    aside);
        }
    }
    if (moved)
        fprintf(stderr, "[SAVES] the title's saves as they were: %s\n", where);
    else
        remove_backup(where);   /* nothing moved: no backup left behind at every start */
}
