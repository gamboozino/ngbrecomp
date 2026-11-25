/* ngb_launch.h -- one recompiled program handing over to another (src/launch.c, #18). */
#ifndef NGB_LAUNCH_H
#define NGB_LAUNCH_H

#include <stddef.h>

/* The save folder when none is given: NGB_SAVE_DIR, else the platform's (as src/main.c). */
void ngb_default_save_dir(char *out, size_t n);
/* stdout and stderr into <save dir>/<name> when not started from a terminal. */
void ngb_launch_log_to_file(const char *save_dir, const char *name);
/* At start: the folders and the window to pass on, and the runtime told how to launch an image. */
void ngb_launch_init(const char *game_dir, const char *save_dir, int width, int height, int fullscreen);
/* The window passed on by the program that launched this one (NGB_LAUNCH_WINDOW), or a default. */
int ngb_launch_window_w(void);
int ngb_launch_window_h(void);
int ngb_launch_fullscreen(void);
/* Whether this program was started by another one, and the launch data it left (NULL if none). */
const unsigned char *ngb_launch_page(void);
/* The arcade's programs: back to the game, as if left, with a notice it shows (when this one cannot run). Returns
 * only when the game could not be started. */
void ngb_launch_hand_back(const char *notice);

#endif
