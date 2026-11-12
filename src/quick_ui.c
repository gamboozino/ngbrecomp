/* Black's own inventory pictures, read from the player's spr.afs at runtime.
 * The presenter owns input and composition; it never reads guest memory. */
#include "input/xinput_xbox.h"
#include "launcher_font.h"
#include "quick_select.h"
#include "bindings.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int xbox_mod_file(const char *rel, char *out, size_t n);   /* xboxrecomp kernel_path.c: a mod in a game file's place */

int ngb_pc_setting(int which);
void ngb_pc_set_setting(int which, int value);   /* src/ingame_menu.c */

static SDL_SpinLock s_lock;
static NgbQuickMenu s_menu;
static int s_open, s_command, s_block;
static int s_navigation;
static int s_renderer_ready;
static uint16_t s_command_item;
static int s_command_category;
/* Only the SDL event thread reads physical inputs. The guest and presenter
 * consume snapshots; neither depends on a gameplay USB poll while paused. */
static Uint8 s_keys[SDL_NUM_SCANCODES];
static unsigned s_mouse;
static unsigned s_held, s_edges, s_request;
static int s_confirm_ready;
static Uint64 s_request_at;
static int s_request_step;      /* opened by Left / Right: a card that way at once (-1, 1) */
static uint64_t s_repeat_at;
/* The card last on in each category, where the menu opens next (0xFFFF none yet) */
static uint16_t s_remember[NGB_QUICK_CATEGORIES] = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
static uint32_t s_repeat;
static char s_game_dir[4096];
static char s_message[64];
static uint64_t s_message_until;
static unsigned s_revision;

void ngb_quick_set_game_dir(const char *path) { snprintf(s_game_dir, sizeof s_game_dir, "%s", path); }

/* Hidden regression runs use an SDL virtual controller and window events.
 * This exercises the real backend, without sending input to the desktop. */
static void test_input(void)
{
    static const char *path;
    static int checked;
    if (!checked) {
        checked = 1;
        path = getenv("NGB_TEST_HIDDEN") ? getenv("NGB_TEST_INPUT") : NULL;
    }
    if (!path)
        return;
    static SDL_Joystick *pad;
    static long offset;
    if (!pad)
    {
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        int device = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX,
                                               SDL_CONTROLLER_BUTTON_MAX, 0);
        if (device < 0)
            return;
        pad = SDL_JoystickOpen(device);
        fprintf(stderr, "[QUICK INPUT TEST] virtual controller: %s\n", pad ? "ready" : SDL_GetError());
    }
    FILE *f = fopen(path, "rb");
    if (!f)
        return;
    fseek(f, offset, SEEK_SET);
    char line[100], type;
    int input, down;
    while (fgets(line, sizeof line, f))
    {
        if (!strchr(line, '\n'))
            break;
        offset = ftell(f);
        if (sscanf(line, " %c %d %d", &type, &input, &down) != 3)
            continue;
        fprintf(stderr, "[QUICK INPUT TEST] %c %d %d at %llu\n", type, input, down,
                (unsigned long long)SDL_GetTicks64());
        if (type == 'p' && pad)
            SDL_JoystickSetVirtualButton(pad, input, down != 0);
        if (type == 'a' && pad && input >= 0 && input < SDL_CONTROLLER_AXIS_MAX)
            SDL_JoystickSetVirtualAxis(pad, input, (Sint16)down);
        if (type == 'k' && input > 0 && input < SDL_NUM_SCANCODES)
        {
            SDL_Event ev = {0};
            ev.type = down ? SDL_KEYDOWN : SDL_KEYUP;
            ev.key.state = down ? SDL_PRESSED : SDL_RELEASED;
            ev.key.keysym.scancode = input;
            ev.key.keysym.sym = SDL_GetKeyFromScancode(input);
            SDL_PushEvent(&ev);
        }
    }
    fclose(f);
    SDL_JoystickUpdate();
}

void ngb_quick_poll(void)
{
    test_input();
    unsigned bits = 0;
    for (unsigned p = 0; p < 4; p++)
    {
        XBOX_INPUT_STATE pad;
        int connected = xbox_InputGetStateRaw(p, &pad) == 0;
        bits |= ngb_bind_quick_state(s_keys, s_mouse, connected ? &pad : NULL);
    }
    {   /* Toggle HUD (Controls; nothing bound by default): each new press flips Display & Audio > HUD */
        static unsigned hud_held;
        unsigned hud = bits & NGB_BIND_HUD_BIT;
        if (hud && !hud_held && !ngb_bind_capturing())
            ngb_pc_set_setting(24, !ngb_pc_setting(24));
        hud_held = hud;
        bits &= ~(unsigned)NGB_BIND_HUD_BIT;
    }
    SDL_AtomicLock(&s_lock);
    if (!ngb_pc_setting(20))
    {
        /* Off must release modal input, including the held-button block.
         * Track physical holds so re-enabling cannot queue a hidden opener. */
        s_open = s_block = s_command = s_request = s_edges = s_navigation = 0;
        s_repeat = 0;
        s_held = bits;
        SDL_AtomicUnlock(&s_lock);
        return;
    }
    unsigned edge = bits & ~s_held;
    if (s_open)
    {
        s_edges |= edge;
        if (!(bits & (Q_CONFIRM | Q_CANCEL)))
            s_confirm_ready = 1;
    }
    else if (!s_block && (edge & (Q_UP | Q_DOWN | Q_LEFT | Q_RIGHT)))
    {
        /* Up the melee weapons, Down the items; Left / Right the melee weapons, a card that way */
        s_request = edge & Q_DOWN ? NGB_QUICK_ITEMS : NGB_QUICK_MELEE;
        s_request_step = edge & (Q_UP | Q_DOWN) ? 0 : edge & Q_LEFT ? -1 : 1;
        s_request_at = SDL_GetTicks64();
    }
    s_held = bits;
    if (!s_open && s_block && !s_held)
        s_block = 0;
    SDL_AtomicUnlock(&s_lock);
}

int ngb_quick_take_open(void)
{
    SDL_AtomicLock(&s_lock);
    unsigned category = SDL_GetTicks64() - s_request_at < 250 ? s_request : 0;
    s_request = 0;
    if (!category)
        s_request_step = 0;                        /* a stale Left / Right does not move a later opening */
    SDL_AtomicUnlock(&s_lock);
    return category;
}

int ngb_quick_controls_active(void)
{
    if (!ngb_pc_setting(20)) return 0;
    SDL_AtomicLock(&s_lock);
    int active = s_open || s_block;
    SDL_AtomicUnlock(&s_lock);
    return active;
}
int ngb_quick_available(void)
{
    SDL_AtomicLock(&s_lock);
    int ready = s_renderer_ready;
    SDL_AtomicUnlock(&s_lock);
    return ready;
}

int ngb_quick_event(const void *event)
{
    const SDL_Event *ev = event;
    if (ev->type == SDL_KEYDOWN || ev->type == SDL_KEYUP)
    {
        unsigned sc = ev->key.keysym.scancode;
        if (sc < SDL_NUM_SCANCODES)
            s_keys[sc] = ev->type == SDL_KEYDOWN;
    }
    if (ev->type == SDL_MOUSEBUTTONDOWN)
        s_mouse |= SDL_BUTTON(ev->button.button);
    if (ev->type == SDL_MOUSEBUTTONUP)
        s_mouse &= ~SDL_BUTTON(ev->button.button);
    if (ev->type == SDL_WINDOWEVENT && ev->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
    {
        memset(s_keys, 0, sizeof s_keys);
        s_mouse = 0;
    }
    SDL_AtomicLock(&s_lock);
    int used = s_open && ngb_pc_setting(20);
    if (used && ev->type == SDL_KEYDOWN && ev->key.keysym.sym == SDLK_F1)
    {
        s_open = 0;
        used = 0;
    }
    SDL_AtomicUnlock(&s_lock);
    return used;
}

void ngb_quick_open(const NgbQuickMenu *menu)
{
    SDL_AtomicLock(&s_lock);
    s_menu = *menu;
    if (s_menu.category < 1 || s_menu.category > NGB_QUICK_CATEGORIES)
        s_menu.category = NGB_QUICK_MELEE;
    s_edges = s_request = 0;
    s_navigation = 0;
    s_confirm_ready = 0;
    s_repeat = 0;
    s_message[0] = 0;
    s_revision++;
    s_open = 1;
    s_block = 1;
    for (int c = 0; c < NGB_QUICK_CATEGORIES; c++) {   /* each category on the card it was left on */
        NgbQuickView *v = &s_menu.views[c];
        for (int i = 0; i < v->count; i++)
            if (v->entries[i].id == s_remember[c])
                v->index = i;
    }
    int step = s_request_step;
    s_request_step = 0;
    NgbQuickView *opened = &s_menu.views[s_menu.category - 1];
    if (step && opened->count > 1) {                  /* opened by Left / Right: that card at once, */
        opened->index = (opened->index + step + opened->count) % opened->count;
        s_remember[s_menu.category - 1] = opened->entries[opened->index].id;
        s_navigation++;                                /* its sound, */
        s_repeat = step < 0 ? Q_LEFT : Q_RIGHT;        /* and held, on as inside the menu */
        s_repeat_at = SDL_GetTicks64() + 380;
    }
    int category = s_menu.category;
    SDL_AtomicUnlock(&s_lock);
    const NgbQuickView *view = &menu->views[category - 1];
    fprintf(stderr, "[QUICK] menu open category=%d count=%d pause=%d ready=%d%s\n", view->category,
            view->count, view->freeze, menu->ready, step ? step < 0 ? " (Left: the card before)" : " (Right: the next card)" : "");
}

void ngb_quick_close(void)
{
    SDL_AtomicLock(&s_lock);
    s_open = 0;
    s_command = 0;
    s_navigation = 0;
    SDL_AtomicUnlock(&s_lock);
}

int ngb_quick_category(void)
{
    SDL_AtomicLock(&s_lock);
    int category = s_open ? s_menu.category : 0;
    SDL_AtomicUnlock(&s_lock);
    return category;
}

void ngb_quick_refresh(const NgbQuickMenu *menu)
{
    SDL_AtomicLock(&s_lock);
    if (s_open && s_menu.ready != menu->ready)
    {
        s_menu.ready = menu->ready;
        /* A press during the unfinished action cannot become a category
         * change or confirmation as soon as that action ends. */
        s_edges &= ~(Q_UP | Q_DOWN | Q_CONFIRM);
        s_repeat = 0;
        s_command = 0;
        s_confirm_ready = !(s_held & (Q_CONFIRM | Q_CANCEL));
        s_revision++;
        fprintf(stderr, "[QUICK] action ready=%d\n", menu->ready);
    }
    for (int c = 0; s_open && c < NGB_QUICK_CATEGORIES; c++)
    {
        NgbQuickView *old = &s_menu.views[c];
        const NgbQuickView *view = &menu->views[c];
        uint16_t selected = old->count ? old->entries[old->index].id : 0xFFFF;
        int changed = old->count != view->count || old->freeze != view->freeze;
        for (int i = 0; i < view->count; i++)
        {
            if (i >= old->count || memcmp(&old->entries[i], &view->entries[i], sizeof view->entries[i]))
                changed = 1;
        }
        if (changed)
        {
            *old = *view;
            for (int i = 0; i < old->count; i++)
                if (old->entries[i].id == selected)
                    old->index = i;
            s_revision++;
        }
    }
    SDL_AtomicUnlock(&s_lock);
}

int ngb_quick_wanted(void)
{
    SDL_AtomicLock(&s_lock);
    int open = s_open;
    SDL_AtomicUnlock(&s_lock);
    return open;
}

int ngb_quick_frozen(void)
{
    if (!ngb_pc_setting(20)) return 0;
    SDL_AtomicLock(&s_lock);
    int freeze = s_open && s_menu.ready && s_menu.views[s_menu.category - 1].freeze;
    SDL_AtomicUnlock(&s_lock);
    return freeze;
}

int ngb_quick_take_command(uint16_t *item, int *category)
{
    SDL_AtomicLock(&s_lock);
    int ready = s_command;
    if (ready)
    {
        *item = s_command_item;
        *category = s_command_category;
        s_command = 0;
    }
    SDL_AtomicUnlock(&s_lock);
    return ready;
}

int ngb_quick_take_navigation(void)
{
    SDL_AtomicLock(&s_lock);
    int steps = s_navigation;
    s_navigation = 0;
    SDL_AtomicUnlock(&s_lock);
    return steps;
}

static uint32_t u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

typedef struct
{
    char name[24];
    uint32_t *px;
    int w, h, x0, y0, x1, y1;
} Icon;
static Icon s_icons[64];
static int s_icon_count;

static unsigned spread(unsigned v, unsigned mask)
{
    unsigned r = 0;
    for (unsigned bit = 1; mask; bit <<= 1)
    {
        unsigned low = mask & -mask;
        if (v & bit)
            r |= low;
        mask &= mask - 1;
    }
    return r;
}

/* XPR0 .spr packages contain sprite definitions and Xbox texture resources.
 * Inventory pictures use one image; Costume Selection uses several. */
static Icon *icon(const char *name)
{
    if (!*name)
        return NULL;
    /* "archive/file#sprite": another archive than spr.afs (the HUD's game.spr is in demo.afs) */
    char archive[16] = "spr.afs", filename[32];
    const char *slash = strchr(name, '/');
    if (slash && slash - name < (int)sizeof archive) {
        snprintf(archive, sizeof archive, "%.*s", (int)(slash - name), name);
        snprintf(filename, sizeof filename, "%s", slash + 1);
    } else
        snprintf(filename, sizeof filename, "%s", name);
    char *part = strchr(filename, '#');
    unsigned sprite = part ? (unsigned)strtoul(part + 1, NULL, 10) : 0;
    if (part)
        *part = 0;
    for (int i = 0; i < s_icon_count; i++)
        if (!strcmp(s_icons[i].name, name))
            return &s_icons[i];
    if (s_icon_count == 64)
        return NULL;
    Icon *it = &s_icons[s_icon_count++];
    snprintf(it->name, sizeof it->name, "%s", name);
    char path[4200];
    FILE *f = xbox_mod_file(archive, path, sizeof path) ? fopen(path, "rb") : NULL;   /* (a mod's, as the game
                                                                                         * reads it: #179) */
    if (!f) {
        snprintf(path, sizeof path, "%s/%s", s_game_dir, archive);
        f = fopen(path, "rb");
    }
    if (!f)
        return it;
    unsigned char header[8];
    if (fread(header, 1, 8, f) != 8 || memcmp(header, "AFS\0", 4))
    {
        fclose(f);
        return it;
    }
    unsigned n = u32(header + 4);
    if (!n || n > 4096)
    {
        fclose(f);
        return it;
    }
    unsigned char *table = malloc((n + 1u) * 8u);
    unsigned char *names = malloc(n * 48u);
    if (!table || !names)
        goto done;
    if (fread(table, 8, n + 1u, f) != n + 1u)
        goto done;
    unsigned noff = u32(table + n * 8u), nsize = u32(table + n * 8u + 4);
    if (nsize < n * 48u || fseek(f, noff, SEEK_SET) || fread(names, 48, n, f) != n)
        goto done;
    for (unsigned i = 0; i < n; i++)
    {
        if (strncmp((char *)names + i * 48u, filename, 32))
            continue;
        unsigned off = u32(table + i * 8u), size = u32(table + i * 8u + 4);
        if (size < 0x80 || size > 0x400000)
            break;
        unsigned char *b = malloc(size);
        if (!b)
            break;
        if (fseek(f, off, SEEK_SET) || fread(b, 1, size, f) != size || memcmp(b, "XPR0", 4))
        {
            free(b);
            break;
        }
        unsigned data = u32(b + 8), definitions = u32(b + 0x10);
        unsigned resource = 0x14u + definitions;
        if (definitions % 24 || sprite >= definitions / 24 || (uint64_t)definitions + 0x14 > data ||
            data > size)
        {
            free(b);
            break;
        }
        unsigned texture = u32(b + 0x14 + sprite * 24 + 16);
        for (unsigned t = 0; t < texture && resource + 20 <= data; t++)
        {
            resource += 20;
            if (resource + 12 <= data && (u32(b + resource) >> 16 & 7) == 3)
                resource += 12;
        }
        if (resource + 32 > data || (u32(b + resource) >> 16 & 7) != 4 ||
            (u32(b + resource + 20) >> 16 & 7) != 3)
        {
            free(b);
            break;
        }
        unsigned fmt = u32(b + resource + 12);
        unsigned lw = fmt >> 20 & 15, lh = fmt >> 24 & 15, kind = fmt >> 8 & 255;
        unsigned toff = data + u32(b + resource + 4), poff = data + u32(b + resource + 24);
        if (kind != 0x0B || lw > 10 || lh > 10 || data >= size || (uint64_t)toff + (1u << (lw + lh)) > size ||
            (uint64_t)poff + 1024 > size)
        {
            free(b);
            break;
        }
        it->w = 1 << lw;
        it->h = 1 << lh;
        it->px = malloc((size_t)it->w * it->h * 4);
        if (!it->px)
        {
            free(b);
            break;
        }
        unsigned mx = 0, my = 0, mask = 1;
        for (unsigned bit = 1; bit < (unsigned)it->w || bit < (unsigned)it->h; bit <<= 1)
        {
            if (bit < (unsigned)it->w)
            {
                mx |= mask;
                mask <<= 1;
            }
            if (bit < (unsigned)it->h)
            {
                my |= mask;
                mask <<= 1;
            }
        }
        it->x0 = it->w;
        it->y0 = it->h;
        for (int y = 0; y < it->h; y++)
            for (int x = 0; x < it->w; x++)
            {
                unsigned at = spread((unsigned)x, mx) | spread((unsigned)y, my);
                uint32_t c = u32(b + poff + b[toff + at] * 4u);
                it->px[y * it->w + x] = c;
                if ((c >> 24) > 16)
                {
                    if (x < it->x0)
                        it->x0 = x;
                    if (y < it->y0)
                        it->y0 = y;
                    if (x + 1 > it->x1)
                        it->x1 = x + 1;
                    if (y + 1 > it->y1)
                        it->y1 = y + 1;
                }
            }
        free(b);
        fprintf(stderr, "[QUICK] Black asset %s %dx%d bounds=%d,%d,%d,%d\n", name, it->w, it->h, it->x0,
                it->y0, it->x1, it->y1);
        break;
    }
done:
    free(table);
    free(names);
    fclose(f);
    return it;
}

static uint32_t *s_px;
static int s_w, s_h;
static void blend(int x, int y, uint32_t c, unsigned a)
{
    if (x < 0 || y < 0 || x >= s_w || y >= s_h || !a)
        return;
    uint32_t d = s_px[y * s_w + x];
    unsigned r = (((c >> 16) & 255) * a + ((d >> 16) & 255) * (255 - a)) / 255;
    unsigned g = (((c >> 8) & 255) * a + ((d >> 8) & 255) * (255 - a)) / 255;
    unsigned b = ((c & 255) * a + (d & 255) * (255 - a)) / 255;
    unsigned opacity = a + (d >> 24) * (255 - a) / 255;
    s_px[y * s_w + x] = opacity << 24 | r << 16 | g << 8 | b;
}

static void art(Icon *it, float cx, float cy, float size, unsigned alpha)
{
    if (!it || !it->px || it->x1 <= it->x0 || it->y1 <= it->y0)
        return;
    /* Visible bounds set the icon's size and centre, but are not a scissor.
     * Black's faint bottle glow extends beyond that opacity threshold. Draw
     * the full texture so the halo keeps its original transparent falloff. */
    float scale = size / (float)((it->x1 - it->x0) > (it->y1 - it->y0) ? it->x1 - it->x0 : it->y1 - it->y0);
    int w = (int)(it->w * scale + 0.5f), h = (int)(it->h * scale + 0.5f);
    if (w <= 0 || h <= 0)
        return;
    int left = (int)(cx - (it->x0 + it->x1) * 0.5f * scale);
    int top = (int)(cy - (it->y0 + it->y1) * 0.5f * scale);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            uint32_t c = it->px[(y * it->h / h) * it->w + x * it->w / w];
            blend(left + x, top + y, c, (c >> 24) * alpha / 255);
        }
}

/* Frames may stretch independently on each axis. Draw their complete native
 * texture, positioning the visible bounds inside the requested rectangle. */
static void art_rect(Icon *it, float cx, float cy, float width, float height, unsigned alpha)
{
    if (!it || !it->px || it->x1 <= it->x0 || it->y1 <= it->y0) return;
    float sx = width / (it->x1 - it->x0), sy = height / (it->y1 - it->y0);
    int w = (int)(it->w * sx + 0.5f), h = (int)(it->h * sy + 0.5f);
    int left = (int)(cx - (it->x0 + it->x1) * 0.5f * sx);
    int top = (int)(cy - (it->y0 + it->y1) * 0.5f * sy);
    if (w < 1 || h < 1) return;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
        {
            uint32_t c = it->px[(y * it->h / h) * it->w + x * it->w / w];
            blend(left + x, top + y, c, (c >> 24) * alpha / 255);
        }
}
static void art_fit(Icon *it, float cx, float cy, float width, float height, unsigned alpha)
{
    if (!it || !it->px || it->x1 <= it->x0 || it->y1 <= it->y0) return;
    float bw = (float)(it->x1 - it->x0), bh = (float)(it->y1 - it->y0);
    float sx = width / bw, sy = height / bh;
    art(it, cx, cy, (sx < sy ? sx : sy) * (bw > bh ? bw : bh), alpha);
}
/* A bow's arrows (#173), as the HUD shows them when the bow is out: its arrow
 * from game.spr (sprite 80, the dark-fletched 81 for explosive arrows) with the glow at the head that tells them
 * apart -- the orange one (85) for explosive arrows, the blue streak (83) for APFSDS cores. kind: e->badge. */
static void badge(const char *kind, float cx, float cy, int size, unsigned alpha)
{
    float s = (float)size, head = s * 0.28f;   /* the arrow's head is at the bottom left */
    if (!strcmp(kind, "fire"))
        art_fit(icon("demo.afs/game.spr#85"), cx - head, cy + head, s * 0.75f, s * 0.75f, alpha);
    art_fit(icon(!strcmp(kind, "fire") ? "demo.afs/game.spr#81" : "demo.afs/game.spr#80"), cx, cy, s, s, alpha);
    if (!strcmp(kind, "core"))
        art_fit(icon("demo.afs/game.spr#83"), cx - head * 0.6f, cy + head * 0.6f, s * 0.7f, s * 0.7f, alpha);
}
static void panel(float cx, float cy, int width, int height, unsigned alpha)
{
    /* Save/Load's silver panel supplies the etched background. Use its plain
     * right-hand area; the flower emblem belongs to that screen's label. The
     * complete border is Mission Selection's own rectangular frame. */
    Icon *paper = icon("saveload.spr#20");
    if (paper && paper->px && paper->w == 128 && paper->h == 64)
        for (int y = 0; y < height - 4; y++)
            for (int x = 0; x < width - 4; x++)
            {
                uint32_t c = paper->px[(8 + y * 40 / (height - 4)) * paper->w +
                                       62 + x * 52 / (width - 4)];
                /* Two 33% reductions leave the native paper at 44.89%
                 * brightness. Frame and icons are drawn afterward. */
                unsigned r = ((c >> 16) & 255) * 4489 / 10000;
                unsigned g = ((c >> 8) & 255) * 4489 / 10000;
                unsigned b = (c & 255) * 4489 / 10000;
                c = (c & 0xFF000000u) | r << 16 | g << 8 | b;
                blend((int)cx - width / 2 + 2 + x, (int)cy - height / 2 + 2 + y,
                      c, (c >> 24) * alpha / 255);
            }
    art_rect(icon("mis_menu.spr#108"), cx, cy, (float)width, (float)height, alpha);
}

static void text_at(const char *t, int x, int y, int height, uint32_t color)
{
    for (; *t; t++)
    {
        unsigned ch = (unsigned char)*t;
        if (ch < 32 || ch > 126)
            continue;
        unsigned g = ch - 32, gw = launcher_font_w[g];
        int w = (int)(gw * height / 32u);
        for (int yy = 0; yy < height; yy++)
            for (int xx = 0; xx < w; xx++)
            {
                unsigned a = launcher_font_px[launcher_font_off[g] + (unsigned)(yy * 32 / height) * gw +
                                              (unsigned)(xx * (int)gw / w)];
                blend(x + xx + 1, y + yy + 2, 0, a);
                blend(x + xx, y + yy, color, a);
            }
        x += w + 1;
    }
}

/* Deliberately simple navigation arrows; weapon/item pictures are exclusively
 * the decoded Black assets. No confirm/cancel prompt is drawn. */
/* Costume Selection uses sprite 0x75A, entry 11 of cos_menu.spr.
 * Its downward grey arrow is turned a quarter turn for either side. */
static void arrow(int cx, int cy, int direction, int radius)
{
    static Icon rotated[2];
    Icon *source = icon("cos_menu.spr#11");
    if (!source || !source->px)
        return;
    int side = direction > 0;
    Icon *it = &rotated[side];
    if (!it->px)
    {
        it->w = source->h;
        it->h = source->w;
        it->px = malloc((size_t)it->w * it->h * 4);
        if (!it->px)
            return;
        for (int y = 0; y < source->h; y++)
            for (int x = 0; x < source->w; x++)
            {
                int dx = side ? y : source->h - 1 - y;
                int dy = side ? source->w - 1 - x : x;
                it->px[dy * it->w + dx] = source->px[y * source->w + x];
            }
        it->x0 = side ? source->y0 : source->h - source->y1;
        it->x1 = side ? source->y1 : source->h - source->y0;
        it->y0 = side ? source->w - source->x1 : source->x0;
        it->y1 = side ? source->w - source->x0 : source->x1;
    }
    art(it, cx, cy, radius * 2.6f, 255);
}

static void category_arrow(int cx, int cy, int down, unsigned alpha)
{
    static Icon up;
    Icon *source = icon("cos_menu.spr#11");
    if (!source || !source->px) return;
    if (!down && !up.px)
    {
        up = *source;
        up.px = malloc((size_t)up.w * up.h * 4);
        if (!up.px) return;
        for (int y = 0; y < up.h; y++)
            for (int x = 0; x < up.w; x++)
                up.px[(up.h - 1 - y) * up.w + up.w - 1 - x] = source->px[y * up.w + x];
        up.x0 = up.w - source->x1; up.x1 = up.w - source->x0;
        up.y0 = up.h - source->y1; up.y1 = up.h - source->y0;
    }
    art(down ? source : &up, (float)cx, (float)cy, 20, alpha);
}

/* The category above or below; wrap 0: held, it stops at the top or the bottom (the same one back) */
static int category_step(int category, int direction, int wrap)
{
    /* Visual order is independent of the native inventory class numbers. */
    static const int order[] = {NGB_QUICK_NINPO, NGB_QUICK_PROJECTILES,
                                NGB_QUICK_MELEE, NGB_QUICK_ITEMS};
    for (int i = 0; i < NGB_QUICK_CATEGORIES; i++)
        if (order[i] == category) {
            int j = i + direction;
            if (!wrap && (j < 0 || j >= NGB_QUICK_CATEGORIES))
                return category;
            return order[(j + NGB_QUICK_CATEGORIES) % NGB_QUICK_CATEGORIES];
        }
    return NGB_QUICK_MELEE;
}

static void category_panel(int category, const NgbQuickEntry *entry, int cy, unsigned alpha)
{
    static const char *const fallback[] = { "katana.spr", "syuri.spr", "scr_1.spr", "re_hp_s.spr" };
    panel(184, (float)cy, 128, 30, alpha);
    art_fit(icon(entry->art[0] ? entry->art : fallback[category - 1]),
            184, (float)cy, 76, 22, entry->art[0] ? alpha : alpha / 3);
}

int ngb_quick_layer(uint32_t *pixels)
{
    if (!ngb_pc_setting(20)) return 0;
    SDL_AtomicLock(&s_lock);
    s_renderer_ready = 1;
    if (!s_open)
    {
        SDL_AtomicUnlock(&s_lock);
        return 0;
    }
    unsigned bits = s_held, pressed = s_edges;
    s_edges = 0;
    uint64_t now = SDL_GetTicks64();
    NgbQuickView *view = &s_menu.views[s_menu.category - 1];
    uint32_t dir = bits & Q_DIRECTIONS;
    if (!dir || (dir & (dir - 1)))
    {
        s_repeat = 0;
        dir = 0;
    }
    if (dir && ((pressed & dir) || (s_repeat == dir && now >= s_repeat_at)))
    {
        /* A press steps a category and goes round; held, Up goes to the melee weapons and Down to the
         * items -- where each opens the menu -- and stays */
        int next = (dir == Q_UP || dir == Q_DOWN) && s_menu.ready
                 ? (pressed & dir) ? category_step(s_menu.category, dir == Q_UP ? -1 : 1, 1)
                                   : dir == Q_UP ? NGB_QUICK_MELEE : NGB_QUICK_ITEMS : 0;
        if (next && next != s_menu.category)
        {
            s_menu.category = next;
            view = &s_menu.views[s_menu.category - 1];
            s_navigation++;
            s_revision++;
            s_message[0] = 0;
            fprintf(stderr, "[QUICK] category=%d\n", s_menu.category);
        }
        else if ((dir == Q_LEFT || dir == Q_RIGHT) && view->count > 1)
        {
            /* A press steps and goes round; held, it steps on to the first or the last card and
             * stops there, as Up / Down do with the categories */
            int i = view->index + (dir == Q_LEFT ? -1 : 1);
            if (pressed & dir)
                i = (i + view->count) % view->count;
            if (i >= 0 && i < view->count && i != view->index)
            {
                view->index = i;
                s_remember[view->category - 1] = view->entries[view->index].id;
                s_navigation++;
                s_revision++;
                s_message[0] = 0;
                fprintf(stderr, "[QUICK] selected category=%d item=%04X\n", view->category,
                        view->entries[view->index].id);
            }
        }
        if (s_menu.ready || dir == Q_LEFT || dir == Q_RIGHT)
        {
            s_repeat_at = now + (s_repeat == dir ? 110 : 380);
            s_repeat = dir;
        }
    }
    if (s_confirm_ready && (pressed & Q_CANCEL))
    {
        s_open = 0;
        fprintf(stderr, "[QUICK] cancelled category=%d\n", view->category);
    }
    if (s_open && s_menu.ready && s_confirm_ready && (pressed & Q_CONFIRM) && view->count)
    {
        NgbQuickEntry *e = &view->entries[view->index];
        if (e->usable)
        {
            s_command_item = e->id;
            s_command_category = view->category;
            s_command = 1;
            /* The guest acknowledges this only after rechecking the action
             * gate. A live-mode race must leave the menu open and waiting. */
            fprintf(stderr, "[QUICK] requested category=%d item=%04X\n", view->category, e->id);
        }
        else
        {
            snprintf(s_message, sizeof s_message, "Already full");
            s_message_until = now + 1400;
            s_revision++;
        }
    }
    NgbQuickView v = *view;
    int above_category = category_step(s_menu.category, -1, 1);
    int below_category = category_step(s_menu.category, 1, 1);
    const NgbQuickView *above = &s_menu.views[above_category - 1];
    const NgbQuickView *below = &s_menu.views[below_category - 1];
    NgbQuickEntry above_entry = {0}, below_entry = {0};
    if (above->count) above_entry = above->entries[above->index];
    if (below->count) below_entry = below->entries[below->index];
    int ready = s_menu.ready;
    char message[sizeof s_message];
    snprintf(message, sizeof message, "%s", s_message);
    uint64_t message_until = s_message_until;
    int open = s_open;
    unsigned revision = s_revision;
    SDL_AtomicUnlock(&s_lock);
    if (!open)
        return 0;
    const int w = NGB_QUICK_WIDTH, h = NGB_QUICK_HEIGHT;
    memset(pixels, 0, (size_t)w * h * 4);
    s_px = pixels;
    s_w = w;
    s_h = h;
    /* NG2's cross: one expanded row, with the adjacent categories folded
     * above and below it. Every texture here comes from Black's spr.afs. */
    const float cx = 184, cy = 145;
    category_panel(above_category, &above_entry, 72, ready ? 220 : 120);
    category_panel(below_category, &below_entry, 218, ready ? 220 : 120);
    category_arrow(184, 40, 0, ready ? 255 : 100);
    category_arrow(184, 250, 1, ready ? 255 : 100);
    if (v.count)
    {
        int order[] = {-1, 1, 0};
        for (int q = 0; q < 3; q++)
        {
            int relative = order[q];
            if (relative && (v.count < 2 || (v.count == 2 && relative == 1)))
                continue;
            int i = (v.index + relative + v.count) % v.count;
            NgbQuickEntry *e = &v.entries[i];
            float x = cx + relative * 89, y = cy;
            panel(x, y, relative ? 66 : 96, relative ? 66 : 96, relative ? 150 : 235);
            art_fit(icon(e->art), x, y, relative ? 52 : 82, relative ? 52 : 82, relative ? 180 : 255);
            if (e->badge[0])   /* a bow's arrows, at the top left of the card (#173; its count is bottom right) */
                badge(e->badge, x - (relative ? 16 : 24), y - (relative ? 16 : 24), relative ? 26 : 38,
                      relative ? 180 : 255);
            if (e->quantity)
            {
                char qty[12];
                snprintf(qty, sizeof qty, "%u", e->count);
                text_at(qty, (int)(x + (relative ? 15 : 24)), (int)(y + (relative ? 9 : 20)),
                        relative ? 18 : 24, 0xFF5757);
            }
        }
        if (v.count > 1)
        {
            arrow(39, (int)cy, -1, 8);
            arrow(329, (int)cy, 1, 8);
        }
    }
    else
    {
        panel(cx, cy, 96, 96, 150);
    }
    if (ready && message[0] && now < message_until)
        text_at(message, 34, 292, 11, 0xFFAD90);
    /* Explicit test capture: only in a scratch directory chosen by the caller. */
    static unsigned captured;
    const char *capture = getenv("NGB_QUICK_CAPTURE");
    if (capture && revision != captured)
    {
        captured = revision;
        char path[4200];
        snprintf(path, sizeof path, "%s/quick_%04u.ppm", capture, revision);
        FILE *f = fopen(path, "wb");
        if (f)
        {
            fprintf(f, "P6\n%d %d\n255\n", w, h);
            for (int i = 0; i < w * h; i++)
            {
                unsigned char c[] = {pixels[i] >> 16, pixels[i] >> 8, pixels[i]};
                fwrite(c, 1, 3, f);
            }
            fclose(f);
        }
    }
    return open;
}
