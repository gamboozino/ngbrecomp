/*
 * bindings.c -- the controls in play, changeable (the Controls page of the
 * recomp's settings, src/pause_tab.c).
 *
 * Each action has two keys or mouse buttons and a controller button; they
 * are kept in controls.ini beside settings.ini. In play the keyboard and
 * mouse press the pad as their bindings say (src/keyboard.c), and the
 * controller's buttons are moved to the actions they are bound to
 * (xbox_pad_remap, called by xboxrecomp's input). The menus keep their own
 * controls -- the controller as it is, the keyboard's arrows, Enter, Esc, Q
 * and E and the mouse pointing -- so no binding can lock them up. The game's
 * own Button Settings is replaced by this (src/pc_menu.c).
 */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "bindings.h"
#include "port.h"
#include "input/xinput_xbox.h"
#include "usb/usb_gamepad.h"

int ngb_quick_controls_active(void);
int ngb_quick_frozen(void);
int ngb_menu_up(void);   /* src/menu_mouse.c */

/* What an action presses on the game's pad. */
enum { T_BTN, T_AN, T_AN2, T_AXIS, T_HOST };
typedef struct {
    const char *name, *ini, *help;
    int key0, key1, pad;
    unsigned char kind;
    unsigned short a, b;          /* T_BTN: the bit; T_AN: the index (T_AN2 two); T_AXIS: 0-3 LX LY RX RY, b 1 negative */
} Action;

#define M(b) (BIND_MOUSE | (b))
#define P(p) (BIND_PAD | (p))
static const Action s_act[A_COUNT] = {
    { "Attack", "attack", "A quick strike.", M(1), SDL_SCANCODE_J, P(P_X), T_AN, XBOX_BUTTON_X, 0 },
    { "Special Attack", "special_attack", "A strong strike.", M(3), SDL_SCANCODE_K, P(P_Y), T_AN, XBOX_BUTTON_Y, 0 },
    { "Jump", "jump", "Jump; at a wall, run along it.", SDL_SCANCODE_SPACE, 0, P(P_A), T_AN, XBOX_BUTTON_A, 0 },
    { "Projectile Weapon", "projectile", "Throws the projectile weapon.", SDL_SCANCODE_E, 0, P(P_B), T_AN, XBOX_BUTTON_B, 0 },
    { "Ninpo", "ninpo", "Casts the ninpo (Special Attack and Projectile together).", SDL_SCANCODE_F, 0, 0,
      T_AN2, XBOX_BUTTON_Y, XBOX_BUTTON_B },
    { "Wind Run", "wind_run", "Runs at the nearest enemy (Attack and Jump together).", SDL_SCANCODE_C, 0, 0,
      T_AN2, XBOX_BUTTON_X, XBOX_BUTTON_A },
    { "Guard", "guard", "Blocks; with a direction, rolls.", SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT, P(P_LT),
      T_AN, XBOX_BUTTON_LTRIGGER, 0 },
    { "Center Camera", "center_camera", "Turns the camera behind Ryu.", SDL_SCANCODE_Q, M(2), P(P_RT), T_AN,
      XBOX_BUTTON_RTRIGGER, 0 },
    { "Display Map", "map", "Shows the map.", SDL_SCANCODE_TAB, 0, P(P_BLACK), T_AN, XBOX_BUTTON_BLACK, 0 },
    { "Karma Window On/Off", "karma_window", "Shows or hides the karma window.", SDL_SCANCODE_R, 0, P(P_WHITE), T_AN,
      XBOX_BUTTON_WHITE, 0 },
    { "Pause", "pause", "The pause menu.", SDL_SCANCODE_ESCAPE, SDL_SCANCODE_RETURN, P(P_START), T_BTN,
      XBOX_GAMEPAD_START, 0 },
    { "Back", "back", "The controller's Back button.", SDL_SCANCODE_BACKSPACE, 0, P(P_BACK), T_BTN,
      XBOX_GAMEPAD_BACK, 0 },
    { "First-Person View", "first_person", "Looks through Ryu's eyes (the left stick's button).", SDL_SCANCODE_Z,
      0, P(P_LS), T_BTN, XBOX_GAMEPAD_LEFT_THUMB, 0 },
    { "Right Stick Button", "right_stick", "The right stick's button.", SDL_SCANCODE_V, 0, P(P_RS), T_BTN,
      XBOX_GAMEPAD_RIGHT_THUMB, 0 },
    { "Move Forward", "move_forward", "Moves Ryu (the left stick).", SDL_SCANCODE_W, 0, 0, T_AXIS, 1, 0 },
    { "Move Back", "move_back", "Moves Ryu (the left stick).", SDL_SCANCODE_S, 0, 0, T_AXIS, 1, 1 },
    { "Move Left", "move_left", "Moves Ryu (the left stick).", SDL_SCANCODE_A, 0, 0, T_AXIS, 0, 1 },
    { "Move Right", "move_right", "Moves Ryu (the left stick).", SDL_SCANCODE_D, 0, 0, T_AXIS, 0, 0 },
    { "Camera Up", "camera_up", "Turns the camera (the right stick; the mouse too).", SDL_SCANCODE_KP_8, 0, 0,
      T_AXIS, 3, 0 },
    { "Camera Down", "camera_down", "Turns the camera (the right stick; the mouse too).", SDL_SCANCODE_KP_5, 0, 0,
      T_AXIS, 3, 1 },
    { "Camera Left", "camera_left", "Turns the camera (the right stick; the mouse too).", SDL_SCANCODE_KP_4, 0, 0,
      T_AXIS, 2, 1 },
    { "Camera Right", "camera_right", "Turns the camera (the right stick; the mouse too).", SDL_SCANCODE_KP_6, 0,
      0, T_AXIS, 2, 0 },
    { "D-Pad Up", "dpad_up", "The controller's directional pad.", SDL_SCANCODE_UP, 0, P(P_DU), T_BTN,
      XBOX_GAMEPAD_DPAD_UP, 0 },
    { "D-Pad Down", "dpad_down", "The controller's directional pad.", SDL_SCANCODE_DOWN, 0, P(P_DD), T_BTN,
      XBOX_GAMEPAD_DPAD_DOWN, 0 },
    { "D-Pad Left", "dpad_left", "The controller's directional pad.", SDL_SCANCODE_LEFT, 0, P(P_DL), T_BTN,
      XBOX_GAMEPAD_DPAD_LEFT, 0 },
    { "D-Pad Right", "dpad_right", "The controller's directional pad.", SDL_SCANCODE_RIGHT, 0, P(P_DR), T_BTN,
      XBOX_GAMEPAD_DPAD_RIGHT, 0 },
    /* Keep the four original setting names and IDs for existing profiles. */
    { "Quick Menu: Category Up", "quick_weapons", "Opens Melee Weapons; while open, selects the previous category.", SDL_SCANCODE_UP, SDL_SCANCODE_1, P(P_DU), T_HOST, 1, 0 },
    { "Quick Menu: Category Down", "quick_items", "Opens Consumables; while open, selects the next category.", SDL_SCANCODE_DOWN, SDL_SCANCODE_2, P(P_DD), T_HOST, 2, 0 },
    { "Quick Menu: Previous", "quick_left", "Opens the melee weapons on the card before; while open, selects the previous entry.", SDL_SCANCODE_LEFT, 0, P(P_DL), T_HOST, 4, 0 },
    { "Quick Menu: Next", "quick_right", "Opens the melee weapons on the next card; while open, selects the next entry.", SDL_SCANCODE_RIGHT, 0, P(P_DR), T_HOST, 8, 0 },
    { "Quick Menu: Confirm", "quick_confirm", "Equips the selected weapon or Ninpo, or uses the selected item.", SDL_SCANCODE_RETURN, 0, P(P_A), T_HOST, 16, 0 },
    { "Quick Menu: Cancel", "quick_cancel", "Closes the quick menu without applying the selection.", SDL_SCANCODE_ESCAPE, SDL_SCANCODE_BACKSPACE, P(P_B), T_HOST, 32, 0 },
    { "Toggle HUD", "toggle_hud", "Shows or hides the HUD (Display & Audio > HUD). Not bound unless chosen.", 0, 0, 0, T_HOST,
      NGB_BIND_HUD_BIT, 0 },
};

static int s_bind[A_COUNT][3];
static int s_loaded;
static volatile unsigned s_gen = 1;
static int s_pad_identity = 1;

static const char *const s_pad_name[P_COUNT] = {
    "A", "B", "X", "Y", "LB", "RB", "LT", "RT", "Start", "Back", "LS", "RS",
    "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
};
/* The same buttons on a PlayStation pad, shown while one is in use (#113); saved as the names above */
static const char *const s_pad_name_ps[P_COUNT] = {
    "Cross", "Circle", "Square", "Triangle", "L1", "R1", "L2", "R2", "Start", "Select", "L3", "R3",
    "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
};
/* A Nintendo pad's, by label; by position (desktop) its face buttons swap: ngb_bind_input_name */
static const char *const s_pad_name_ns[P_COUNT] = {
    "A", "B", "X", "Y", "L", "R", "ZL", "ZR", "+", "-", "L Stick", "R Stick",
    "D-Pad Up", "D-Pad Down", "D-Pad Left", "D-Pad Right",
};
static const char *const s_mouse_name[6] = { "", "Mouse Left", "Mouse Middle", "Mouse Right", "Mouse 4", "Mouse 5" };

const char *ngb_bind_name(int a) { return a >= 0 && a < A_COUNT ? s_act[a].name : ""; }
const char *ngb_bind_help(int a) { return a >= 0 && a < A_COUNT ? s_act[a].help : ""; }
int ngb_bind_pad_fixed(int a) { return a >= 0 && a < A_COUNT && s_act[a].kind == T_AXIS; }

/* What the controller always has for an action that is two buttons together on the Xbox (Ninpo: Y + B, Wind
 * Run: X + A), whatever button it is also bound to; NULL for the others (src/pause_tab.c shows it in an
 * empty controller slot, #75). */
/* ---------- the game's own Button Settings ----------
 *
 * The game gives each controller button an action in its options (Options > Controller > Button
 * Settings): ten entries at 0xDB9380 + 0xA0 -- A, B, X, Y, Black, White, LT, RT, then X + A and Y + B --
 * each an action 0-10: not used, Ninpo, Wind Run, Jump, Projectile Weapon, Attack, Special Attack,
 * Guard, Center Camera, Display Map, Karma Window (the menu's text order, sub_0021D970). Ours bind
 * a key or a controller button to an action; the Xbox button that action takes is the one the game's
 * table gives it now, so a change there does not move what our bindings do. */
#define GAME_BUTTONS 0xDB9420u
static const unsigned char s_game_default[10] = { 3, 4, 5, 6, 9, 10, 7, 8, 2, 1 };
extern ptrdiff_t g_xbox_mem_offset;

static void game_buttons(unsigned char out[10])
{
    memcpy(out, s_game_default, 10);
    if (!g_xbox_mem_offset)
        return;
    const uint32_t *t = (const uint32_t *)((uintptr_t)GAME_BUTTONS + g_xbox_mem_offset);
    unsigned char v[10];
    int any = 0;
    for (int i = 0; i < 10; i++) {
        if (t[i] > 10)
            return;                                  /* not the options (yet) */
        v[i] = (unsigned char)t[i];
        any |= v[i];
    }
    if (any)
        memcpy(out, v, 10);
}

/* The game's action for one of ours (by the button it has by default), or -1. */
static int game_action(const Action *t)
{
    if (t->kind == T_AN2)
        return t->a == XBOX_BUTTON_Y ? 1 : 2;        /* Ninpo (Y + B), Wind Run (X + A) */
    if (t->kind == T_AN && t->a < 8)
        return s_game_default[t->a];
    return -1;
}

/* Where the game has that action now: 0-7 a button (the analog index), 8 X + A, 9 Y + B; -1 nowhere. */
static int game_slot(const Action *t, const unsigned char tab[10])
{
    int id = game_action(t);
    if (id < 0)
        return -1;
    for (int i = 0; i < 10; i++)
        if (tab[i] == id)
            return i;
    return -1;
}

/* The controller button (0-7: A, B, X, Y, Black, White, LT, RT) the game's Button Settings give an action, -1
 * none (or only a pair) */
int ngb_game_button_for(int action)
{
    unsigned char b[10];
    game_buttons(b);
    for (int i = 0; i < 8; i++)
        if (b[i] == action)
            return i;
    return -1;
}

static int game_buttons_default(void)
{
    unsigned char tab[10];
    game_buttons(tab);
    return !memcmp(tab, s_game_default, 10);
}

/* What the controller always has for an action that is two buttons together in the game's Button Settings
 * (by default Ninpo: Y + B, Wind Run: X + A), whatever button it is also bound to; NULL for the others
 * (src/pause_tab.c shows it in an empty controller slot, #75). */
const char *ngb_bind_pad_combo(int a)
{
    if (a < 0 || a >= A_COUNT)
        return NULL;
    unsigned char tab[10];
    game_buttons(tab);
    int slot = game_slot(&s_act[a], tab);
    if (ngb_pad_prompts_ps())
        return slot == 8 ? "Square + Cross" : slot == 9 ? "Triangle + Circle" : NULL;
    if (ngb_pad_prompts_nintendo() && ngb_bind_nintendo_position())
        return slot == 8 ? "Y + B" : slot == 9 ? "X + A" : NULL;
    return slot == 8 ? "X + A" : slot == 9 ? "Y + B" : NULL;
}

/* The game's slot of that action's two buttons: 8 X + A (Wind Run's by default), 9 Y + B; -1 none */
int ngb_bind_pad_combo_slot(int a)
{
    if (a < 0 || a >= A_COUNT)
        return -1;
    unsigned char tab[10];
    game_buttons(tab);
    int slot = game_slot(&s_act[a], tab);
    return slot == 8 || slot == 9 ? slot : -1;
}
unsigned ngb_bind_generation(void) { return s_gen; }

static void identity_check(void)
{
    s_pad_identity = 1;
    for (int a = 0; a < A_COUNT; a++)
        if (s_bind[a][2] != s_act[a].pad)
            s_pad_identity = 0;
}

/* The right stick's X and Y, inverted in play, for the third-person camera ([0]) and the first-person view
 * ([1]; its Y is the game's own option, src/camera.c); and the view the stick starts in after a load
 * (controls.ini; #33 #63 #23) */
static int s_invert[2][2];
static int s_third_start;
static int s_soft_reset = 1;   /* Start+Back Reset: the game's own reset to the title (#195) */
static int s_mouse_pct = 100;   /* Camera Sensitivity, percent: the right stick, mouse and keys (#67) */
static int s_prompts;           /* Button Prompts: 0 Auto (the device in use), 1 Xbox, 2 PlayStation, 3 Nintendo, 4 PC */
#define PROMPTS_N 5
static const char *const s_prompts_ini[PROMPTS_N] = { "auto", "xbox", "playstation", "nintendo", "pc" };

static void soft_reset_apply(void);

void ngb_bind_defaults(void)
{
    memset(s_invert, 0, sizeof s_invert);
    s_third_start = 1;   /* (on from 0.9.45) */
    s_soft_reset = 1;
    soft_reset_apply();
    s_mouse_pct = 100;
    s_prompts = 0;
    for (int a = 0; a < A_COUNT; a++) {
        s_bind[a][0] = s_act[a].key0;
        s_bind[a][1] = s_act[a].key1;
        s_bind[a][2] = s_act[a].pad;
    }
    identity_check();
    s_gen++;
}

/* ---------- names ---------- */

static void ensure(void);
static void save(void);

/* Button Prompts (the Controls page): which buttons the game's prompts show. Auto follows the device in use --
 * the keyboard's keys while it is (xboxrecomp's xbox_InputKeyboardActive), a PlayStation pad's buttons while
 * one is (xbox_InputPadPlayStation, #113), else the Xbox's; the others keep one set whatever is in use.
 * NGB_PAD_PROMPTS (tests) overrides it: ps the PlayStation's, anything else the Xbox's. */
int ngb_bind_prompts(void)
{
    ensure();
    return s_prompts;
}

void ngb_bind_step_prompts(int dir)
{
    ensure();
    s_prompts = (s_prompts + (dir < 0 ? PROMPTS_N - 1 : 1)) % PROMPTS_N;
    s_gen++;
    save();
}

const char *ngb_bind_prompts_name(int v)
{
    static const char *const name[PROMPTS_N] = { "Auto", "Xbox", "PlayStation", "Nintendo", "PC" };
    return v >= 0 && v < PROMPTS_N ? name[v] : "";
}

static int prompts_forced(void)   /* NGB_PAD_PROMPTS: 2 PlayStation, 3 Nintendo, 1 Xbox, 0 not set */
{
    static int forced = -1;
    if (forced < 0) {
        const char *e = getenv("NGB_PAD_PROMPTS");
        forced = !e ? 0 : !SDL_strcasecmp(e, "ps") ? 2 : !SDL_strcasecmp(e, "nintendo") ? 3 : 1;
    }
    return forced;
}

/* The keys and mouse buttons in the prompts (src/pause_tab.c) */
int ngb_prompts_keys(void)
{
    int p = ngb_bind_prompts();
    if (prompts_forced())
        return 0;
    return p == 4 || (p == 0 && xbox_InputKeyboardActive());
}

/* A PlayStation pad's buttons in the prompts and the button names */
int ngb_pad_prompts_ps(void)
{
    int p = ngb_bind_prompts();
    if (prompts_forced())
        return prompts_forced() == 2;
    return p == 2 || (p == 0 && xbox_InputPadPlayStation());
}

/* A Nintendo pad's (a Switch Pro Controller's, Joy-Cons') */
int ngb_pad_prompts_nintendo(void)
{
    int p = ngb_bind_prompts();
    if (prompts_forced())
        return prompts_forced() == 3;
    return p == 3 || (p == 0 && xbox_InputPadNintendo());
}

/* A Nintendo pad's face buttons by position: the game's A is the button where the Xbox pad has its A (marked
 * B), so two-button moves such as Ninpo are where the game's own layout puts them (#170). SDL makes the change
 * (its hint), the prompts follow. Android's own controller mappings go by label only. */
int ngb_bind_nintendo_position(void)
{
#ifdef __ANDROID__
    return 0;
#else
    return 1;
#endif
}

/* The picture (assets/prompts) of the game's button b -- XBOX_BUTTON_A..RTRIGGER, Black for the left bumper,
 * White for the right -- on the pad the prompts show: set 1 PlayStation, 2 Nintendo (with its layout); NULL for
 * the Xbox's own (the game's). */
const char *ngb_pad_picture(int set, int b)
{
    static const char *const ps[8] = { "PS3_Cross", "PS3_Circle", "PS3_Square", "PS3_Triangle",
                                       "PS3_L1", "PS3_R1", "PS3_L2", "PS3_R2" };
    static const char *const ns[8] = { "Switch_A", "Switch_B", "Switch_X", "Switch_Y",
                                       "Switch_LB", "Switch_RB", "Switch_LT", "Switch_RT" };
    int i = b == XBOX_BUTTON_A ? 0 : b == XBOX_BUTTON_B ? 1 : b == XBOX_BUTTON_X ? 2 : b == XBOX_BUTTON_Y ? 3
          : b == XBOX_BUTTON_BLACK ? 4 : b == XBOX_BUTTON_WHITE ? 5 : b == XBOX_BUTTON_LTRIGGER ? 6
          : b == XBOX_BUTTON_RTRIGGER ? 7 : -1;
    if (i < 0 || set < 1 || set > 2)
        return NULL;
    if (set == 1)
        return ps[i];
    if (ngb_bind_nintendo_position() && i < 4)
        i ^= 1;                                    /* the face buttons by place: A <-> B, X <-> Y */
    return ns[i];
}

/* The picture of the controller's button p (P_A...) on set 0 Xbox, 1 PlayStation, 2 Nintendo (its layout as in
 * ngb_bind_input_name); P_COUNT and P_COUNT + 1 the left and right sticks (the moving and camera rows) */
const char *ngb_pad_picture_p(int set, int p)
{
    static const char *const pic[3][P_COUNT + 2] = {
        { "360_A", "360_B", "360_X", "360_Y", "360_LB", "360_RB", "360_LT", "360_RT", "360_Start", "360_Back",
          "360_Left_Stick_Click", "360_Right_Stick_Click", "360_Dpad_Up", "360_Dpad_Down", "360_Dpad_Left",
          "360_Dpad_Right", "360_Left_Stick", "360_Right_Stick" },
        { "PS3_Cross", "PS3_Circle", "PS3_Square", "PS3_Triangle", "PS3_L1", "PS3_R1", "PS3_L2", "PS3_R2",
          "PS3_Start", "PS3_Select", "PS3_Left_Stick_Click", "PS3_Right_Stick_Click", "PS3_Dpad_Up",
          "PS3_Dpad_Down", "PS3_Dpad_Left", "PS3_Dpad_Right", "PS3_Left_Stick", "PS3_Right_Stick" },
        { "Switch_A", "Switch_B", "Switch_X", "Switch_Y", "Switch_LB", "Switch_RB", "Switch_LT", "Switch_RT",
          "Switch_Plus", "Switch_Minus", "Switch_Left_Stick_Click", "Switch_Right_Stick_Click", "Switch_Dpad_Up",
          "Switch_Dpad_Down", "Switch_Dpad_Left", "Switch_Dpad_Right", "Switch_Left_Stick", "Switch_Right_Stick" },
    };
    if (set < 0 || set > 2 || p < 0 || p >= P_COUNT + 2)
        return NULL;
    if (set == 2 && p < 4 && ngb_bind_nintendo_position())
        p ^= 1;                                    /* (P_A..P_Y: A B X Y, by place) */
    return pic[set][p];
}

/* The controller's set whatever device is in use (the Controls page's controller column): 0 Xbox, 1 PS, 2 Nintendo */
int ngb_pad_set_pad(void)
{
    return ngb_pad_prompts_ps() ? 1 : ngb_pad_prompts_nintendo() ? 2 : 0;
}

/* The pad set the prompts show now: 0 the Xbox's (the game's own), 1 PlayStation, 2 Nintendo; -1 keys */
int ngb_pad_set(void)
{
    return ngb_prompts_keys() ? -1 : ngb_pad_prompts_ps() ? 1 : ngb_pad_prompts_nintendo() ? 2 : 0;
}

void ngb_bind_input_name(int in, char *out, int n)
{
    if (!in)
        snprintf(out, n, "-");
    else if ((in & BIND_PAD) && BIND_PAD_SECOND(in) >= 0) {   /* two together: "LB + RB" */
        char a[32], b[32];
        ngb_bind_input_name(BIND_PAD | (in & 0xFF), a, sizeof a);
        ngb_bind_input_name(BIND_PAD | BIND_PAD_SECOND(in), b, sizeof b);
        snprintf(out, n, "%s + %s", a, b);
    } else if (in & BIND_PAD) {
        int b = in & 0xFF;
        if (b < 4 && ngb_pad_prompts_nintendo() && !ngb_pad_prompts_ps() && ngb_bind_nintendo_position())
            b ^= 1;                                    /* (P_A..P_Y: A B X Y) */
        snprintf(out, n, "%s", b < P_COUNT ? (ngb_pad_prompts_ps() ? s_pad_name_ps
                                              : ngb_pad_prompts_nintendo() ? s_pad_name_ns : s_pad_name)[b] : "?");
    }
    else if (in & BIND_MOUSE)
        snprintf(out, n, "%s", (in & 0xFF) >= 1 && (in & 0xFF) <= 5 ? s_mouse_name[in & 0xFF] : "Mouse ?");
    else {
        const char *s = SDL_GetScancodeName((SDL_Scancode)in);
        snprintf(out, n, "%s", s && *s ? s : "?");
    }
}

void ngb_bind_input_short(int in, char *out, int n)
{
    out[0] = 0;
    if (!in || (in & BIND_MOUSE) || (in & BIND_PAD))
        return;
    switch (in) {
    case SDL_SCANCODE_SPACE:     snprintf(out, n, "SPC"); return;
    case SDL_SCANCODE_TAB:       snprintf(out, n, "TAB"); return;
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: snprintf(out, n, "^"); return;
    case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL:   snprintf(out, n, "CTL"); return;
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT:     snprintf(out, n, "ALT"); return;
    case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: snprintf(out, n, "ENT"); return;
    case SDL_SCANCODE_ESCAPE:    snprintf(out, n, "ESC"); return;
    case SDL_SCANCODE_BACKSPACE: snprintf(out, n, "BS"); return;
    case SDL_SCANCODE_CAPSLOCK:  snprintf(out, n, "CAP"); return;
    case SDL_SCANCODE_UP:        snprintf(out, n, "\x01"); return;
    case SDL_SCANCODE_DOWN:      snprintf(out, n, "\x02"); return;
    case SDL_SCANCODE_LEFT:      snprintf(out, n, "\x03"); return;
    case SDL_SCANCODE_RIGHT:     snprintf(out, n, "\x04"); return;
    default: break;
    }
    if (in >= SDL_SCANCODE_KP_1 && in <= SDL_SCANCODE_KP_0) {
        snprintf(out, n, "N%c", in == SDL_SCANCODE_KP_0 ? '0' : '1' + (in - SDL_SCANCODE_KP_1));
        return;
    }
    if (in >= SDL_SCANCODE_F1 && in <= SDL_SCANCODE_F12) {
        snprintf(out, n, "F%d", in - SDL_SCANCODE_F1 + 1);
        return;
    }
    const char *s = SDL_GetScancodeName((SDL_Scancode)in);
    if (s && s[0] && !s[1]) {                      /* letters, digits, punctuation: itself */
        snprintf(out, n, "%c", s[0] >= 'a' && s[0] <= 'z' ? s[0] - 32 : s[0]);
        return;
    }
    int j = 0;                                     /* else its first letters */
    for (int i = 0; s && s[i] && j < 3 && j < n - 1; i++)
        if (s[i] != ' ')
            out[j++] = s[i] >= 'a' && s[i] <= 'z' ? s[i] - 32 : s[i];
    out[j] = 0;
}

/* A controller button by its saved name (the Xbox's), spaces around it allowed; -1 none */
static int pad_named(const char *t)
{
    char s[64];
    while (*t == ' ')
        t++;
    snprintf(s, sizeof s, "%s", t);
    for (int i = (int)strlen(s) - 1; i >= 0 && s[i] == ' '; i--)
        s[i] = 0;
    for (int i = 0; i < P_COUNT; i++)
        if (!SDL_strcasecmp(s, s_pad_name[i]))
            return i;
    return -1;
}

static int parse_input(const char *t)
{
    while (*t == ' ')
        t++;
    char s[64];
    snprintf(s, sizeof s, "%s", t);
    for (int i = (int)strlen(s) - 1; i >= 0 && s[i] == ' '; i--)
        s[i] = 0;
    if (!s[0] || !strcmp(s, "-"))
        return 0;
    for (int i = 1; i <= 5; i++)
        if (!SDL_strcasecmp(s, s_mouse_name[i]))
            return BIND_MOUSE | i;
    if (!SDL_strncasecmp(s, "Pad ", 4)) {
        char one[64], *plus;
        snprintf(one, sizeof one, "%s", s + 4);
        if ((plus = strchr(one, '+'))) {           /* "Pad LB+RB": two together */
            *plus = 0;
            int a = pad_named(one), b = pad_named(plus + 1);
            return a < 0 || b < 0 || a == b ? -1 : a < b ? BIND_PAD2(a, b) : BIND_PAD2(b, a);
        }
        int a = pad_named(one);
        return a >= 0 ? BIND_PAD | a : -1;
    }
    SDL_Scancode sc = SDL_GetScancodeFromName(s);
    return sc != SDL_SCANCODE_UNKNOWN ? (int)sc : -1;
}

/* ---------- the file ---------- */

static void file_path(char *out, size_t n, int make_dir)
{
    char dir[1024];
    ngb_config_dir(dir, sizeof dir, make_dir);
    snprintf(out, n, "%s/controls.ini", dir);
}

static void save(void)
{
    char path[1100];
    file_path(path, sizeof path, 1);
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "# Ninja Gaiden Black controls in play (the Controls page writes this file):\n"
               "# action = key or mouse button, another, Pad <button>\n");
    fprintf(f, "defaults = 1\n");   /* (the defaults it was written with: see ngb_bind_load) */
    for (int a = 0; a < A_COUNT; a++) {
        char k0[64], k1[64], p[64];
        ngb_bind_input_name(s_bind[a][0], k0, sizeof k0);
        ngb_bind_input_name(s_bind[a][1], k1, sizeof k1);
        if (s_bind[a][2] && BIND_PAD_SECOND(s_bind[a][2]) >= 0)
            snprintf(p, sizeof p, "Pad %s+%s", s_pad_name[s_bind[a][2] & 0xFF],
                     s_pad_name[BIND_PAD_SECOND(s_bind[a][2])]);
        else if (s_bind[a][2])
            snprintf(p, sizeof p, "Pad %s", s_pad_name[s_bind[a][2] & 0xFF]);
        else
            snprintf(p, sizeof p, "-");
        fprintf(f, "%s = %s, %s, %s\n", s_act[a].ini, k0, k1, p);
    }
    fprintf(f, "invert_camera_x = %d\ninvert_camera_y = %d\n", s_invert[0][0], s_invert[0][1]);
    fprintf(f, "invert_first_person_x = %d\n", s_invert[1][0]);
    fprintf(f, "third_person_on_load = %d\n", s_third_start);
    fprintf(f, "start_back_reset = %d\n", s_soft_reset);
    fprintf(f, "mouse_sensitivity = %d\n", s_mouse_pct);
    fprintf(f, "button_prompts = %s\n", s_prompts_ini[s_prompts]);
    fclose(f);
}

/* Start+Back Reset off: while Back is held the game gets no Start, so the two are never held together (another
 * program on the same combo reset the game, #195). NGB_F1_MENU takes Start under Back too (src/ingame_menu.c). */
static void soft_reset_apply(void)
{
    usb_gamepad_back_start_host = !s_soft_reset || getenv("NGB_F1_MENU") != NULL;
}

static void nintendo_apply(void)   /* the controllers start with it (xbox_InputInit) */
{
    xbox_InputNintendoLabels(!ngb_bind_nintendo_position());
    soft_reset_apply();
}

void ngb_bind_load(void)
{
    s_loaded = 1;
    ngb_bind_defaults();
    char path[1100], line[512];
    file_path(path, sizeof path, 0);
    FILE *f = fopen(path, "r");
    if (!f) {
        nintendo_apply();
        return;
    }
    int first_set = 0;   /* a file from before the views were separate: its choice holds for both */
    int defaults = 0;
    while (fgets(line, sizeof line, f)) {
        char key[64], val[400];
        if (line[0] == '#' || sscanf(line, " %63[^= ] = %399[^\r\n]", key, val) != 2)
            continue;
        if (!strcmp(key, "invert_camera_x") || !strcmp(key, "invert_camera_y")) {
            s_invert[0][key[14] == 'y'] = atoi(val) != 0;
            continue;
        }
        if (!strcmp(key, "invert_first_person_x")) {
            s_invert[1][0] = atoi(val) != 0;
            first_set = 1;
            continue;
        }
        if (!strcmp(key, "third_person_on_load")) {
            s_third_start = atoi(val) != 0;
            continue;
        }
        if (!strcmp(key, "start_back_reset")) {
            s_soft_reset = atoi(val) != 0;
            continue;
        }
        if (!strcmp(key, "defaults")) {
            defaults = atoi(val);
            continue;
        }
        if (!strcmp(key, "button_prompts")) {
            for (int i = 0; i < PROMPTS_N; i++)
                if (!SDL_strcasecmp(val, s_prompts_ini[i]))
                    s_prompts = i;
            continue;
        }
        if (!strcmp(key, "nintendo_layout"))   /* (a choice in the 0.9.56 test builds) */
            continue;
        if (!strcmp(key, "mouse_sensitivity")) {
            int v = atoi(val);
            v = (v + 12) / 25 * 25;
            s_mouse_pct = v < 25 ? 25 : v > 300 ? 300 : v;
            continue;
        }
        for (int a = 0; a < A_COUNT; a++) {
            if (strcmp(key, s_act[a].ini))
                continue;
            char *save_p = NULL, *tok = strtok_r(val, ",", &save_p);
            for (int slot = 0; slot < 3 && tok; slot++, tok = strtok_r(NULL, ",", &save_p)) {
                int in = parse_input(tok);
                if (in < 0)
                    continue;                          /* not known here: the default */
                if (slot < 2 && (in & BIND_PAD))
                    continue;
                if (slot == 2 && (in && !(in & BIND_PAD)))
                    continue;
                if (slot == 2 && s_act[a].kind == T_AXIS)
                    continue;
                if ((in & BIND_PAD) && BIND_PAD_SECOND(in) >= 0 && s_act[a].kind != T_AN2)
                    continue;                          /* (two together only for the game's two-button moves) */
                s_bind[a][slot] = in;
            }
        }
    }
    fclose(f);
    if (!first_set)
        s_invert[1][0] = s_invert[0][0];
    /* Start in Third Person is on from 0.9.45: a file from before (which always wrote it, off unless chosen)
     * gets it on once */
    if (defaults < 1)
        s_third_start = 1;
    s_invert[1][1] = 0;   /* first-person up/down: the game's own Options > 1st Person Camera Control */
    nintendo_apply();
    identity_check();
    s_gen++;
}

static void ensure(void)
{
    if (!s_loaded)
        ngb_bind_load();
}

int ngb_bind_get(int a, int slot)
{
    ensure();
    return a >= 0 && a < A_COUNT && slot >= 0 && slot < 3 ? s_bind[a][slot] : 0;
}

void ngb_bind_set(int a, int slot, int in)
{
    ensure();
    if (a < 0 || a >= A_COUNT || slot < 0 || slot > 2)
        return;
    if (slot == 2 && ngb_bind_pad_fixed(a))
        return;
    /* Pause keeps a controller button (else no pause menu, nor this page, on a controller): it can't be cleared,
     * and a button taken from it is refused when this action has none (or a pair) to give it in exchange */
    if (slot == 2 && a == A_PAUSE && !in)
        return;
    if (slot == 2 && in && a != A_PAUSE && a < A_QUICK_WEAPONS && s_bind[A_PAUSE][2] == in) {
        int old = s_bind[a][2];
        if (!old || ((old & BIND_PAD) && BIND_PAD_SECOND(old) >= 0))
            return;
    }
    if (in) {
        for (int o = 0; o < A_COUNT; o++)
            for (int s = 0; s < 3; s++) {
                if ((a >= A_QUICK_WEAPONS) != (o >= A_QUICK_WEAPONS) ||
                    s_bind[o][s] != in || (o == a && s == slot))
                    continue;
                if (slot == 2 && o == A_PAUSE) {       /* (above: this one's old button, a single one) */
                    s_bind[o][s] = s_bind[a][2];
                    continue;
                }
                s_bind[o][s] = 0;                      /* a key or a controller button: moved here, the other
                                                        * action left without it (Default Controls brings all back) */
            }
    }
    s_bind[a][slot] = in;
    if (slot < 2 && s_bind[a][0] == 0 && s_bind[a][1]) {   /* the first slot first */
        s_bind[a][0] = s_bind[a][1];
        s_bind[a][1] = 0;
    }
    identity_check();
    s_gen++;
    save();
}

int ngb_bind_prompt(int b)
{
    ensure();
    for (int a = 0; a < A_COUNT; a++)
        if (s_act[a].kind == T_AN && s_act[a].a == b)
            return s_bind[a][0] ? s_bind[a][0] : s_bind[a][1];
    return 0;
}

/* ---------- in play ---------- */

static int input_down(int in, const Uint8 *k, Uint32 mouse)
{
    if (!in || (in & BIND_PAD))
        return 0;
    if (in & BIND_MOUSE)
        return (mouse & SDL_BUTTON(in & 0xFF)) != 0;
    return in < SDL_NUM_SCANCODES && k[in];
}

/* Quick-menu bindings share physical buttons with combat without stealing
 * their saved assignments. Consume those inputs only while choosing. */
static int quick_input(int input)
{
    if (!input) return 0;
    for (int a = A_QUICK_WEAPONS; a < A_COUNT; a++)
        for (int slot = 0; slot < 3; slot++)
            if (s_bind[a][slot] == input) return 1;
    return 0;
}

static void press(XBOX_INPUT_STATE *st, const Action *t, int v, int axes[4])
{
    BYTE *an = st->Gamepad.bAnalogButtons;
    switch (t->kind) {
    case T_BTN:
        if (v > 30)
            st->Gamepad.wButtons |= t->a;
        break;
    case T_AN2:
    case T_AN: {   /* the button (or the two) the game's Button Settings give this action now */
        unsigned char tab[10];
        game_buttons(tab);
        int slot = game_slot(t, tab);
        int b0 = slot < 0 ? -1 : slot < 8 ? slot : slot == 8 ? XBOX_BUTTON_X : XBOX_BUTTON_Y;
        int b1 = slot == 8 ? XBOX_BUTTON_A : slot == 9 ? XBOX_BUTTON_B : -1;
        if (b0 >= 0 && v > an[b0])
            an[b0] = (BYTE)v;
        if (b1 >= 0 && v > an[b1])
            an[b1] = (BYTE)v;
        break;
    }
    case T_AXIS:
        if (v > 30)
            axes[t->a] += t->b ? -32767 : 32767;
        break;
    }
}

static SHORT clamp_axis(int v)
{
    return (SHORT)(v > 32767 ? 32767 : v < -32767 ? -32767 : v);
}

int ngb_bind_invert(int view, int axis)
{
    ensure();
    return view >= 0 && view < 2 && axis >= 0 && axis < 2 && s_invert[view][axis];
}

void ngb_bind_set_invert(int view, int axis, int on)
{
    ensure();
    if (view < 0 || view > 1 || axis < 0 || axis > 1)
        return;
    s_invert[view][axis] = on != 0;
    s_gen++;
    save();
}

int ngb_bind_third_person_start(void)
{
    ensure();
    return s_third_start;
}

void ngb_bind_set_third_person_start(int on)
{
    ensure();
    s_third_start = on != 0;
    s_gen++;
    save();
}

int ngb_bind_soft_reset(void)
{
    ensure();
    return s_soft_reset;
}

void ngb_bind_set_soft_reset(int on)
{
    ensure();
    s_soft_reset = on != 0;
    soft_reset_apply();
    s_gen++;
    save();
}

/* Camera Sensitivity: 25 to 300 percent in quarters; a step up or down (round) */
int ngb_bind_mouse_sensitivity(void)
{
    ensure();
    return s_mouse_pct;
}

void ngb_bind_step_mouse_sensitivity(int dir)
{
    static const int steps[] = { 25, 50, 75, 100, 125, 150, 175, 200, 225, 250, 275, 300 };
    const int n = (int)(sizeof steps / sizeof steps[0]);
    ensure();
    int i = 0;
    while (i < n - 1 && steps[i] < s_mouse_pct)
        i++;
    s_mouse_pct = steps[(i + (dir < 0 ? n - 1 : 1)) % n];
    s_gen++;
    save();
}

/* Camera Sensitivity on one axis of the right stick: the part past where the game's camera starts to
 * turn, scaled. Measured (0.6 s, in play): 14078 turns 0 degrees, 20308 20, 26537 85, 32767 113 -- the
 * camera starts at about 18400, below it nothing. Below that the stick is left alone (a controller's
 * stick at rest is not quite 0). */
#define CAMERA_START 18000
int ngb_bind_camera_start(void) { return CAMERA_START; }   /* src/keyboard.c: where the mouse starts */
float ngb_bind_camera_boost(void)
{
    ensure();
    return s_mouse_pct > 100 ? s_mouse_pct / 100.0f : 1.0f;
}
/* Up to 100%: the stick past where the camera starts, scaled down. Over 100%: the stick as it is, and the
 * cameras' turn multiplied (src/camera.c): a full stick is already the game's top speed. */
static int camera_scale(int v)
{
    int m = v < 0 ? -v : v;
    if (s_mouse_pct >= 100 || m <= CAMERA_START)
        return v;
    m = CAMERA_START + (m - CAMERA_START) * s_mouse_pct / 100;
    return v < 0 ? -m : m;
}

/* The right stick as the player wants the camera turned (in play only), for the view it turns now:
 * inverted, and at the Camera Sensitivity -- the controller's, the mouse's and the keys' alike. */
static void apply_invert(XBOX_INPUT_STATE *st)
{
    extern int ngb_camera_first_person(void);   /* src/camera.c */
    const int *inv = s_invert[ngb_camera_first_person() ? 1 : 0];
    if (inv[0])
        st->Gamepad.sThumbRX = clamp_axis(-(int)st->Gamepad.sThumbRX);
    if (inv[1])
        st->Gamepad.sThumbRY = clamp_axis(-(int)st->Gamepad.sThumbRY);
    int rx = st->Gamepad.sThumbRX;
    st->Gamepad.sThumbRX = clamp_axis(camera_scale(st->Gamepad.sThumbRX));
    st->Gamepad.sThumbRY = clamp_axis(camera_scale(st->Gamepad.sThumbRY));
    static const char *log = (const char *)1;          /* NGB_CAMERA_SENS_LOG=1: the stick before and after */
    if (log == (const char *)1) log = getenv("NGB_CAMERA_SENS_LOG");
    if (log && rx) {
        static int n;
        if (n++ % 30 == 0)
            fprintf(stderr, "[CAMERA] sensitivity %d%%: right stick x %d -> %d\n", s_mouse_pct, rx, st->Gamepad.sThumbRX);
    }
}

/* The keyboard and mouse in play (src/keyboard.c): the pad as their bindings press it. */
void ngb_bind_play_state(const Uint8 *k, Uint32 mouse, XBOX_INPUT_STATE *st, int look_x, int look_y)
{
    ensure();
    if (ngb_quick_frozen()) {
        memset(&st->Gamepad, 0, sizeof st->Gamepad);
        return;
    }
    int axes[4] = { 0, 0, look_x, look_y };
    st->Gamepad.wButtons = 0;
    memset(st->Gamepad.bAnalogButtons, 0, sizeof st->Gamepad.bAnalogButtons);
    int quick = ngb_quick_controls_active();
    for (int a = 0; a < A_QUICK_WEAPONS; a++)
        if (((!quick || !quick_input(s_bind[a][0])) && input_down(s_bind[a][0], k, mouse)) ||
            ((!quick || !quick_input(s_bind[a][1])) && input_down(s_bind[a][1], k, mouse)))
            press(st, &s_act[a], 255, axes);
    st->Gamepad.sThumbLX = clamp_axis(axes[0]);
    st->Gamepad.sThumbLY = clamp_axis(axes[1]);
    st->Gamepad.sThumbRX = clamp_axis(axes[2]);
    st->Gamepad.sThumbRY = clamp_axis(axes[3]);
    apply_invert(st);
}

/* A controller's state, each input 0-255 (P_*). */
static void pad_inputs(const XBOX_INPUT_STATE *st, int v[P_COUNT])
{
    const BYTE *an = st->Gamepad.bAnalogButtons;
    WORD b = st->Gamepad.wButtons;
    static const unsigned char ai[8] = { P_A, P_B, P_X, P_Y, P_BLACK, P_WHITE, P_LT, P_RT };
    for (int i = 0; i < 8; i++)
        v[ai[i]] = an[i];
    static const struct { WORD bit; int p; } bm[8] = {
        { XBOX_GAMEPAD_START, P_START }, { XBOX_GAMEPAD_BACK, P_BACK }, { XBOX_GAMEPAD_LEFT_THUMB, P_LS },
        { XBOX_GAMEPAD_RIGHT_THUMB, P_RS }, { XBOX_GAMEPAD_DPAD_UP, P_DU }, { XBOX_GAMEPAD_DPAD_DOWN, P_DD },
        { XBOX_GAMEPAD_DPAD_LEFT, P_DL }, { XBOX_GAMEPAD_DPAD_RIGHT, P_DR },
    };
    for (int i = 0; i < 8; i++)
        v[bm[i].p] = b & bm[i].bit ? 255 : 0;
}

unsigned ngb_bind_quick_state(const Uint8 *keys, unsigned mouse, const XBOX_INPUT_STATE *pad)
{
    ensure();
    int v[P_COUNT] = {0};
    if (pad)
        pad_inputs(pad, v);
    unsigned bits = 0;
    for (int a = A_QUICK_WEAPONS; a < A_COUNT; a++) {
        int p = s_bind[a][2];
        if (input_down(s_bind[a][0], keys, mouse) || input_down(s_bind[a][1], keys, mouse) ||
            ((p & BIND_PAD) && (p & 255) < P_COUNT && v[p & 255] > 40))
            bits |= s_act[a].a;
    }
    return bits;
}

/* xboxrecomp's input, each controller read (weak there): in play the
 * controller's buttons go to the actions they are bound to. */
/* (xbox_pad_remap's per-controller state, 4 the touch controls: the pairs' latches, a pair button's time down
 * alone and the end of a tap let go in that time; the hold-back of a two-button move's own pair) */
static int s_pair_latched[5][A_COUNT];
static Uint64 s_pair_alone[5][P_COUNT], s_pair_tap[5][P_COUNT];
static Uint64 s_hb_down[5][8];
static BYTE s_hb_was[5][8], s_hb_back[5][8];

static void hold_back_reset(DWORD port)   /* (the remap stepped aside: none of it holds after) */
{
    int i = port < 5 ? (int)port : 4;
    memset(s_pair_latched[i], 0, sizeof s_pair_latched[i]);
    memset(s_pair_alone[i], 0, sizeof s_pair_alone[i]);
    memset(s_pair_tap[i], 0, sizeof s_pair_tap[i]);
    memset(s_hb_down[i], 0, sizeof s_hb_down[i]);
    memset(s_hb_was[i], 0, sizeof s_hb_was[i]);
    memset(s_hb_back[i], 0, sizeof s_hb_back[i]);
}

void xbox_pad_remap(XBOX_INPUT_STATE *st, DWORD port)
{
    if (ngb_quick_frozen()) {
        memset(&st->Gamepad, 0, sizeof st->Gamepad);
        hold_back_reset(port);
        return;
    }
    ensure();
    if (ngb_menu_up()) {
        hold_back_reset(port);
        return;
    }
    apply_invert(st);
    int quick = ngb_quick_controls_active();
    /* The controller as it is when nothing is rebound -- here or in the game's own Button Settings. */
    if (!quick && ((s_pad_identity && game_buttons_default()) || getenv("NGB_NO_PAD_REMAP"))) {
        hold_back_reset(port);
        return;
    }
    int v[P_COUNT], axes[4] = { 0 };
    pad_inputs(st, v);
    if (quick)
        for (int p = 0; p < P_COUNT; p++)
            if (quick_input(BIND_PAD | p)) v[p] = 0;
    st->Gamepad.wButtons = 0;
    memset(st->Gamepad.bAnalogButtons, 0, sizeof st->Gamepad.bAnalogButtons);
    /* Two buttons held together first: their action. Each one's own action waits: from the pair's press until
     * both are let go (the second let go after the first would do its own), and, as a pair's first button goes
     * down alone, a few frames for the second to join it (let go in them, a tap: its own action, a moment). */
    int *latched = s_pair_latched[port < 5 ? port : 4];
    Uint64 *alone_since = s_pair_alone[port < 5 ? port : 4], *tap_until = s_pair_tap[port < 5 ? port : 4];
    Uint64 now = SDL_GetTicks64();
    int held[P_COUNT], quiet[P_COUNT] = { 0 }, member[P_COUNT] = { 0 }, in_latched[P_COUNT] = { 0 };
    memcpy(held, v, sizeof held);
    for (int a = 0; a < A_COUNT; a++) {             /* the pairs held: their actions */
        int in = s_bind[a][2], b = BIND_PAD_SECOND(in), f = in & 0xFF;
        if (!(in & BIND_PAD) || b < 0 || b >= P_COUNT || f >= P_COUNT) {
            latched[a] = 0;
            continue;
        }
        member[f] = member[b] = 1;
        if (held[f] > 40 && held[b] > 40) {
            press(st, &s_act[a], held[f] < held[b] ? held[f] : held[b], axes);
            latched[a] = 1;
        } else if (held[f] <= 40 && held[b] <= 40) {
            latched[a] = 0;
        }
        if (latched[a])
            in_latched[f] = in_latched[b] = 1;
    }
    for (int p = 0; p < P_COUNT; p++) {             /* then each pair button once (two pairs may share one) */
        if (!member[p])
            continue;
        if (in_latched[p]) {
            quiet[p] = 1;
            alone_since[p] = 0;
            continue;
        }
        if (held[p] <= 40 && alone_since[p] && now - alone_since[p] < 60)   /* down alone: 60 ms for the other */
            tap_until[p] = now + 50;
        if (held[p] <= 40)
            alone_since[p] = 0;
        else if (!alone_since[p])
            alone_since[p] = now;
        if (alone_since[p] && now - alone_since[p] < 60)
            quiet[p] = 1;
    }
    for (int p = 0; p < P_COUNT; p++)
        if (quiet[p])
            v[p] = 0;
        else if (tap_until[p] > now && !v[p])
            v[p] = 255;
    for (int a = 0; a < A_COUNT; a++) {
        int in = s_bind[a][2];
        if ((in & BIND_PAD) && BIND_PAD_SECOND(in) < 0 && (in & 0xFF) < P_COUNT && v[in & 0xFF])
            press(st, &s_act[a], v[in & 0xFF], axes);
    }
    /* A two-button move bound elsewhere: the game's own pair (Y + B for Ninpo) made by two other actions held
     * together (within 100 ms of each other, as a pair is pressed) would still be it: the one of the two that went
     * down last waits until it is let go. One pressed while the other is long held (an attack in a jump) goes. */
    Uint64 *down_at = s_hb_down[port < 5 ? port : 4];
    BYTE *was = s_hb_was[port < 5 ? port : 4], *held_back = s_hb_back[port < 5 ? port : 4];
    BYTE *an = st->Gamepad.bAnalogButtons;
    for (int i = 0; i < 8; i++) {
        if (an[i] && !was[i])
            down_at[i] = now;
        if (!an[i])
            held_back[i] = 0;
        was[i] = an[i] != 0;
    }
    unsigned char tab[10];
    game_buttons(tab);
    for (int a = 0; a < A_COUNT; a++) {
        int in = s_bind[a][2];
        if (s_act[a].kind != T_AN2 || latched[a] || in == s_act[a].pad)
            continue;
        if ((in & BIND_PAD) && BIND_PAD_SECOND(in) < 0 && (in & 0xFF) < P_COUNT && v[in & 0xFF])
            continue;                              /* (its own one button: the pair is it) */
        int slot = game_slot(&s_act[a], tab);
        int b0 = slot < 0 ? -1 : slot < 8 ? slot : slot == 8 ? XBOX_BUTTON_X : XBOX_BUTTON_Y;
        int b1 = slot == 8 ? XBOX_BUTTON_A : slot == 9 ? XBOX_BUTTON_B : -1;
        if (b0 < 0 || b1 < 0 || !an[b0] || !an[b1] || held_back[b0] || held_back[b1])
            continue;
        Uint64 gap = down_at[b0] > down_at[b1] ? down_at[b0] - down_at[b1] : down_at[b1] - down_at[b0];
        if (gap < 100)
            held_back[down_at[b0] > down_at[b1] ? b0 : b1] = 1;
    }
    for (int i = 0; i < 8; i++)
        if (held_back[i])
            an[i] = 0;
}

/* ---------- waiting for an input ---------- */

static volatile int s_cap_action = -1, s_cap_slot, s_cap_result;
static Uint64 s_cap_since, s_cap_first_at;
static int s_cap_prev[P_COUNT], s_cap_first = -1;   /* a two-button action's first button, while held */

/* The actions a pair of buttons can be bound to: the game's own two-button moves */
int ngb_bind_pad_pair(int a) { return a >= 0 && a < A_COUNT && s_act[a].kind == T_AN2; }

int ngb_bind_capturing(void) { return s_cap_action >= 0; }

void ngb_bind_capture_begin(int a, int slot)
{
    ensure();
    s_cap_result = 0;
    s_cap_slot = slot;
    s_cap_since = SDL_GetTicks64();
    s_cap_first = -1;
    XBOX_INPUT_STATE st;
    memset(s_cap_prev, 0, sizeof s_cap_prev);
    if (xbox_InputGetStateRaw(0, &st) == 0)
        pad_inputs(&st, s_cap_prev);
    __atomic_store_n(&s_cap_action, a, __ATOMIC_RELEASE);
}

void ngb_bind_capture_cancel(void)
{
    s_cap_action = -1;
}

int ngb_bind_capture_tick(void)
{
    int a = s_cap_action;
    if (a < 0)
        return 0;
    int r = __atomic_load_n(&s_cap_result, __ATOMIC_ACQUIRE);
    if (!r && s_cap_slot == 2) {                   /* the controller: its next button down */
        XBOX_INPUT_STATE st;
        int v[P_COUNT] = { 0 };
        if (xbox_InputGetStateRaw(0, &st) == 0) {
            pad_inputs(&st, v);
            for (int p = 0; p < P_COUNT && !r; p++)
                if (v[p] > 120 && s_cap_prev[p] <= 120) {
                    if (!ngb_bind_pad_pair(a))
                        r = BIND_PAD | p;
                    else if (s_cap_first < 0) {        /* a pair's first: held, a second may join it */
                        s_cap_first = p;
                        s_cap_first_at = SDL_GetTicks64();
                    } else if (p != s_cap_first)       /* (one order: X + A is A + X) */
                        r = s_cap_first < p ? BIND_PAD2(s_cap_first, p) : BIND_PAD2(p, s_cap_first);
                }
            /* let go alone, or held a while alone: that one button */
            if (!r && s_cap_first >= 0 &&
                (v[s_cap_first] <= 40 || SDL_GetTicks64() - s_cap_first_at > 1500))
                r = BIND_PAD | s_cap_first;
            memcpy(s_cap_prev, v, sizeof v);
        }
        if (!r && s_cap_first < 0 && SDL_GetTicks64() - s_cap_since > 6000)
            r = -1;                                    /* none: as it was */
    }
    if (!r)
        return 1;
    if (r > 0)
        ngb_bind_set(a, s_cap_slot, r);
    s_cap_action = -1;
    return 0;
}

/* The window's thread, every event (src/ingame_menu.c): while waiting for
 * a key, the next key or mouse button down is it (Esc: none); true when
 * the event was taken. */
int ngb_bind_event(const SDL_Event *ev)
{
    if (s_cap_action < 0)
        return 0;
    if (ev->type == SDL_KEYDOWN) {
        if (ev->key.repeat || __atomic_load_n(&s_cap_result, __ATOMIC_ACQUIRE))
            return 1;
        SDL_Scancode sc = ev->key.keysym.scancode;
        int r = sc == SDL_SCANCODE_ESCAPE || s_cap_slot == 2 ? -1 : (int)sc;
        __atomic_store_n(&s_cap_result, r, __ATOMIC_RELEASE);
        return 1;
    }
    if (ev->type == SDL_MOUSEBUTTONDOWN) {
        if (__atomic_load_n(&s_cap_result, __ATOMIC_ACQUIRE))
            return 1;
        int b = ev->button.button;
        int m = b == SDL_BUTTON_LEFT ? 1 : b == SDL_BUTTON_MIDDLE ? 2 : b == SDL_BUTTON_RIGHT ? 3
              : b == SDL_BUTTON_X1 ? 4 : b == SDL_BUTTON_X2 ? 5 : 0;
        int r = s_cap_slot == 2 ? (b == SDL_BUTTON_RIGHT ? -1 : 0) : m ? BIND_MOUSE | m : 0;
        if (r)
            __atomic_store_n(&s_cap_result, r, __ATOMIC_RELEASE);
        return 1;
    }
    return ev->type == SDL_KEYUP || ev->type == SDL_MOUSEBUTTONUP || ev->type == SDL_MOUSEWHEEL;
}
