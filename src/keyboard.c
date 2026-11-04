/*
 * keyboard.c -- Ninja Gaiden Black on the keyboard and mouse.
 *
 * The runtime (xboxrecomp src/input) switches between the controllers and
 * the keyboard and mouse by which was touched last; while the keyboard and
 * mouse are in use, this is how they press the pad (the game's default
 * actions in brackets):
 *
 *   W A S D        left stick (move)       mouse, numpad 8456  right stick
 *   left mouse, J  X (attack)              right mouse, K      Y (special attack)
 *   Space          A (jump)                E                   B (projectile)
 *   F              Y+B (ninpo)             C                   X+A (wind run)
 *   Shift          L (guard)               Q, middle mouse     R (center camera)
 *   Tab            Black (map)             R                   White (karma window)
 *   Esc, Enter     START                   Backspace           BACK
 *   arrows         d-pad                   Z / V               left / right stick click
 *
 * These are the defaults: in play the keys and mouse buttons are as the
 * Controls page has bound them (src/bindings.c; NGB_FIXED_KEYS=1 keeps these).
 * In the game's menus Esc is B (src/ingame_menu.c), Q and E are L and R
 * (the pause menu's tabs) and the mouse points (src/menu_mouse.c); in play the mouse is captured, its motion the right
 * stick. The pointer is hidden except in the menus with the keyboard and mouse in use. The button prompts become keys (src/pause_tab.c).
 */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "input/xinput_xbox.h"

int ngb_menu_up(void);   /* src/menu_mouse.c */

static SHORT axis(const Uint8 *k, int neg, int pos)
{
    return (SHORT)((k[pos] ? 32767 : 0) - (k[neg] ? 32767 : 0));
}

void ngb_bind_play_state(const Uint8 *k, Uint32 mouse, XBOX_INPUT_STATE *st, int look_x, int look_y);

/* The button that skips a movie: Start on the pad (or the key bound to it), as for the in-game cutscenes
 * (#139), from the first pad's state as the title reads it. */
int ngb_skip_button(void)
{
    XBOX_INPUT_STATE st;
    if (xbox_InputGetState(0, &st) != 0)
        return 0;
    return (st.Gamepad.wButtons & XBOX_GAMEPAD_START) != 0;
}


/* The mouse's speed as the camera's stick: from where the game's camera starts to turn (src/bindings.c),
 * not from 0 -- below that the camera does nothing, and a slow movement of the mouse turned nothing (#67) */
int ngb_bind_camera_start(void);
static int mouse_look(int v)
{
    if (!v)
        return 0;
    int start = ngb_bind_camera_start(), m = v < 0 ? -v : v;
    m = start + (int)((long)m * (32767 - start) / 32767);
    return v < 0 ? -m : m;
}

static int keyboard_map(const Uint8 *k, Uint32 mouse, XBOX_INPUT_STATE *st)
{
    int menu = ngb_menu_up();
    {   /* NGB_MENU_UP_LOG=1: when the keys change between the bindings (play) and the menus' layout */
        static int last = -1;
        static const char *log = (const char *)1;
        if (log == (const char *)1) log = getenv("NGB_MENU_UP_LOG");
        if (log && menu != last) {
            fprintf(stderr, "[KEYS] %s\n", menu ? "menu layout (a game menu ran)" : "play: the bindings");
            last = menu;
        }
    }
    static int fixed_keys = -1;
    if (fixed_keys < 0)
        fixed_keys = getenv("NGB_FIXED_KEYS") != NULL;
    if (!menu && !fixed_keys) {   /* in play: as bound (src/bindings.c, the Controls page) */
        SHORT mx = 0, my = 0;
        if (SDL_GetRelativeMouseMode()) {
            xbox_InputMouseStick(900.0f, &mx, &my);
            mx = (SHORT)mouse_look(mx);
            my = (SHORT)mouse_look(my);
        }
        ngb_bind_play_state(k, mouse, st, mx, my);
        return 1;
    }
    WORD b = 0;
    if (k[SDL_SCANCODE_UP])        b |= XBOX_GAMEPAD_DPAD_UP;
    if (k[SDL_SCANCODE_DOWN])      b |= XBOX_GAMEPAD_DPAD_DOWN;
    if (k[SDL_SCANCODE_LEFT])      b |= XBOX_GAMEPAD_DPAD_LEFT;
    if (k[SDL_SCANCODE_RIGHT])     b |= XBOX_GAMEPAD_DPAD_RIGHT;
    if (k[SDL_SCANCODE_RETURN] || k[SDL_SCANCODE_KP_ENTER] || (k[SDL_SCANCODE_ESCAPE] && !menu))
        b |= XBOX_GAMEPAD_START;
    if (k[SDL_SCANCODE_BACKSPACE]) b |= XBOX_GAMEPAD_BACK;
    if (k[SDL_SCANCODE_Z])         b |= XBOX_GAMEPAD_LEFT_THUMB;
    if (k[SDL_SCANCODE_V])         b |= XBOX_GAMEPAD_RIGHT_THUMB;
    st->Gamepad.wButtons = b;

    /* In the menus the mouse's buttons are the pointer's (src/menu_mouse.c). */
    int lmb = !menu && (mouse & SDL_BUTTON(SDL_BUTTON_LEFT));
    int rmb = !menu && (mouse & SDL_BUTTON(SDL_BUTTON_RIGHT));
    int mmb = !menu && (mouse & SDL_BUTTON(SDL_BUTTON_MIDDLE));
    int ninpo = k[SDL_SCANCODE_F], windrun = k[SDL_SCANCODE_C];
    BYTE *an = st->Gamepad.bAnalogButtons;
    an[XBOX_BUTTON_X] = (lmb || k[SDL_SCANCODE_J] || windrun) ? 255 : 0;
    an[XBOX_BUTTON_Y] = (rmb || k[SDL_SCANCODE_K] || ninpo) ? 255 : 0;
    an[XBOX_BUTTON_A] = (k[SDL_SCANCODE_SPACE] || windrun) ? 255 : 0;
    if (menu) {   /* Q and E change tabs (L, R); Esc is B (src/ingame_menu.c) */
        an[XBOX_BUTTON_B] = 0;
        an[XBOX_BUTTON_LTRIGGER] = k[SDL_SCANCODE_Q] ? 255 : 0;
        an[XBOX_BUTTON_RTRIGGER] = k[SDL_SCANCODE_E] ? 255 : 0;
    } else {
        an[XBOX_BUTTON_B] = (k[SDL_SCANCODE_E] || ninpo) ? 255 : 0;
        an[XBOX_BUTTON_LTRIGGER] = (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT]) ? 255 : 0;
        an[XBOX_BUTTON_RTRIGGER] = (k[SDL_SCANCODE_Q] || mmb) ? 255 : 0;
    }
    an[XBOX_BUTTON_BLACK] = k[SDL_SCANCODE_TAB] ? 255 : 0;
    an[XBOX_BUTTON_WHITE] = k[SDL_SCANCODE_R] ? 255 : 0;

    st->Gamepad.sThumbLX = axis(k, SDL_SCANCODE_A, SDL_SCANCODE_D);
    st->Gamepad.sThumbLY = axis(k, SDL_SCANCODE_S, SDL_SCANCODE_W);
    SHORT mx = 0, my = 0;
    if (!menu && SDL_GetRelativeMouseMode())
        xbox_InputMouseStick(900.0f, &mx, &my);
    int rx = axis(k, SDL_SCANCODE_KP_4, SDL_SCANCODE_KP_6) + mx;
    int ry = axis(k, SDL_SCANCODE_KP_5, SDL_SCANCODE_KP_8) + my;
    st->Gamepad.sThumbRX = (SHORT)(rx > 32767 ? 32767 : rx < -32767 ? -32767 : rx);
    st->Gamepad.sThumbRY = (SHORT)(ry > 32767 ? 32767 : ry < -32767 ? -32767 : ry);
    return 1;
}

int xbox_keyboard_map(const Uint8 *k, Uint32 mouse, XBOX_INPUT_STATE *st)
{
    int r = keyboard_map(k, mouse, st);
#ifdef __ANDROID__
    {   /* the touch controls (src/touch_pad.c), a pad: through its bindings as a controller's state goes */
        extern void ngb_touch_map(XBOX_INPUT_STATE *st);
        extern void xbox_pad_remap(XBOX_INPUT_STATE *st, DWORD port);
        XBOX_INPUT_STATE t;
        memset(&t, 0, sizeof t);
        ngb_touch_map(&t);
        xbox_pad_remap(&t, 4);   /* (a port of its own: the pairs' state, src/bindings.c) */
        st->Gamepad.wButtons |= t.Gamepad.wButtons;
        for (int i = 0; i < 8; i++)
            if (t.Gamepad.bAnalogButtons[i] > st->Gamepad.bAnalogButtons[i])
                st->Gamepad.bAnalogButtons[i] = t.Gamepad.bAnalogButtons[i];
        if (t.Gamepad.sThumbLX || t.Gamepad.sThumbLY) {
            st->Gamepad.sThumbLX = t.Gamepad.sThumbLX;
            st->Gamepad.sThumbLY = t.Gamepad.sThumbLY;
        }
        if (t.Gamepad.sThumbRX || t.Gamepad.sThumbRY) {
            st->Gamepad.sThumbRX = t.Gamepad.sThumbRX;
            st->Gamepad.sThumbRY = t.Gamepad.sThumbRY;
        }
    }
#endif
    return r;
}

/* The window's thread, each pump: the mouse captured in play while it is
 * the device in use, given back in the menus and to the controller. */
static volatile Uint64 s_capture_changed;

void nv2a_window_tick(void)
{
    static int captured;
    SDL_Window *w = SDL_GetKeyboardFocus();
    static int no_capture = -1;
    if (no_capture < 0)
        no_capture = getenv("NGB_NO_MOUSE_CAPTURE") != NULL;
    int want = w && xbox_InputKeyboardActive() && !ngb_menu_up() && !no_capture;
    if (want != captured) {
        captured = want;
        s_capture_changed = SDL_GetTicks64();
        SDL_SetRelativeMouseMode(want ? SDL_TRUE : SDL_FALSE);
    }
    /* The pointer shows only where it points at something: in the game's
     * menus, with the keyboard and mouse in use. */
    static int shown = -1;
    int show = xbox_InputKeyboardActive() && ngb_menu_up();
    if (show != shown) {
        shown = show;
        SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
    }
}

/* Just after the capture changed the pointer moves by itself (SDL puts it
 * back): not the player pointing at a menu item. */
int ngb_mouse_settling(void)
{
    return SDL_GetTicks64() - s_capture_changed < 300;
}
