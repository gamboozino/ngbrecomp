#ifndef NGB_INSTALL_H
#define NGB_INSTALL_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Resolve the game directory into out (see install.c): the argument (a game
 * folder, disc image or archive -- installed on first use), NGB_GAME_DIR,
 * <save_dir>/game, or <exe_dir>game, else the setup window (ngb-setup) or a
 * file dialog. 0 on success. */
void ngb_remember_game(const char *save_dir, const char *dir);   /* the folder found next start */
int ngb_find_game(const char *arg, const char *save_dir, const char *exe_dir,
                  char *out, size_t n);

/* Install src (a disc image, an archive, a game folder or its default.xbe)
 * into installed, or use a folder in place; the game folder goes to out.
 * 0 on success, else -1 with the reason in ngb_install_error(). */
int ngb_install(const char *src, const char *installed, char *out, size_t n);
const char *ngb_install_error(void);

/* Called as files are extracted: bytes done of total (total 0 while an
 * archive is being unpacked, which has no measure). From the installing
 * thread. */
typedef void (*ngb_progress_fn)(uint64_t done, uint64_t total, void *ud);
void ngb_install_set_progress(ngb_progress_fn fn, void *ud);
/* Stop an install in progress (from another thread): it fails, "Cancelled". */
void ngb_install_cancel(void);

/* 0 when dir/default.xbe is the build this code was recompiled from, or that build's code with
 * the game's data changed (a translation, a data mod); 1 when it is not (-1: no default.xbe). */
int ngb_check_xbe(const char *dir);

/* Portable mode (src/main.c): where the program is -- beside the AppImage on Linux -- and the file there
 * that turns it on. 1 on success. */
int ngb_portable_base(char *dir, size_t n);
int ngb_portable_enable(void);

#ifdef __cplusplus
}
#endif
#endif
