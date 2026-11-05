/*
 * bindings.h -- the controls in play: for each action, two keys or mouse
 * buttons and a controller button (src/bindings.c). The menus keep their own
 * fixed controls; the Controls page (src/pause_tab.c) changes these.
 */
#ifndef NGB_BINDINGS_H
#define NGB_BINDINGS_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    A_ATTACK, A_SPECIAL, A_JUMP, A_PROJECTILE, A_NINPO, A_WINDRUN, A_GUARD, A_CAMERA,
    A_MAP, A_KARMA, A_PAUSE, A_BACK, A_LSTICK, A_RSTICK,
    A_MOVE_UP, A_MOVE_DOWN, A_MOVE_LEFT, A_MOVE_RIGHT,
    A_CAM_UP, A_CAM_DOWN, A_CAM_LEFT, A_CAM_RIGHT,
    A_DPAD_UP, A_DPAD_DOWN, A_DPAD_LEFT, A_DPAD_RIGHT,
    A_QUICK_WEAPONS, A_QUICK_ITEMS, A_QUICK_LEFT, A_QUICK_RIGHT,
    A_QUICK_CONFIRM, A_QUICK_CANCEL,
    A_HUD,                       /* Toggle HUD: Display & Audio > HUD (src/hud.c), nothing bound by default */
    A_COUNT
};

/* An input: 0 none; a key (its SDL scancode, 1-511); a mouse button
 * (BIND_MOUSE | 1 left, 2 middle, 3 right, 4 and 5 the side ones); a
 * controller input (BIND_PAD | one of the P_ below). */
#define BIND_MOUSE 0x1000
#define BIND_PAD   0x2000
/* Two controller buttons held together (Ninpo, Wind Run: a pair of the player's choosing), the second one's
 * P_ + 1 in bits 16-20 of a BIND_PAD input */
#define BIND_PAD2(first, second) (BIND_PAD | (first) | ((second) + 1) << 16)
#define BIND_PAD_SECOND(in) ((((in) >> 16) & 0x1F) - 1)
enum { P_A, P_B, P_X, P_Y, P_BLACK, P_WHITE, P_LT, P_RT, P_START, P_BACK, P_LS, P_RS,
       P_DU, P_DD, P_DL, P_DR, P_COUNT };

#define BIND_KEYS 2              /* keyboard and mouse slots an action has */
#define NGB_BIND_HUD_BIT 64      /* Toggle HUD in ngb_bind_quick_state's bits (the quick menu's are 1-32) */

const char *ngb_bind_name(int action);        /* "Attack" */
const char *ngb_bind_help(int action);        /* a line about it */
int ngb_bind_get(int action, int slot);       /* slots 0-1 the keys, 2 the controller */
int ngb_bind_pad_fixed(int action);           /* the sticks: not the controller's to change */
/* Sets a slot: a key or controller button taken from another action leaves it
 * (that action has none there then). Saved at once. */
void ngb_bind_set(int action, int slot, int input);
void ngb_bind_defaults(void);
/* The camera's direction in play (the right stick, and the mouse and keys for it), for view 0 the
 * third-person camera, 1 the first-person view; axis 0 X, 1 Y. The game turns its third-person camera
 * inverted on both axes and has no option for it (issue #26); the views apart: #33, #63. */
int ngb_bind_invert(int view, int axis);
void ngb_bind_set_invert(int view, int axis, int on);
/* After a load, the right stick turns the third-person camera (1) instead of looking in first person, the
 * game's own start (0; src/camera.c, #23). */
int ngb_bind_third_person_start(void);
void ngb_bind_set_third_person_start(int on);
/* Start+Back Reset (Controls): the game's own reset to the title, Start and Back held; off, the game gets no
 * Start while Back is held (#195). On by default. */
int ngb_bind_soft_reset(void);
void ngb_bind_set_soft_reset(int on);
/* Camera Sensitivity (Controls): the camera's camera speed in percent; a step either way */
int ngb_bind_mouse_sensitivity(void);
float ngb_bind_camera_boost(void);   /* the cameras' turn: Camera Sensitivity over 100% (src/camera.c) */
void ngb_bind_step_mouse_sensitivity(int dir);
void ngb_bind_load(void);
/* The input's name for the page ("Left Shift", "Mouse Left", "Y"), and a
 * short one for a keycap (1-3 characters; "" for a mouse button). */
void ngb_bind_input_name(int input, char *out, int n);
int ngb_pad_prompts_ps(void);            /* a PlayStation pad's buttons in the prompts (#113) */
int ngb_prompts_keys(void);              /* the keyboard's keys in the prompts */
int ngb_pad_prompts_nintendo(void);      /* a Nintendo pad's (#170) */
int ngb_bind_prompts(void);              /* Button Prompts: 0 Auto, 1 Xbox, 2 PlayStation, 3 Nintendo, 4 PC */
int ngb_pad_set(void);                   /* the prompts' pad: 0 Xbox, 1 PlayStation, 2 Nintendo; -1 the keys */
int ngb_pad_set_pad(void);               /* the same, the controller's whatever is in use */
const char *ngb_pad_picture_p(int set, int p);   /* the picture of P_ button p (P_COUNT, +1: the sticks) */
const char *ngb_pad_picture(int set, int xbox_button);   /* its picture for the game's button; NULL the Xbox's */
int ngb_bind_nintendo_position(void);    /* Nintendo pads' face buttons: 1 by position, 0 by label (Android) */
int ngb_bind_pad_pair(int a);            /* the action takes two controller buttons held together (Ninpo, Wind Run) */
int ngb_bind_pad_combo_slot(int a);      /* the game's two-button slot of the action: 8 X + A, 9 Y + B; -1 */
void ngb_bind_step_prompts(int dir);
const char *ngb_bind_prompts_name(int v);
const char *ngb_bind_pad_combo(int a);   /* "X + A" for Wind Run, "Y + B" for Ninpo; else NULL */
void ngb_bind_input_short(int input, char *out, int n);
/* The keyboard or mouse input shown for the pad's button b (XBOX_BUTTON_*)
 * in play's prompts; 0 none. */
int ngb_bind_prompt(int analog_button);
unsigned ngb_bind_generation(void);           /* changes when the bindings do */
/* Quick-menu actions from physical inputs, before gameplay remapping. */
struct XBOX_INPUT_STATE;
unsigned ngb_bind_quick_state(const unsigned char *keys, unsigned mouse, const struct XBOX_INPUT_STATE *pad);

/* Waiting for an input for a slot: the next key or mouse button (slots 0-1,
 * from the window's events) or controller button (slot 2, polled). */
void ngb_bind_capture_begin(int action, int slot);
void ngb_bind_capture_cancel(void);
/* Each frame while capturing: 1 still waiting, 0 done (set or cancelled). */
int ngb_bind_capture_tick(void);
int ngb_bind_capturing(void);

#ifdef __cplusplus
}
#endif
#endif
