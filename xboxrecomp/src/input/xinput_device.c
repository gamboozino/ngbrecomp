/**
 * Xbox Input Compatibility Layer
 *
 * Translates the Xbox controller API to a host gamepad backend.
 * Handles the structural differences between the Xbox gamepad
 * (analog buttons as bytes, separate trigger channels) and the host
 * (digital face buttons, trigger axes).
 *
 *   _WIN32 -> Windows XInput (unless XBOX_SDL_BACKENDS)
 *   POSIX, or XBOX_SDL_BACKENDS -> SDL2 GameController
 */

#include "xinput_xbox.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================== */
#if defined(_WIN32) && !defined(XBOX_SDL_BACKENDS)
/* ====================  XInput backend  ================================== */
/* ======================================================================== */

#include <xinput.h>
#pragma comment(lib, "xinput.lib")

static BOOL  g_controller_connected[XBOX_MAX_CONTROLLERS] = { FALSE };
static DWORD g_last_packet[XBOX_MAX_CONTROLLERS] = { 0 };

/* ---- keyboard, when there is no pad --------------------------------------
 *
 * A title that waits on PRESS START is unreachable on a machine with no
 * controller plugged in, which is most machines someone brings this up on.
 * The whole point of a recompilation is to be able to look at the thing
 * running, and a build nobody can press a button in cannot be looked at.
 *
 * Off by default, because a keyboard silently acting as player 1 is
 * surprising when a real pad is what you meant to use. RECOMP_KEYBOARD=1
 * turns it on. It only ever answers for port 0, and is merged on top of a
 * pad connected there, so a real controller keeps working.
 *
 * The keys are the ones a Dreamcast or Saturn emulator would pick, which is
 * the closest thing to a convention here:
 *
 *   arrows        d-pad             Enter      START
 *   Z X A S       A B X Y           Backspace  BACK
 *   Q E           white black       1 3        triggers
 *   numpad 8/2/4/6 left thumb       I/K/J/L    right thumb
 *
 * Keys come from the framebuffer window, which only receives them while it
 * has the focus, so typing in another window does not drive the game.
 */
static BOOL keyboard_enabled(void)
{
    static int on = -1;
    if (on < 0) {
        const char *v = getenv("RECOMP_KEYBOARD");
        on = (v && *v && *v != '0') ? 1 : 0;
    }
    return on ? TRUE : FALSE;
}

/* The framebuffer window records what is held; see src/video/fb_present.c.
 *
 * GetAsyncKeyState was the first attempt and reads nothing here -- under
 * Wine it answers about a state a GDI-drawing guest never touches. Going
 * through the window is also the better gate: a window only receives keys
 * while it has the focus, so there is no separate focus check to get wrong
 * and no way for typing in another application to drive the game.
 *
 * Declared rather than included so src/input does not depend on the video
 * module's include path. */
extern int xbox_FramebufferKeyDown(int vk);

static BOOL key_down(int vk)
{
    return xbox_FramebufferKeyDown(vk) ? TRUE : FALSE;
}

/* A thumb axis from two keys, at the full deflection a digital key implies. */
static SHORT axis_from_keys(int negative, int positive)
{
    int v = 0;
    if (key_down(negative)) v -= 32767;
    if (key_down(positive)) v += 32767;
    return (SHORT)v;
}

static void keyboard_state(XBOX_INPUT_STATE *pState)
{
    static DWORD packet;
    WORD b = 0;

    memset(pState, 0, sizeof(*pState));

    if (key_down(VK_UP))     b |= XBOX_GAMEPAD_DPAD_UP;
    if (key_down(VK_DOWN))   b |= XBOX_GAMEPAD_DPAD_DOWN;
    if (key_down(VK_LEFT))   b |= XBOX_GAMEPAD_DPAD_LEFT;
    if (key_down(VK_RIGHT))  b |= XBOX_GAMEPAD_DPAD_RIGHT;
    if (key_down(VK_RETURN)) b |= XBOX_GAMEPAD_START;
    if (key_down(VK_BACK))   b |= XBOX_GAMEPAD_BACK;
    if (key_down(VK_SHIFT))  b |= XBOX_GAMEPAD_LEFT_THUMB;
    if (key_down(VK_CONTROL))b |= XBOX_GAMEPAD_RIGHT_THUMB;
    pState->Gamepad.wButtons = b;

    /* Analog on the console, so a key is 255 rather than a flag -- a title
     * that reads these as a pressure never sees a press if they are 1. */
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_A]        = key_down('Z') ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_B]        = key_down('X') ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_X]        = key_down('A') ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_Y]        = key_down('S') ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_WHITE]    = key_down('Q') ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_BLACK]    = key_down('E') ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_LTRIGGER] = key_down('1') ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_RTRIGGER] = key_down('3') ? 255 : 0;

    /* W/A/S/D would collide with the face buttons above, so the left thumb
     * shares the arrow keys' row on the numeric pad instead. */
    pState->Gamepad.sThumbLX = axis_from_keys(VK_NUMPAD4, VK_NUMPAD6);
    pState->Gamepad.sThumbLY = axis_from_keys(VK_NUMPAD2, VK_NUMPAD8);
    pState->Gamepad.sThumbRX = axis_from_keys('J', 'L');
    pState->Gamepad.sThumbRY = axis_from_keys('K', 'I');

    /* The title's input layer looks for button edges, so the packet number
     * has to move whenever the state does or a press is never noticed. */
    pState->dwPacketNumber = ++packet;
}

void xbox_InputInit(void)
{
    for (DWORD i = 0; i < XBOX_MAX_CONTROLLERS; i++) {
        XINPUT_STATE state;
        DWORD result = XInputGetState(i, &state);
        g_controller_connected[i] = (result == ERROR_SUCCESS);
    }
}

static DWORD input_get_state(DWORD dwPort, XBOX_INPUT_STATE *pState, int raw)
{
    XINPUT_STATE xi_state;
    DWORD result;

    if (dwPort >= XBOX_MAX_CONTROLLERS || !pState)
        return ERROR_DEVICE_NOT_CONNECTED;

    result = XInputGetState(dwPort, &xi_state);
    if (result != ERROR_SUCCESS) {
        g_controller_connected[dwPort] = FALSE;
        if (!raw && dwPort == 0 && keyboard_enabled()) {
            keyboard_state(pState);
            return ERROR_SUCCESS;
        }
        return result;
    }

    g_controller_connected[dwPort] = TRUE;
    g_last_packet[dwPort] = xi_state.dwPacketNumber;

    memset(pState, 0, sizeof(XBOX_INPUT_STATE));
    pState->dwPacketNumber = xi_state.dwPacketNumber;
    pState->Gamepad.wButtons = xi_state.Gamepad.wButtons & 0x00FF;

    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_A] =
        (xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_A) ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_B] =
        (xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_B) ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_X] =
        (xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_X) ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_Y] =
        (xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_BLACK] =
        (xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_WHITE] =
        (xi_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_LTRIGGER] = xi_state.Gamepad.bLeftTrigger;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_RTRIGGER] = xi_state.Gamepad.bRightTrigger;

    pState->Gamepad.sThumbLX = xi_state.Gamepad.sThumbLX;
    pState->Gamepad.sThumbLY = xi_state.Gamepad.sThumbLY;
    pState->Gamepad.sThumbRX = xi_state.Gamepad.sThumbRX;
    pState->Gamepad.sThumbRY = xi_state.Gamepad.sThumbRY;

    /* Merge the keyboard on top rather than only standing in for a missing
     * pad.
     *
     * The first version put the keyboard behind XInput's failure, on the
     * assumption that with nothing plugged in the call would fail. It does
     * not: under Wine XInputGetState returns ERROR_SUCCESS and a gamepad
     * with every button at rest, so the fallback was unreachable and
     * pressing a key did nothing at all.
     *
     * Merging is also the better rule. A real pad keeps working -- its
     * buttons are already in pState and the keyboard only adds to them --
     * and there is no special case left to get wrong. */
    if (!raw && dwPort == 0 && keyboard_enabled()) {
        XBOX_INPUT_STATE kb;
        int i;
        keyboard_state(&kb);
        pState->Gamepad.wButtons |= kb.Gamepad.wButtons;
        for (i = 0; i < 8; i++)
            if (kb.Gamepad.bAnalogButtons[i] > pState->Gamepad.bAnalogButtons[i])
                pState->Gamepad.bAnalogButtons[i] = kb.Gamepad.bAnalogButtons[i];
        if (kb.Gamepad.sThumbLX) pState->Gamepad.sThumbLX = kb.Gamepad.sThumbLX;
        if (kb.Gamepad.sThumbLY) pState->Gamepad.sThumbLY = kb.Gamepad.sThumbLY;
        if (kb.Gamepad.sThumbRX) pState->Gamepad.sThumbRX = kb.Gamepad.sThumbRX;
        if (kb.Gamepad.sThumbRY) pState->Gamepad.sThumbRY = kb.Gamepad.sThumbRY;
        /* The input layer records edges, so an unchanged packet number is
         * read as the same state and the press never happens. */
        pState->dwPacketNumber = kb.dwPacketNumber;
    }

    return ERROR_SUCCESS;
}

int xbox_InputControllerPresent(void)
{
    for (int i = 0; i < XBOX_MAX_CONTROLLERS; i++)
        if (g_controller_connected[i])
            return 1;
    return 0;
}

/* (The keyboard here stands in only for a missing pad.) */
int xbox_InputKeyboardActive(void)
{
    return !xbox_InputControllerPresent();
}

int xbox_InputPadPlayStation(void)   /* XInput: Xbox pads only */
{
    return 0;
}

int xbox_InputPadNintendo(void)
{
    return 0;
}

void xbox_InputNintendoLabels(int labels)
{
    (void)labels;
}

void xbox_InputMouseStick(float full, short *x, short *y)
{
    (void)full;
    *x = *y = 0;
}

DWORD xbox_InputSetState(DWORD dwPort, const XBOX_VIBRATION *pVibration)
{
    XINPUT_VIBRATION xi_vib;

    if (dwPort >= XBOX_MAX_CONTROLLERS || !pVibration)
        return ERROR_DEVICE_NOT_CONNECTED;

    xi_vib.wLeftMotorSpeed = pVibration->wLeftMotorSpeed;
    xi_vib.wRightMotorSpeed = pVibration->wRightMotorSpeed;
    return XInputSetState(dwPort, &xi_vib);
}

BOOL xbox_InputIsConnected(DWORD dwPort)
{
    if (dwPort >= XBOX_MAX_CONTROLLERS) return FALSE;
    return g_controller_connected[dwPort];
}

DWORD xbox_InputGetCapabilities(DWORD dwPort, DWORD dwFlags, XBOX_INPUT_CAPABILITIES *pCaps)
{
    XINPUT_CAPABILITIES xi_caps;
    DWORD result;

    if (dwPort >= XBOX_MAX_CONTROLLERS || !pCaps)
        return ERROR_DEVICE_NOT_CONNECTED;

    result = XInputGetCapabilities(dwPort, dwFlags, &xi_caps);
    if (result != ERROR_SUCCESS) return result;

    memset(pCaps, 0, sizeof(XBOX_INPUT_CAPABILITIES));
    pCaps->Type = xi_caps.Type;
    pCaps->SubType = xi_caps.SubType;
    pCaps->Flags = xi_caps.Flags;
    return ERROR_SUCCESS;
}

/* ======================================================================== */
#else /* !_WIN32 */
/* ====================  SDL2 GameController backend  ===================== */
/* ======================================================================== */

#include <SDL.h>

static SDL_GameController *g_pads[XBOX_MAX_CONTROLLERS];
static SDL_Joystick *g_vpad;   /* RECOMP_PAD_HOST's test pad: the only one taken while it is there */
/* Each slot's joystick instance and SDL controller type, cached on the window's thread (open_controllers),
 * for xbox_InputPadPlayStation; -1 an empty slot. */
static volatile int g_pad_id[XBOX_MAX_CONTROLLERS] = { -1, -1, -1, -1 };
static volatile int g_pad_type[XBOX_MAX_CONTROLLERS];
static BOOL  g_controller_connected[XBOX_MAX_CONTROLLERS];
static DWORD g_packet[XBOX_MAX_CONTROLLERS];

/* Controllers are opened and closed on the thread that pumps SDL events --
 * the window's -- through an event watch, so hotplug works and nothing here
 * calls into SDL's joystick layer from the USB controller thread, which only
 * reads state the pump already updated. */
static SDL_mutex *g_pad_lock;

/* (Re)open every attached game controller into the first XBOX_MAX_CONTROLLERS
 * slots, keeping already-open ones where they are. */
static void open_controllers(void)
{
    SDL_LockMutex(g_pad_lock);
    for (int s = 0; s < XBOX_MAX_CONTROLLERS; s++)
        if (g_pads[s] && !SDL_GameControllerGetAttached(g_pads[s])) {
            SDL_GameControllerClose(g_pads[s]);
            g_pads[s] = NULL;
            g_controller_connected[s] = FALSE;
            g_pad_id[s] = -1;
        }
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        SDL_JoystickID id;
        int s, have = 0;
        if (!SDL_IsGameController(i))
            continue;
        if (g_vpad && SDL_JoystickGetDeviceInstanceID(i) != SDL_JoystickInstanceID(g_vpad))
            continue;
#ifdef __ANDROID__
        /* Fingerprint sensors on many phones (uinput-fpc, uinput-goodix) say they are game controllers: taken as
         * pads, they hid the touch controls (#181) */
        {
            const char *name = SDL_JoystickNameForIndex(i);
            if (name && !strncmp(name, "uinput-", 7))
                continue;
        }
#endif
        id = SDL_JoystickGetDeviceInstanceID(i);
        for (s = 0; s < XBOX_MAX_CONTROLLERS; s++)
            if (g_pads[s] && SDL_JoystickInstanceID(
                    SDL_GameControllerGetJoystick(g_pads[s])) == id)
                have = 1;
        if (have)
            continue;
        for (s = 0; s < XBOX_MAX_CONTROLLERS && g_pads[s]; s++)
            ;
        if (s == XBOX_MAX_CONTROLLERS)
            break;
        g_pads[s] = SDL_GameControllerOpen(i);
        g_controller_connected[s] = g_pads[s] != NULL;
        if (g_pads[s]) {
            g_pad_type[s] = (int)SDL_GameControllerGetType(g_pads[s]);
            g_pad_id[s] = (int)SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(g_pads[s]));
        }
        if (g_pads[s])
            fprintf(stderr, "[INPUT] pad %d: %s\n", s + 1,
                    SDL_GameControllerName(g_pads[s]));
    }
    SDL_UnlockMutex(g_pad_lock);
}

/* The device in use: the keyboard and mouse or the controllers, whichever
 * was last touched -- the other's input is ignored until then. -1 not yet
 * known (the keyboard while no controller is connected). */
static volatile int g_kbm_active = -1;
static volatile int g_hotplug;
static volatile int g_pad_last = -1;   /* the joystick instance of the pad last touched */

static volatile int g_labels = 1, g_labels_set = -1;   /* Nintendo pads by label (SDL's default) or by position */

/* RECOMP_PAD_HOST=<file>, for tests: a virtual controller SDL sees as a real one (the host's own pages, a
 * title's bindings and its pairs read it, unlike RECOMP_PAD_LIVE's USB pad). Each line appended to the file
 * is <button>[+<button>...][:<hold ms>] (150 ms by default), one after another with 200 ms between (none
 * after a line starting with &, to change what is held without letting go):
 * a b x y lb rb lt rt start back ls rs up down left right. Polled on the window's thread. */
static struct { unsigned mask; Uint64 at, until; } g_vq[64];
static int g_vq_n, g_vq_on = -1;

static unsigned vpad_mask(const char *s, unsigned *hold)
{
    static const char *const name[] = { "a", "b", "x", "y", "back", NULL, "start", "ls", "rs", "lb", "rb",
                                        "up", "down", "left", "right" };   /* (SDL_CONTROLLER_BUTTON_*) */
    unsigned m = 0;
    *hold = 150;
    while (*s) {
        size_t n = strcspn(s, "+:\r\n");
        int b = -1;
        for (int i = 0; i < (int)(sizeof name / sizeof *name); i++)
            if (name[i] && strlen(name[i]) == n && !strncmp(s, name[i], n))
                b = i;
        if (n == 2 && !strncmp(s, "lt", 2)) b = 16;
        if (n == 2 && !strncmp(s, "rt", 2)) b = 17;
        if (b >= 0) m |= 1u << b;
        s += n;
        if (*s == ':') { *hold = (unsigned)strtoul(s + 1, NULL, 0); break; }
        if (*s) s++;
    }
    return m;
}

static void vpad_poll(void)
{
    static const char *path;
    static long consumed;
    static char line[128];
    static size_t nl;
    if (!g_vpad)
        return;
    if (!path)
        path = getenv("RECOMP_PAD_HOST");
    Uint64 now = SDL_GetTicks64();
    FILE *f = fopen(path, "rb");
    if (f && fseek(f, 0, SEEK_END) == 0 && ftell(f) < consumed)
        consumed = 0;                                   /* (the file started over) */
    if (f && fseek(f, consumed, SEEK_SET) == 0) {
        int c;
        while ((c = fgetc(f)) != EOF) {
            consumed++;
            if (c != '\n') {
                if (nl < sizeof line - 1) line[nl++] = (char)c;
                continue;
            }
            line[nl] = 0;
            nl = 0;
            int join = line[0] == '&';
            unsigned hold, m = vpad_mask(line + join, &hold);
            if (!m || g_vq_n >= 64)
                continue;
            static Uint64 last_until;                  /* (the last one's end, popped or not) */
            Uint64 at = last_until + (join ? 0 : 200);
            if (at < now) at = now;
            last_until = at + hold;
            g_vq[g_vq_n].mask = m;
            g_vq[g_vq_n].at = at;
            g_vq[g_vq_n].until = at + hold;
            g_vq_n++;
            fprintf(stderr, "  PAD: host \"%s\"\n", line);
        }
    }
    if (f)
        fclose(f);
    unsigned m = 0;
    while (g_vq_n && now >= g_vq[0].until) {
        memmove(g_vq, g_vq + 1, --g_vq_n * sizeof *g_vq);
    }
    if (g_vq_n && now >= g_vq[0].at)
        m = g_vq[0].mask;
    if ((int)m == g_vq_on)
        return;
    g_vq_on = (int)m;
    for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX && b < 15; b++)
        SDL_JoystickSetVirtualButton(g_vpad, b, (m >> b) & 1);
    SDL_JoystickSetVirtualAxis(g_vpad, SDL_CONTROLLER_AXIS_TRIGGERLEFT, (m >> 16) & 1 ? 32767 : -32768);
    SDL_JoystickSetVirtualAxis(g_vpad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, (m >> 17) & 1 ? 32767 : -32768);
}

static void vpad_attach(void)
{
    const char *p = getenv("RECOMP_PAD_HOST");
    if (!p || !*p)
        return;
    SDL_VirtualJoystickDesc d;
    SDL_zero(d);
    d.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    d.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    d.naxes = SDL_CONTROLLER_AXIS_MAX;
    d.nbuttons = 15;
    d.name = "Test pad";
    int i = SDL_JoystickAttachVirtualEx(&d);
    g_vpad = i < 0 ? NULL : SDL_JoystickOpen(i);
    if (g_vpad) {
        SDL_JoystickSetVirtualAxis(g_vpad, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
        SDL_JoystickSetVirtualAxis(g_vpad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
    }
    fprintf(stderr, "  PAD: host test pad from %s%s\n", p, g_vpad ? "" : " (not attached)");
}

/* The window's thread, after it pumped SDL's events (outside its locks). */
void xbox_InputHotplugPoll(void)
{
    vpad_poll();
    if (g_pad_lock && __atomic_exchange_n(&g_hotplug, 0, __ATOMIC_ACQ_REL))
        open_controllers();
}

/* A Nintendo pad's face buttons by their labels (1: the game's A is the one marked A) or by their places (0: the
 * one where an Xbox pad's A is), for xbox_InputInit: SDL picks a pad's mapping as it starts its controllers (only
 * its own Switch driver, not Android's nor evdev's, would follow a later change), so a change waits for the next
 * start. */
void xbox_InputNintendoLabels(int labels)
{
    __atomic_store_n(&g_labels, labels != 0, __ATOMIC_RELEASE);
}
/* Mouse motion since the last sample, for the right stick (see keyboard_state). */
static volatile int g_mouse_dx, g_mouse_dy;

static void set_kbm(int on)
{
    __atomic_store_n(&g_kbm_active, on, __ATOMIC_RELEASE);
}

static int SDLCALL pad_event_watch(void *unused, SDL_Event *ev)
{
    (void)unused;
    switch (ev->type) {
    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
        /* Not here: a watcher runs inside SDL's event pump, which holds the
         * joystick lock, while the game's thread may hold g_pad_lock waiting
         * for that lock (xbox_InputGetState) -- taking g_pad_lock here hung
         * both (a pad connecting or dropping in a menu). After the pump. */
        __atomic_store_n(&g_hotplug, 1, __ATOMIC_RELEASE);
        break;
    case SDL_KEYDOWN:
#ifdef __ANDROID__   /* (its back gesture and volume keys come as keys too: not a keyboard in use) */
        if (ev->key.keysym.scancode == SDL_SCANCODE_AC_BACK || ev->key.keysym.scancode == SDL_SCANCODE_VOLUMEUP ||
            ev->key.keysym.scancode == SDL_SCANCODE_VOLUMEDOWN)
            break;
#endif
        if (!ev->key.repeat)
            set_kbm(1);
        break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEWHEEL:
        set_kbm(1);
        break;
    case SDL_MOUSEMOTION:
        __atomic_add_fetch(&g_mouse_dx, ev->motion.xrel, __ATOMIC_RELAXED);
        __atomic_add_fetch(&g_mouse_dy, ev->motion.yrel, __ATOMIC_RELAXED);
        if (abs(ev->motion.xrel) + abs(ev->motion.yrel) > 3)   /* not a nudge of the desk */
            set_kbm(1);
        break;
    case SDL_CONTROLLERBUTTONDOWN:
        set_kbm(0);
        __atomic_store_n(&g_pad_last, (int)ev->cbutton.which, __ATOMIC_RELEASE);
        break;
    case SDL_CONTROLLERAXISMOTION:
        if (abs(ev->caxis.value) > 12000) {
            set_kbm(0);
            __atomic_store_n(&g_pad_last, (int)ev->caxis.which, __ATOMIC_RELEASE);
        }
        break;
    }
    return 0;
}

int xbox_InputKeyboardActive(void)
{
    int v = __atomic_load_n(&g_kbm_active, __ATOMIC_ACQUIRE);
#ifdef __ANDROID__
    return v > 0;   /* (no controller there is the touch controls, a pad: the keyboard once a key is pressed) */
#else
    return v < 0 ? !xbox_InputControllerPresent() : v;
#endif
}

/* The touch controls were used (a title's own, on Android): the pad's prompts again, as a controller's would */
void xbox_InputTouchUsed(void)
{
    if (__atomic_load_n(&g_kbm_active, __ATOMIC_ACQUIRE) != 0)
        set_kbm(0);
}

/* The pad in use (last touched; before any, the first connected) is a PlayStation one -- a DualShock 3 or 4,
 * a DualSense, or an adapter that says so -- for its button prompts. */
int xbox_InputPadPlayStation(void)
{
    int id = __atomic_load_n(&g_pad_last, __ATOMIC_ACQUIRE);
    for (int s = 0; s < XBOX_MAX_CONTROLLERS; s++)
        if (g_pad_id[s] >= 0 && (id < 0 || g_pad_id[s] == id)) {
            int t = g_pad_type[s];
            return t == SDL_CONTROLLER_TYPE_PS3 || t == SDL_CONTROLLER_TYPE_PS4 || t == SDL_CONTROLLER_TYPE_PS5;
        }
    return 0;
}

/* The pad in use is a Nintendo one: a Switch Pro Controller, Joy-Cons, or an adapter that says so (#170). */
int xbox_InputPadNintendo(void)
{
    int id = __atomic_load_n(&g_pad_last, __ATOMIC_ACQUIRE);
    for (int s = 0; s < XBOX_MAX_CONTROLLERS; s++)
        if (g_pad_id[s] >= 0 && (id < 0 || g_pad_id[s] == id)) {
            int t = g_pad_type[s];
            return t == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO || t == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT ||
                   t == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT ||
                   t == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR;
        }
    return 0;
}

/* The mouse's motion as a stick: its speed over the last ~10 ms, filtered,
 * at full deflection from about `full` pixels a second. Every caller sees
 * the same value (the state is polled from several places). */
void xbox_InputMouseStick(float full, SHORT *x, SHORT *y)
{
    static float vx, vy;
    static Uint64 last;
    static SDL_SpinLock lock;
    Uint64 now = SDL_GetTicks64();
    SDL_AtomicLock(&lock);
    if (now - last >= 10) {
        float dt = last ? (float)(now - last) / 1000.0f : 0.01f;
        if (dt > 0.1f)
            dt = 0.1f;
        int dx = __atomic_exchange_n(&g_mouse_dx, 0, __ATOMIC_RELAXED);
        int dy = __atomic_exchange_n(&g_mouse_dy, 0, __ATOMIC_RELAXED);
        vx = vx * 0.5f + (dx / dt) * 0.5f;
        vy = vy * 0.5f + (dy / dt) * 0.5f;
        last = now;
    }
    float sx = vx / full, sy = -vy / full;           /* Xbox y points up */
    SDL_AtomicUnlock(&lock);
    sx = sx > 1 ? 1 : sx < -1 ? -1 : sx;
    sy = sy > 1 ? 1 : sy < -1 ? -1 : sy;
    *x = (SHORT)(sx * 32767);
    *y = (SHORT)(sy * 32767);
}

/* Must run on the thread that pumps SDL events (the window's). */
void xbox_InputInit(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    g_pad_lock = SDL_CreateMutex();
    g_labels_set = __atomic_load_n(&g_labels, __ATOMIC_ACQUIRE);
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, g_labels_set ? "1" : "0");
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) && !g_labels_set)
        SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);   /* (the settings screen's, with SDL's default: by label) */
    if (!SDL_WasInit(SDL_INIT_GAMECONTROLLER))
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    vpad_attach();
    open_controllers();
    SDL_AddEventWatch(pad_event_watch, NULL);
}

/* ---- keyboard ----------------------------------------------------------
 *
 * Port 0 is the keyboard and mouse while they are the device in use (the
 * last touched: xbox_InputKeyboardActive), so the game is playable on a
 * desktop without setup. Keys only arrive while the game window has the
 * focus (SDL clears the state otherwise). RECOMP_KEYBOARD=0 turns it off;
 * =1 merges it on top of a connected pad. A title may lay the keys out
 * itself (xbox_keyboard_map, weak); else:
 *
 *   W A S D      left stick          arrows      d-pad
 *   J K L I      X A B Y             Enter       START
 *   Space        A                   Backspace   BACK
 *   Shift (L/R)  right trigger       Q / E       left trigger / black
 *   R            white               numpad 8456 right stick
 *   F / G        left / right thumb click
 */
static int keyboard_mode(void)
{
    static int mode = -1;
    if (mode < 0) {
        const char *v = getenv("RECOMP_KEYBOARD");
        mode = !v || !*v ? 2 : (*v == '0' ? 0 : 1);   /* 2 = when no pad */
    }
    return mode;
}

static SHORT key_axis(const Uint8 *k, SDL_Scancode neg, SDL_Scancode pos)
{
    int v = 0;
    if (k[neg]) v -= 32767;
    if (k[pos]) v += 32767;
    return (SHORT)v;
}

/* A title's own layout: fill st from the keys (k) and mouse buttons
 * (SDL_BUTTON masks); nonzero if it did. */
__attribute__((weak)) int xbox_keyboard_map(const Uint8 *k, Uint32 mouse, XBOX_INPUT_STATE *st)
{
    (void)k; (void)mouse; (void)st;
    return 0;
}

static int keyboard_state(XBOX_INPUT_STATE *st)
{
    static DWORD packet;
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    WORD b = 0;
    int any = 0;

    memset(st, 0, sizeof(*st));
    if (!k)
        return 0;
    if (xbox_keyboard_map(k, SDL_GetMouseState(NULL, NULL), st)) {
        st->dwPacketNumber = ++packet;
        return 1;
    }
    if (k[SDL_SCANCODE_UP])        b |= XBOX_GAMEPAD_DPAD_UP;
    if (k[SDL_SCANCODE_DOWN])      b |= XBOX_GAMEPAD_DPAD_DOWN;
    if (k[SDL_SCANCODE_LEFT])      b |= XBOX_GAMEPAD_DPAD_LEFT;
    if (k[SDL_SCANCODE_RIGHT])     b |= XBOX_GAMEPAD_DPAD_RIGHT;
    if (k[SDL_SCANCODE_RETURN])    b |= XBOX_GAMEPAD_START;
    if (k[SDL_SCANCODE_BACKSPACE]) b |= XBOX_GAMEPAD_BACK;
    if (k[SDL_SCANCODE_F])         b |= XBOX_GAMEPAD_LEFT_THUMB;
    if (k[SDL_SCANCODE_G])         b |= XBOX_GAMEPAD_RIGHT_THUMB;
    st->Gamepad.wButtons = b;

    st->Gamepad.bAnalogButtons[XBOX_BUTTON_A] =
        (k[SDL_SCANCODE_K] || k[SDL_SCANCODE_SPACE]) ? 255 : 0;
    st->Gamepad.bAnalogButtons[XBOX_BUTTON_X] = k[SDL_SCANCODE_J] ? 255 : 0;
    st->Gamepad.bAnalogButtons[XBOX_BUTTON_B] = k[SDL_SCANCODE_L] ? 255 : 0;
    st->Gamepad.bAnalogButtons[XBOX_BUTTON_Y] = k[SDL_SCANCODE_I] ? 255 : 0;
    st->Gamepad.bAnalogButtons[XBOX_BUTTON_WHITE] = k[SDL_SCANCODE_R] ? 255 : 0;
    st->Gamepad.bAnalogButtons[XBOX_BUTTON_BLACK] = k[SDL_SCANCODE_E] ? 255 : 0;
    st->Gamepad.bAnalogButtons[XBOX_BUTTON_LTRIGGER] = k[SDL_SCANCODE_Q] ? 255 : 0;
    st->Gamepad.bAnalogButtons[XBOX_BUTTON_RTRIGGER] =
        (k[SDL_SCANCODE_LSHIFT] || k[SDL_SCANCODE_RSHIFT]) ? 255 : 0;

    st->Gamepad.sThumbLX = key_axis(k, SDL_SCANCODE_A, SDL_SCANCODE_D);
    st->Gamepad.sThumbLY = key_axis(k, SDL_SCANCODE_S, SDL_SCANCODE_W);
    st->Gamepad.sThumbRX = key_axis(k, SDL_SCANCODE_KP_4, SDL_SCANCODE_KP_6);
    st->Gamepad.sThumbRY = key_axis(k, SDL_SCANCODE_KP_5, SDL_SCANCODE_KP_8);

    any = b != 0 || st->Gamepad.sThumbLX || st->Gamepad.sThumbLY ||
          st->Gamepad.sThumbRX || st->Gamepad.sThumbRY;
    for (int i = 0; i < 8; i++)
        any |= st->Gamepad.bAnalogButtons[i] != 0;
    st->dwPacketNumber = ++packet;
    return any;
}

/* The title may move the controller's buttons to other actions (its
 * controls settings): each read of a controller, before the keyboard is
 * merged. Weak: by default as they are. */
__attribute__((weak)) void xbox_pad_remap(XBOX_INPUT_STATE *st, DWORD port)
{
    (void)st; (void)port;
}

static DWORD input_get_state(DWORD dwPort, XBOX_INPUT_STATE *pState, int raw)
{
    if (dwPort >= XBOX_MAX_CONTROLLERS || !pState)
        return ERROR_DEVICE_NOT_CONNECTED;

    SDL_GameController *c = g_pad_lock ? g_pads[dwPort] : NULL;
    if (!c || !SDL_GameControllerGetAttached(c)) {
        g_controller_connected[dwPort] = FALSE;
        if (!raw && dwPort == 0 && keyboard_mode() && g_pad_lock) {
            keyboard_state(pState);
            return ERROR_SUCCESS;
        }
        return ERROR_DEVICE_NOT_CONNECTED;
    }
    g_controller_connected[dwPort] = TRUE;
    if (!raw && dwPort == 0 && keyboard_mode() == 2 && g_pad_lock && xbox_InputKeyboardActive()) {
        keyboard_state(pState);                     /* the keyboard and mouse are in use */
        return ERROR_SUCCESS;
    }
    SDL_LockMutex(g_pad_lock);
    c = g_pads[dwPort];
    if (!c) {
        SDL_UnlockMutex(g_pad_lock);
        return ERROR_DEVICE_NOT_CONNECTED;
    }

    g_controller_connected[dwPort] = TRUE;

    memset(pState, 0, sizeof(XBOX_INPUT_STATE));
    pState->dwPacketNumber = ++g_packet[dwPort];

    WORD btn = 0;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_UP))    btn |= XBOX_GAMEPAD_DPAD_UP;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_DOWN))  btn |= XBOX_GAMEPAD_DPAD_DOWN;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_LEFT))  btn |= XBOX_GAMEPAD_DPAD_LEFT;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) btn |= XBOX_GAMEPAD_DPAD_RIGHT;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_START))      btn |= XBOX_GAMEPAD_START;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_BACK))       btn |= XBOX_GAMEPAD_BACK;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_LEFTSTICK))  btn |= XBOX_GAMEPAD_LEFT_THUMB;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_RIGHTSTICK)) btn |= XBOX_GAMEPAD_RIGHT_THUMB;
    pState->Gamepad.wButtons = btn;

    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_A] =
        SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_A) ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_B] =
        SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_B) ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_X] =
        SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_X) ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_Y] =
        SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_Y) ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_BLACK] =
        SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ? 255 : 0;
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_WHITE] =
        SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ? 255 : 0;

    /* SDL trigger axes are 0..32767 -> Xbox analog button 0..255 */
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_LTRIGGER] =
        (BYTE)(SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >> 7);
    pState->Gamepad.bAnalogButtons[XBOX_BUTTON_RTRIGGER] =
        (BYTE)(SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) >> 7);

    /* SDL Y axis points down; the Xbox Y axis points up -- invert.
     * Use (-1 - v) so v = -32768 does not overflow SHORT. */
    pState->Gamepad.sThumbLX = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX);
    pState->Gamepad.sThumbLY =
        (SHORT)(-1 - SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY));
    pState->Gamepad.sThumbRX = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTX);
    pState->Gamepad.sThumbRY =
        (SHORT)(-1 - SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTY));
    SDL_UnlockMutex(g_pad_lock);
    if (!raw) xbox_pad_remap(pState, dwPort);   /* the title's button bindings, if it has any */
    if (!raw && g_vpad && dwPort == 0) {         /* (the test pad's: what the title is handed, as it changes) */
        static unsigned last;
        unsigned now = pState->Gamepad.wButtons;
        for (int i = 0; i < 8; i++)
            now |= (pState->Gamepad.bAnalogButtons[i] > 30u) << (16 + i);
        if (now != last)
            fprintf(stderr, "  PAD: title gets %04X A%d B%d X%d Y%d Bk%d Wh%d LT%d RT%d\n", now & 0xFFFF,
                    (now >> 16) & 1, (now >> 17) & 1, (now >> 18) & 1, (now >> 19) & 1, (now >> 20) & 1,
                    (now >> 21) & 1, (now >> 22) & 1, (now >> 23) & 1);
        last = now;
    }

    /* RECOMP_KEYBOARD=1: the keyboard drives port 0 alongside the pad. */
    if (!raw && dwPort == 0 && keyboard_mode() == 1) {
        XBOX_INPUT_STATE kb;
        if (keyboard_state(&kb)) {
            pState->Gamepad.wButtons |= kb.Gamepad.wButtons;
            for (int i = 0; i < 8; i++)
                if (kb.Gamepad.bAnalogButtons[i])
                    pState->Gamepad.bAnalogButtons[i] = kb.Gamepad.bAnalogButtons[i];
            if (kb.Gamepad.sThumbLX) pState->Gamepad.sThumbLX = kb.Gamepad.sThumbLX;
            if (kb.Gamepad.sThumbLY) pState->Gamepad.sThumbLY = kb.Gamepad.sThumbLY;
            if (kb.Gamepad.sThumbRX) pState->Gamepad.sThumbRX = kb.Gamepad.sThumbRX;
            if (kb.Gamepad.sThumbRY) pState->Gamepad.sThumbRY = kb.Gamepad.sThumbRY;
        }
    }
    return ERROR_SUCCESS;
}

int xbox_InputControllerPresent(void)
{
    for (int i = 0; i < XBOX_MAX_CONTROLLERS; i++)
        if (g_pads[i])
            return 1;
    return 0;
}

DWORD xbox_InputSetState(DWORD dwPort, const XBOX_VIBRATION *pVibration)
{
    if (dwPort >= XBOX_MAX_CONTROLLERS || !pVibration)
        return ERROR_DEVICE_NOT_CONNECTED;

    if (!g_pad_lock)
        return ERROR_DEVICE_NOT_CONNECTED;
    SDL_LockMutex(g_pad_lock);
    SDL_GameController *c = g_pads[dwPort];
    /* SDL rumble needs a duration; refresh for ~1s on each call (the game
     * polls vibration continuously). */
    if (c)
        SDL_GameControllerRumble(c, pVibration->wLeftMotorSpeed,
                                 pVibration->wRightMotorSpeed, 1000);
    SDL_UnlockMutex(g_pad_lock);
    return c ? ERROR_SUCCESS : ERROR_DEVICE_NOT_CONNECTED;
}

BOOL xbox_InputIsConnected(DWORD dwPort)
{
    if (dwPort >= XBOX_MAX_CONTROLLERS) return FALSE;
    return g_controller_connected[dwPort];
}

DWORD xbox_InputGetCapabilities(DWORD dwPort, DWORD dwFlags, XBOX_INPUT_CAPABILITIES *pCaps)
{
    (void)dwFlags;
    if (dwPort >= XBOX_MAX_CONTROLLERS || !pCaps)
        return ERROR_DEVICE_NOT_CONNECTED;
    if (!g_pads[dwPort])
        return ERROR_DEVICE_NOT_CONNECTED;

    memset(pCaps, 0, sizeof(XBOX_INPUT_CAPABILITIES));
    pCaps->Type    = 1;   /* XINPUT_DEVTYPE_GAMEPAD */
    pCaps->SubType = 1;   /* XINPUT_DEVSUBTYPE_GAMEPAD */
    pCaps->Flags   = 0;
    return ERROR_SUCCESS;
}

#endif /* _WIN32 */

DWORD xbox_InputGetState(DWORD port, XBOX_INPUT_STATE *state)
{
    return input_get_state(port, state, 0);
}
DWORD xbox_InputGetStateRaw(DWORD port, XBOX_INPUT_STATE *state)
{
    return input_get_state(port, state, 1);
}
