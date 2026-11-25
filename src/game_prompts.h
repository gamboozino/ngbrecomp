/* game_prompts.h -- button prompts and touch controls made at run time from the game's own art (src/game_prompts.c). */
#ifndef NGB_GAME_PROMPTS_H
#define NGB_GAME_PROMPTS_H
#include <stdint.h>

typedef struct {
    int w, h;
    uint8_t *px;                  /* straight-alpha RGBA, R G B A bytes */
} NgbPic;

/* Game thread: once the game's button, d-pad and font textures are loaded, copies them (each call until then;
 * cheap once done). 1 once they are read. */
int ngb_gp_poll(void);
int ngb_gp_ready(void);

/* Game thread. The picture at 4 times the game's size for a prompt picture's name ("PS3_Cross", "Switch_A",
 * "360_LB", "Q_Key_Dark", "Blank_Black_Normal"...: src/bindings.c, src/pause_tab.c), made the first time; NULL
 * before the game's textures are read (the prompts are typed meanwhile) or for a name the set has no picture
 * of. Kept: not to be freed. */
const NgbPic *ngb_gp_prompt(const char *name);
/* A blank key with this legend in the game's letters (a key the set has no picture of); NULL as above. */
const NgbPic *ngb_gp_key_label(const char *label);
/* The pause menu's tab plates with Q (0) or E (1) on them: 128 x 128, the plate's 32 x 32 at 4x. */
const NgbPic *ngb_gp_tab_key(int e);

/* Any thread. The touch controls' pieces ("button_a", "button_a_lit", "pad", "dpad_east_lit"...), made on a
 * thread of their own once the game's textures are read (ngb_gp_touch_start), or as an earlier start saved
 * them (ngb_gp_touch_saved); NULL until then. */
const NgbPic *ngb_gp_touch(const char *piece);
void ngb_gp_touch_start(void);
/* At start: the pieces saved in <save_dir>/touch-art read (Android; NGB_TOUCH_ART=1 elsewhere), and the folder
 * the ones made are saved in when there are none, or of another version. */
void ngb_gp_touch_saved(const char *save_dir);

#endif
