/* D-pad quick selection. Gameplay inventory changes run on the guest thread
 * through Black's own equip, effect, and inventory-debit routines. */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include "quick_select.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void native_pause_check(void);
int ngb_pc_setting(int which);
int ngb_movie_active(void);
extern uint64_t SDL_GetTicks64(void);
static int s_in_quick;
static int s_pause;
static uint16_t s_last_heal = 0xFFFF;
static unsigned s_recovery_sound;
static uint64_t s_recovery_due;

typedef struct
{
    uint32_t r[9];
    RecompXmm x[8];
    double fp[8];
    int top;
} GuestContext;
static GuestContext save_context(void)
{
    GuestContext c = {{g_eax, g_ecx, g_edx, g_esp, g_ebx, g_esi, g_edi, g_ebp, g_seh_ebp},
                      {g_xmm0, g_xmm1, g_xmm2, g_xmm3, g_xmm4, g_xmm5, g_xmm6, g_xmm7},
                      {0},
                      g_fp_top};
    memcpy(c.fp, g_fp_stack, sizeof c.fp);
    return c;
}
static void restore_context(const GuestContext *c)
{
    g_eax = c->r[0];
    g_ecx = c->r[1];
    g_edx = c->r[2];
    g_esp = c->r[3];
    g_ebx = c->r[4];
    g_esi = c->r[5];
    g_edi = c->r[6];
    g_ebp = c->r[7];
    g_seh_ebp = c->r[8];
    g_xmm0 = c->x[0];
    g_xmm1 = c->x[1];
    g_xmm2 = c->x[2];
    g_xmm3 = c->x[3];
    g_xmm4 = c->x[4];
    g_xmm5 = c->x[5];
    g_xmm6 = c->x[6];
    g_xmm7 = c->x[7];
    memcpy(g_fp_stack, c->fp, sizeof c->fp);
    g_fp_top = c->top;
}
static uint32_t call(void (*fn)(void))
{
    PUSH32(esp, 0x001669EEu);
    fn();
    return eax;
}
static void play_sound(unsigned sound, const char *event)
{
    PUSH32(esp, 0x40);
    PUSH32(esp, sound);
    int voice = (int)call(sub_001E3400);
    esp += 8;
    fprintf(stderr, "[QUICK] %s sound=%X voice=%d ms=%llu\n", event, sound, voice,
            (unsigned long long)SDL_GetTicks64());
}
static void finish_recovery(void)
{
    unsigned sound = s_recovery_sound;
    s_recovery_sound = 0;
    if (sound) play_sound(sound, "recovery");
}
static uint32_t record(uint16_t item)
{
    uint32_t base = MEM32(0xDB98E4), count = MEM32(0xDB5E24);
    /* Descriptor storage can live in the runtime's high contiguous-memory
     * arena. Do not mistake its Xbox alias for an out-of-range RAM pointer. */
    if (!base || count > 1024 || item >= count)
        return 0;
    uint32_t o = MEM32(base + item * 4u);
    return o && o < 0x800000 - 0x40 && base + o >= base ? base + o : 0;
}
static int supported_layout(void)
{
    /* These task lists belong to the supported retail game, rather than
     * the host executable's addresses. A different game layout must never
     * receive our inventory writes or pause-counter changes. The installer's
     * full XBE section check remains the primary game-version check. */
    return MEM32(0x428210) == 4 && MEM32(0x428214) == 0x320E0 &&
           MEM32(0x428218) == 6 && MEM32(0x428248) == 4 &&
           MEM32(0x428250) == 5 && record(1) && MEM8(record(1) + 0xD) == 1;
}
static unsigned stock(uint16_t item)
{
    for (uint32_t p = 0x5891D0; p < 0x5893D0; p += 4)
        if (MEM16(p) == item)
            return MEM8(p + 2);
    return 0;
}
static uint32_t refill_effect(uint16_t item, int *kind)
{
    uint32_t r = record(item), base = MEM32(0xDB98E4);
    if (!r || !MEM8(r + 0x1D))
        return 0;
    uint32_t o = MEM32(r + 0x10);
    if (!o || o > 0x800000 - 32)
        return 0;
    int found = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        uint32_t effect = MEM32(base + o + i * 8);
        if (!effect)
            break;
        if (effect != 0x12 && effect != 0x13)
            return 0;
        if (found && found != (int)effect)
            return 0;
        found = (int)effect;
    }
    if (kind)
        *kind = found;
    return found ? base + o : 0;
}
static int regular_play(void)
{
    return !ngb_movie_active() && (!MEM8(0xDB9AD5) || s_pause) && MEM8(0x7F1B12) == 1 && !MEM8(0x7F1B11) &&
           (MEM8(0xDB9894) == 2 || MEM8(0xDB9894) == 3) && MEM8(0x4B7640) < 0x20 &&
           MEM16(0x58A566 + MEM8(0x4B7640) * 0x210u) != 0;
}
/* The pad's held buttons (0xD219A1) that Guard takes: the game's Button Settings (0xDB9420) give it
 * button 0-7 (that bit), X + A (8: bits 0 and 2) or Y + B (9: bits 1 and 3); sub_000DFCF0 reads them so. */
static unsigned guard_bits(void)
{
    static const unsigned char pair[2] = { 0x05, 0x0A };
    unsigned any = 0, bits = 0;
    for (unsigned i = 0; i < 10; i++)
    {
        if (MEM32(0xDB9420 + i * 4) == 7)
            bits |= i < 8 ? 1u << i : pair[i - 8];
        any |= MEM32(0xDB9420 + i * 4);
    }
    return any ? bits : 1u << 6;   /* the table not set yet: the default, LT */
}
static int can_pause(void) { return regular_play() && (s_pause || (call(sub_00166BD0) & 255)); }
static void pause_menu(int pause)
{
    if (pause == s_pause) return;
    /* The main game loop observes this counter and suspends its gameplay
     * task group. Rendering, input polling and music continue on their usual
     * schedule. Never wait inside the gameplay task to hold the picture. */
    if (pause) MEM8(0xDB5A68)++;
    else
    {
        /* Native input histories keep action transitions as well as the
         * current pad state. Flush them as the ordinary pause exit does. */
        GuestContext c = save_context();
        /* A guard held through the menu stays held: with it clear for a frame,
         * Ryu drops his block and nearby essence flies to him (#124). It stays
         * in the held state sub_00032460 copies to the input history too, or
         * the next frame reads it as a fresh press (an Intercept, with it on). */
        unsigned bits = guard_bits();
        for (unsigned i = 0; i < 5; i++)
        {
            MEM32(0xD219A0 + i * 0x78u) &= bits << 8;
            MEM32(0xD219A8 + i * 0x78u) = 0;
            MEM8(0xD21A16 + i * 0x78u) = 1;
        }
        call(sub_00032460);
        restore_context(&c);
        if (MEM8(0xDB5A68)) MEM8(0xDB5A68)--;
    }
    s_pause = pause;
    fprintf(stderr, "[QUICK] pause=%d depth=%u\n", pause, MEM8(0xDB5A68));
}
static void picture(NgbQuickEntry *e)
{
    const char *art = "", *name = "Weapon";
    unsigned lv = e->count;
    uint16_t art_id = e->id;
    /* Mission and costume variants use the same Black pictures. */
    switch (art_id)
    {
    case 0x14C:
        art_id = 1;
        break;
    case 0x14E:
        art_id = 0;
        break;
    case 0xF:
    case 0x9C:
    case 0x16D:
    case 0x16E:
    case 0x16F:
        art_id = 0x9B;
        break;
    case 0x9F:
    case 0xA0:
        art_id = 0x9E;
        break;
    case 0xAD:
    case 0xAE:
        art_id = 0xAC;
        break;
    case 0x160:
        art_id = 0x9A;
        break;
    case 0x14F:
        art_id = 0x26;
        break;
    case 0x150:
    case 0x161:
        art_id = 0x44;
        break;
    case 0x151:
    case 0x162:
    case 0x163:
        art_id = 0x95;
        break;
    case 0x152:
        art_id = 0xC9;
        break;
    case 0x153:
    case 0x164:
        art_id = 0xCA;
        break;
    case 0x154:
        art_id = 0xCB;
        break;
    case 0x155:
        art_id = 0xCC;
        break;
    case 0x156:
        art_id = 0xCD;
        break;
    case 0x157:
        art_id = 0xCE;
        break;
    case 0x159:
        art_id = 0xE1;
        break;
    case 0x15C:
        art_id = 0xF7;
        break;
    case 0x15D:
    case 0x15E:
    case 0x15F:
        art_id = 0x12B;
        break;
    case 0x170:
        art_id = 0x98;
        break;
    default:
        break;
    }
    uint32_t rec = record(e->id);
    if (rec)
        lv += MEM8(rec + 0x1E);
    if (rec && (MEM8(rec + 0xD) == 1 || MEM8(rec + 0xD) == 3) && art_id != 0x98)
        e->level = (uint8_t)lv;
    switch (art_id)
    {
    case 1:
        art = lv >= 3 ? "kata3" : lv == 2 ? "kata2" : "katana";
        name = "Dragon Sword";
        break;
    case 0xC9:
        art = "skata";
        name = "True Dragon Sword";
        break;
    case 0x26:
        art = "nchak";
        name = "Nunchaku";
        break;
    case 0x12B:
    case 0x133:
    case 0x134:
        art = lv >= 3 ? "rod3" : lv == 2 ? "rod2" : "rod";
        name = "Lunar";
        break;
    case 0x44:
        art = lv >= 2 ? "ham2" : "hammer";
        name = "War Hammer";
        break;
    case 0x95:
        art = lv >= 3 ? "zan3" : lv == 2 ? "zan2" : "zanba";
        name = "Dabilahro";
        break;
    case 0xCA:
        art = lv >= 2 ? "fl2" : "flail";
        name = "Vigoorian Flail";
        break;
    case 0xCB:
        art = "boku";
        name = "Wooden Sword";
        break;
    case 0xE1:
        art = "bokux";
        name = "Unlabored Flawlessness";
        break;
    case 0xCC:
    case 0x182:
    case 0x183:
        art = "yoto";                 /* every level: the game's yoto2.spr is its "No Data" picture */
        name = "Kitetsu";
        break;
    case 0xCD:
    case 0xFA:
        art = "koku";
        name = "Dark Dragon Blade";
        break;
    case 0xCE:
        art = lv >= 3 ? "sabe3" : lv == 2 ? "sabe2" : "saber";
        name = "Plasma Saber";
        break;
    case 0xF7:
        art = "ssabe";
        name = "Plasma Saber Mk.II";
        break;
    case 0:
        art = "syuri";
        name = "Shuriken";
        break;
    case 8:
        art = "ex_syuri";
        name = "Incendiary Shuriken";
        break;
    case 0x30:
        art = "fu_syuri";
        name = "Windmill Shuriken";
        break;
    case 0x165:                       /* its own picture, not the incendiary shuriken's */
        art = "smokebom";
        name = "Smoke Bomb";
        break;
    /* The bows look nearly alike: each also shows, small in a corner, the arrows it shoots (the HUD's pictures
     * of them, #173) */
    case 0x27:
    case 0x29:
        art = "bow01";
        name = "Bow";
        snprintf(e->badge, sizeof e->badge, "core");
        break;
    case 0x8D:
        art = "bow02";
        name = "Bow";
        snprintf(e->badge, sizeof e->badge, "plain");
        break;
    case 0xE2:
        art = "bow03";
        name = "Bow";
        snprintf(e->badge, sizeof e->badge, "fire");
        break;
    case 0xF8:
        art = "bow04";
        name = "Strongbow";
        snprintf(e->badge, sizeof e->badge, "plain");
        break;
    case 0x98:
        art = "sgun";
        name = "Spear Gun";
        break;
    case 0x9A:
        art = "scr_4";
        name = "The Art of the Fire Wheels";
        break;
    case 0x9B:
        art = lv >= 3 ? "scr_1_3" : lv == 2 ? "scr_1_2" : "scr_1";
        name = "The Art of the Inferno";
        break;
    case 0x9E:
        art = lv >= 3 ? "scr_2_3" : lv == 2 ? "scr_2_2" : "scr_2";
        name = "The Art of the Ice Storm";
        break;
    case 0xAC:
        art = lv >= 3 ? "scr_3_3" : lv == 2 ? "scr_3_2" : "scr_3";
        name = "The Art of the Inazuma";
        break;
    case 0xBE:
        art = "re_hp_s";
        name = "Elixir of Spiritual Life";
        break;
    case 0xBF:
        art = "re_hp_l";
        name = "Great Spirit Elixir";
        break;
    case 0xC0:
        art = "re_mp_s";
        name = "Elixir of the Devil Way";
        break;
    case 0xC1:
        art = "re_mp_l";
        name = "Great Devil Elixir";
        break;
    case 0x181:
        art = "re_mp_ex";
        name = "Fragrance of Dayflower";
        break;
    case 0x17E:
        art = "re_mp_ex_s";
        name = "Fragrance of Hydrangea";
        break;
    default:
        break;
    }
    if (*art)
        snprintf(e->art, sizeof e->art, "%s.spr", art);
    snprintf(e->name, sizeof e->name, "%s", name);
}
static int entry_category(uint16_t item)
{
    uint32_t r = record(item);
    if (!r) return 0;
    /* Keep the spear gun with Black's melee weapons, including its mission
     * variant. The loader still uses each descriptor's native equip slot. */
    if (item == 0x98 || item == 0x170) return NGB_QUICK_MELEE;
    unsigned cls = MEM8(r + 0xD);
    if (cls >= 1 && cls <= 3) return (int)cls;
    return refill_effect(item, NULL) ? NGB_QUICK_ITEMS : 0;
}
static void make_view(NgbQuickView *v, int category)
{
    memset(v, 0, sizeof *v);
    v->category = category;
    v->equipped = 0xFFFF;
    v->freeze = ngb_pc_setting(19);
    uint32_t player = MEM8(0x4B7640) * 0x210u;
    for (uint32_t p = 0x5891D0; p < 0x5893D0 && v->count < NGB_QUICK_CAP; p += 4)
    {
        uint16_t item = MEM16(p);
        unsigned count = MEM8(p + 2);
        uint32_t r = record(item);
        int kind = 0;
        if (!count || item == 0xFFFF || !r)
            continue;
        if (entry_category(item) != category)
            continue;
        if (category == NGB_QUICK_ITEMS) refill_effect(item, &kind);
        int duplicate = 0;
        for (int i = 0; i < v->count; i++)
            if (v->entries[i].id == item)
                duplicate = 1;
        if (duplicate)
            continue;
        NgbQuickEntry *e = &v->entries[v->count];
        e->id = item;
        e->count = (uint8_t)count;
        e->usable = category != NGB_QUICK_ITEMS || (kind == 0x12 ? MEM16(0x58A566 + player) < MEM16(0x58A564 + player)
                                                   : MEM16(0x58A56Eu + player) < MEM16(0x58A56C + player));
        picture(e);
        /* Unsupported artwork cannot enter an invisible carousel cell. */
        if (!e->art[0])
            continue;
        e->quantity = category == NGB_QUICK_ITEMS;
        if (category == NGB_QUICK_PROJECTILES)
        {
            /* A bow's inventory count is ownership, not its ammunition.
             * Its descriptor names the arrows required by that bow variant. */
            uint32_t components = MEM32(r + 4), base = MEM32(0xDB98E4);
            if (components && components < 0x800000 - 4 && MEM16(base + components) != 0xFFFF)
            {
                e->count = (uint8_t)stock(MEM16(base + components));
                e->quantity = 1;
            }
            else if (item == 8 || item == 0x165)
                e->quantity = 1;
        }
        unsigned cls = MEM8(r + 0xD);
        if ((category != NGB_QUICK_ITEMS && item == MEM16(0x58A570 + player + (cls - 1) * 2)) ||
            (category == NGB_QUICK_ITEMS && item == s_last_heal))
        {
            /* the melee view's: its own slot's (the Spear Gun, listed there, has the ranged slot) */
            if (category != NGB_QUICK_MELEE || cls == 1 || v->equipped == 0xFFFF)
            {
                v->index = v->count;
                v->equipped = item;
            }
        }
        e->other_slot = category == NGB_QUICK_MELEE && cls != 1;
        v->count++;
    }
}
static void make_menu(NgbQuickMenu *menu, int category, int ready)
{
    memset(menu, 0, sizeof *menu);
    menu->category = category;
    menu->ready = ready;
    for (int i = 0; i < NGB_QUICK_CATEGORIES; i++) make_view(&menu->views[i], i + 1);
}
static void apply(uint16_t item, int category)
{
    if (!can_pause() || !stock(item))
    {
        fprintf(stderr, "[QUICK] refused stale selection %04X\n", item);
        return;
    }
    uint32_t r = record(item), player = MEM8(0x4B7640) * 0x210u;
    if (!r)
        return;
    if (entry_category(item) != category)
    {
        fprintf(stderr, "[QUICK] refused %04X: not in category %d\n", item, category);
        return;
    }
    if (category != NGB_QUICK_ITEMS)
    {
        unsigned cls = MEM8(r + 0xD);
        if (cls < 1 || cls > 3)
            return;
        /* The inventory handler checks ownership of the weapon before
         * passing it to the resource loader. */
        esi = item;
        if (!call(sub_001542C0))
        {
            fprintf(stderr, "[QUICK] refused %04X: the game does not count it as owned\n", item);
            return;
        }
        esp -= 64;
        uint32_t scratch = esp;
        ecx = item;
        eax = scratch;
        unsigned n = call(sub_001550F0);
        if (n > 16)
        {
            fprintf(stderr, "[QUICK] refused %04X: %u required parts\n", item, n);
            return;
        }
        /* A part the game does not count as owned does not stop it: its own Weapon menu equips such a
         * weapon (the Kitetsu of a chapter 13 save, part 0x0EA; the Spear Gun without spears, 0x099). */
        for (unsigned i = 0; i < n; i++)
        {
            esi = MEM16(scratch + i * 2);
            if (!call(sub_001542C0))
                fprintf(stderr, "[QUICK] %04X: its part %04X is not owned\n", item, (unsigned)MEM16(scratch + i * 2));
        }
        if (MEM16(0x58A570 + player + (cls - 1) * 2) == item)
        {
            play_sound(5, "confirmation");
            return;
        }
        /* Weapon assets share memory with the world. Use Black's complete
         * inventory transition, including the world/player restoration on
         * exit, even when selection itself is configured to remain live. */
        s_in_quick = 1;
        call(sub_002FD820);
        eax = cls - 1;
        PUSH32(esp, item);
        unsigned accepted = call(sub_002EAA20) & 255;
        esp += 4;
        PUSH32(esp, 1);
        call(sub_00166B90);
        esp += 4;
        s_in_quick = 0;
        fprintf(stderr, "[QUICK] equipped accepted=%u slot=%u item=%04X actual=%04X\n", accepted, cls - 1, item,
                MEM16(0x58A570 + player + (cls - 1) * 2));
        /* The native resource transition pauses active voices. Confirm only
         * after it has restored the world and resumed audio. */
        if (accepted && MEM16(0x58A570 + player + (cls - 1) * 2) == item)
            play_sound(5, "confirmation");
    }
    else
    {
        int kind = 0;
        uint32_t effect = refill_effect(item, &kind);
        if (!effect)
            return;
        edi = effect;
        if (!call(sub_00183EC0))
            return;
        unsigned before = stock(item);
        uint16_t hp = MEM16(0x58A566 + player), mp = MEM16(0x58A56Eu + player);
        esp -= 64;
        uint32_t state = esp;
        memset((void *)XBOX_PTR(state), 0, 48);
        ebx = effect;
        PUSH32(esp, state);
        unsigned result = call(sub_00183C60);
        esp += 4;
        if (result == 0x80 && (hp != MEM16(0x58A566 + player) || mp != MEM16(0x58A56Eu + player)))
        {
            uint16_t old_item = MEM16(0xDB5E20), old_count = MEM16(0xDB5E22);
            MEM16(0xDB5E20) = item;
            MEM16(0xDB5E22) = 1;
            call(sub_00155D10);
            MEM16(0xDB5E20) = old_item;
            MEM16(0xDB5E22) = old_count;
            s_last_heal = item;
            fprintf(stderr, "[QUICK] used %04X stock=%u->%u health=%u->%u ninpo=%u->%u\n", item, before,
                    stock(item), hp, MEM16(0x58A566 + player), mp, MEM16(0x58A56Eu + player));
            play_sound(5, "confirmation");
            /* Give confirmation a short lead before the inventory gauge's
             * recovery effect. Service it on later guest frames; never wait
             * here or delay the item effect, gameplay or music. */
            s_recovery_sound = kind == 0x12 ? 0xA1 : 0xA6;
            s_recovery_due = SDL_GetTicks64() + 100;
        }
        else
            fprintf(stderr, "[QUICK] item refused %04X result=%X stock=%u\n", item, result, stock(item));
    }
}
static void dispatch_command(uint16_t item, int category)
{
    if (category > 0)
    {
        /* A fast second command must not overwrite an earlier item's sound
         * or carry it into the weapon loader's audio pause. */
        finish_recovery();
        apply(item, category);
    }
}
static uint64_t s_closed_since;   /* the gate closed on an open menu, since (SDL ms) */
static void quick_step(void)
{
    if (!regular_play()) s_recovery_sound = 0;
    else if (s_recovery_sound && SDL_GetTicks64() >= s_recovery_due)
        finish_recovery();
    if (!ngb_pc_setting(20))
    {
        ngb_quick_close();
        ngb_quick_take_open();
        pause_menu(0);
        return;
    }
    int steps = ngb_quick_take_navigation();
    if (regular_play())
        while (steps-- > 0)
        {
            /* Native pause-menu cursor movement. Host input only queues the
             * event; sound dispatch belongs on the guest gameplay thread. */
            play_sound(1, "navigation");
        }
    uint16_t item;
    int category;
    /* The native inventory gate includes the player's current action and
     * resource transitions. Opening is independent of this gate; pausing,
     * category changes and applying a selection must wait for it. Recheck
     * queued host input here: live gameplay may have begun another action. */
    int ready = can_pause();
    if (ngb_quick_take_command(&item, &category) && ready)
    {
        /* With the game paused (Quick Menu Pause), a restorative leaves the menu open, so several can be used
         * in a row (#200); its count is rebuilt below, and one more than can help reads "Already full". Live,
         * it closes as before: open, the menu keeps the controls while the fight goes on. A weapon or Ninpo
         * closes it either way: equipping one reloads the world. */
        if (category != NGB_QUICK_ITEMS || !ngb_quick_frozen())
            ngb_quick_close();
        dispatch_command(item, category);
    }
    if (!ngb_quick_wanted()) pause_menu(0);
    if (!regular_play())
    {
        ngb_quick_close();
        pause_menu(0);
        return;
    }
    /* The game allows no menu while the gate is closed -- an attack finishing, but also states that last:
     * aiming the bow in first person, its own Start ignored there too. A request waits for the gate (it
     * lapses after a quarter second, src/quick_ui.c), and a menu the gate stays closed on for a second
     * closes: open there, it showed the cards with nothing working but Left and Right (#76). */
    int requested = ready ? ngb_quick_take_open() : 0;
    category = ngb_quick_category();
    if (category && !ready && !s_pause)
    {
        if (!s_closed_since)
            s_closed_since = SDL_GetTicks64();
        else if (SDL_GetTicks64() - s_closed_since > 1000)
        {
            fprintf(stderr, "[QUICK] closed: the game allows no menu now\n");
            ngb_quick_close();
            s_closed_since = 0;
            return;
        }
    }
    else
        s_closed_since = 0;
    if (category)
    {
        NgbQuickMenu menu;
        make_menu(&menu, category, ready);
        ngb_quick_refresh(&menu);
    }
    else
    {
        const char *enabled = getenv("NGB_QUICK_SELECT");
        if (s_in_quick || (enabled && !strcmp(enabled, "0")))
            return;
        if (!requested)
            return;
        if (!ngb_quick_available() || !supported_layout())
        {
            fprintf(stderr, "[QUICK] unavailable: unsupported renderer or game layout\n");
            return;
        }
        NgbQuickMenu menu;
        make_menu(&menu, requested, ready);
        ngb_quick_open(&menu);
        /* Keep the input history of the action already in progress. In
         * particular, clearing it here could release a charged attack.
         * Host binding filters consume menu controls on subsequent polls. */
    }
    pause_menu(ngb_quick_frozen());
}
void sub_00166960(void)
{
    GuestContext c = save_context();
    quick_step();
    restore_context(&c);
    if (s_pause) { esp += 4; return; }
    native_pause_check();
}
void ngb_quick_tick(void)
{
    /* The pause-check hook is absent in cutscenes and other game states. */
    if (!s_in_quick && (!regular_play() || !ngb_pc_setting(20)))
    {
        s_recovery_sound = 0;
        ngb_quick_close();
        pause_menu(0);
    }
    static int probe = -1;
    if (probe < 0)
        probe = getenv("NGB_QUICK_PROBE") != NULL;
    if (probe)
    {
        /* SDL's clock is host time, independent of the guest pause counter. */
        static uint64_t last;
        static unsigned frames;
        uint64_t now = SDL_GetTicks64();
        frames++;
        if (now - last >= 1000)
        {
            fprintf(stderr, "[QUICK PROBE] ms=%llu frames=%u menu=%d pause=%d depth=%u/%u gate=%u state=%u task4=%X pos=%.5f,%.5f,%.5f weapon=%04X\n",
                    (unsigned long long)now, frames, ngb_quick_wanted(), s_pause,
                    MEM8(0xDB5A68), MEM8(0xDB5A62), MEM8(0xDB9AD5), MEM8(0x7F1B12),
                    MEM32(0xD21C80), MEMF(0x4B5710), MEMF(0x4B5714), MEMF(0x4B5718),
                    MEM16(0x58A570 + MEM8(0x4B7640) * 0x210u));
            last = now;
            frames = 0;
        }
    }
}
/* The original pause check follows, kept in the generated code's style.
 * Its only change is the name, so Black retains every can-pause check. */
/**
 * sub_00166960
 * Original: 0x00166960 - 0x00166A38 (216 bytes, 57 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
static void native_pause_check(void)
{
    RECOMP_ENTRY(0x00166960u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_00166960: ;
    SET_LO8(eax, MEM8(0x7F1B11));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001669A3; /* je: equal / zero */

loc_00166969: ;
    SET_LO8(eax, MEM8(0xDB5A68));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    MEM8(0x7F1B11) = 0;
    if (CMP_LE(_fas, _fbs)) goto loc_00166980; /* jle: less or equal (signed <=) */

loc_00166979: ;
    SET_LO8(eax, LO8(eax) - 1);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fb = (_fa == 0x7Fu); /* dec result/SF/OF; CF unchanged */
    MEM8(0xDB5A68) = LO8(eax);

loc_00166980: ;
    _fa = (uint32_t)(MEM8(0xDB9C6A)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0xDB9C6A), 4 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00166999; /* jne: not equal / not zero */

loc_00166989: ;
    edx = MEM32(0x4B4B70);
    PUSH32(esp, 0x00166994u); RECOMP_ABI_CALL(0x0003FA10u, sub_0003FA10); /* call 0x0003FA10 */

loc_00166994: ;
    PUSH32(esp, 0x00166999u); RECOMP_ABI_CALL(0x00032E70u, sub_00032E70); /* call 0x00032E70 */

loc_00166999: ;
    eax = 3;
    g_seh_ebp = ebp; sub_0030C930(); return; /* tail jmp 0x0030C930 */

loc_001669A3: ;
    PUSH32(esp, ebx);
    ecx = 9;
    PUSH32(esp, 0x001669AEu); RECOMP_ABI_CALL(0x000324C0u, sub_000324C0); /* call 0x000324C0 */

loc_001669AE: ;
    ecx = ZX8(MEM8(0xDB932E));
    ebx = MEM32(0xD21980);
    edx = 1;
    edx = edx << LO8(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edx, ebx (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_00166A20; /* jne: not equal / not zero */

loc_001669C6: ;
    ebx = MEM32(0x4B75CC);
    eax = 0x4B4E50;
    PUSH32(esp, 0x001669D6u); RECOMP_ABI_CALL(0x00034A10u, sub_00034A10); /* call 0x00034A10 */

loc_001669D6: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001669EE; /* je: equal / zero */

loc_001669DA: ;
    SET_LO8(eax, MEM8(eax + 6));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x73) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x73 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001669E5; /* je: equal / zero */

loc_001669E1: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x74) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x74 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001669EE; /* jne: not equal / not zero */

loc_001669E5: ;
    MEM16(0x4B75D2) = 0;

loc_001669EE: ;
    PUSH32(esp, 0x001669F3u); RECOMP_ABI_CALL(0x00166BD0u, sub_00166BD0); /* call 0x00166BD0 */

loc_001669F3: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00166A1E; /* je: equal / zero */

loc_001669F7: ;
    SET_LO8(edx, MEM8(0xDB5A68));
    SET_LO8(edx, LO8(edx) + 1);
    _fa = (uint32_t)(LO8(edx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fb = (_fa == 0x80u); /* inc result/SF/OF; CF unchanged */
    PUSH32(esp, 0x17B740);
    eax = 3;
    MEM8(0x7F1B11) = 1;
    MEM8(0xDB5A68) = LO8(edx);
    PUSH32(esp, 0x00166A1Bu); RECOMP_ABI_CALL(0x0030C8E0u, sub_0030C8E0); /* call 0x0030C8E0 */

loc_00166A1B: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_00166A1E: ;
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_00166A20: ;
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x78);
    ecx = MEM32(ecx + 0xD219A8);
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 0x10 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_001669EE; /* jne: not equal / not zero */

loc_00166A2E: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, ecx (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_00166A1E; /* je: equal / zero */

loc_00166A32: ;
    POP32(esp, ebx);
    g_seh_ebp = ebp; sub_00166A40(); return; /* tail jmp 0x00166A40 */

}

/* The player's per-frame status tick: the timed items' counters, the breath underwater and the
 * drowning damage once it is gone. The lifted body, kept here (src/manual_functions.json) for the
 * one check at its start. */
/**
 * sub_00155E30
 * Original: 0x00155E30 - 0x001561F4 (964 bytes, 258 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_00155E30(void)
{
    RECOMP_ENTRY(0x00155E30u);
    /* The quick menu's pause raises the game's pause counter, which holds the gameplay tasks but not
     * this tick: underwater the breath ran out behind the open menu, and then the health (#108).
     * Nothing of it advances while that pause holds. */
    if (s_pause) { esp += 4; return; }
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    double _fca = 0.0, _fcb = 0.0;
    (void)_fca; (void)_fcb;

loc_00155E30: ;
    PUSH32(esp, ecx);
    eax = ZX8(MEM8(0x4B7640));
    { int64_t _r = (int64_t)(int32_t)(eax) * (int64_t)(int32_t)(0x210);
      eax = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    PUSH32(esp, ebx);
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(MEM16(eax + 0x58A566)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(eax + 0x58A566), LO16(ebx) (16-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_001561F1; /* jbe: below or equal (unsigned <=) */

loc_00155E4E: ;
    PUSH32(esp, edi);
    PUSH32(esp, 0xB);
    edi = esp + 0xC;
    PUSH32(esp, 0x00155E5Au); RECOMP_ABI_CALL(0x00154D30u, sub_00154D30); /* call 0x00154D30 */

loc_00155E5A: ;
    xmm2 = XMM_SCALAR(MEMF(0xDB988C)); /* movss */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00155E8E; /* je: equal / zero */

loc_00155E69: ;
    ecx = ZX16(MEM16(esp + 8));
    xmm0 = XMM_SCALAR(MEMF(0xDB5E10)); /* movss */
    xmm1.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00155EC7; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */

loc_00155E7F: ;
    xmm1 = xmm2; /* movaps */
    xmm1.f[0] = xmm1.f[0] + xmm0.f[0]; /* addss */
    MEMF(0xDB5E10) = xmm1.f[0]; /* movss */

loc_00155E8E: ;
    xmm1 = XMM_ZERO(); /* xorps self = zero */

loc_00155E91: ;
    PUSH32(esp, 0xC);
    edi = esp + 0xC;
    PUSH32(esp, 0x00155E9Cu); RECOMP_ABI_CALL(0x00154D30u, sub_00154D30); /* call 0x00154D30 */

loc_00155E9C: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00155F0E; /* je: equal / zero */

loc_00155EA3: ;
    eax = ZX16(MEM16(esp + 8));
    xmm0 = XMM_SCALAR(MEMF(0xDB5E14)); /* movss */
    xmm3.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    _fca = xmm3.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00155EF7; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm0.f[0]) */

loc_00155EB9: ;
    xmm0.f[0] = xmm0.f[0] + xmm2.f[0]; /* addss */
    MEMF(0xDB5E14) = xmm0.f[0]; /* movss */
    goto loc_00155F0E;

loc_00155EC7: ;
    edx = ZX16(MEM16(esp + 0xA));
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    PUSH32(esp, edx);
    PUSH32(esp, 0x4B4E50);
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    MEMF(0xDB5E10) = xmm1.f[0]; /* movss */
    PUSH32(esp, 0x00155EE4u); RECOMP_ABI_CALL(0x001E1D90u, sub_001E1D90); /* call 0x001E1D90 */

loc_00155EE4: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00155E91; /* je: equal / zero */

loc_00155EEB: ;
    eax = 0x4B4E50;
    PUSH32(esp, 0x00155EF5u); RECOMP_ABI_CALL(0x002FD3E0u, sub_002FD3E0); /* call 0x002FD3E0 */

loc_00155EF5: ;
    goto loc_00155E91;

loc_00155EF7: ;
    edx = ZX16(MEM16(esp + 0xA));
    eax = 0x4B4E50;
    MEMF(0xDB5E14) = xmm1.f[0]; /* movss */
    PUSH32(esp, 0x00155F0Eu); RECOMP_ABI_CALL(0x001E21D0u, sub_001E21D0); /* call 0x001E21D0 */

loc_00155F0E: ;
    PUSH32(esp, 0xD);
    edi = esp + 0xC;
    PUSH32(esp, 0x00155F19u); RECOMP_ABI_CALL(0x00154D30u, sub_00154D30); /* call 0x00154D30 */

loc_00155F19: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00155F9F; /* je: equal / zero */

loc_00155F20: ;
    ecx = ZX16(MEM16(esp + 8));
    xmm0 = XMM_SCALAR(MEMF(0xDB5E18)); /* movss */
    xmm3.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    _fca = xmm3.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00155F44; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs xmm0.f[0]) */

loc_00155F36: ;
    xmm0.f[0] = xmm0.f[0] + xmm2.f[0]; /* addss */
    MEMF(0xDB5E18) = xmm0.f[0]; /* movss */
    goto loc_00155F9F;

loc_00155F44: ;
    edx = ZX8(MEM8(0x4B7640));
    { int64_t _r = (int64_t)(int32_t)(edx) * (int64_t)(int32_t)(0x210);
      edx = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    SET_LO16(eax, MEM16(edx + 0x58A566));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(1) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 1 (16-bit) */
    MEMF(0xDB5E18) = xmm1.f[0]; /* movss */
    if (CMP_BE(_fa, _fb)) goto loc_00155F9F; /* jbe: below or equal (unsigned <=) */

loc_00155F66: ;
    SET_LO16(ecx, MEM16(esp + 0xA));
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fb = (uint32_t)(LO16(eax)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ecx), LO16(eax) (16-bit) */
    if (CMP_B(_fa, _fb)) goto loc_00155F7C; /* jb: below (unsigned <) */

loc_00155F70: ;
    ecx = ZX16(LO16(eax));
    eax = 1;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    goto loc_00155F81;

loc_00155F7C: ;
    eax = ZX16(LO16(ecx));
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */

loc_00155F81: ;
    PUSH32(esp, eax);
    PUSH32(esp, 0x4B4E50);
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    PUSH32(esp, 0x00155F8Eu); RECOMP_ABI_CALL(0x001E1D90u, sub_001E1D90); /* call 0x001E1D90 */

loc_00155F8E: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00155F9F; /* je: equal / zero */

loc_00155F95: ;
    eax = 0x4B4E50;
    PUSH32(esp, 0x00155F9Fu); RECOMP_ABI_CALL(0x002FD3E0u, sub_002FD3E0); /* call 0x002FD3E0 */

loc_00155F9F: ;
    PUSH32(esp, 0x18);
    PUSH32(esp, 0x00155FA6u); RECOMP_ABI_CALL(0x00154E40u, sub_00154E40); /* call 0x00154E40 */

loc_00155FA6: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001561E7; /* jne: not equal / not zero */

loc_00155FB1: ;
    SET_LO8(eax, MEM8(0x4B75E9));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(5) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 5 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00156165; /* jne: not equal / not zero */

loc_00155FBE: ;
    SET_LO16(eax, MEM16(0x5895E0));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(ebx) (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001560A4; /* jne: not equal / not zero */

loc_00155FCD: ;
    SET_LO8(eax, MEM8(0xDB9ADA));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_00156006; /* je: equal / zero */

loc_00155FDE: ;
    edx = ZX8(MEM8(0x4B7640));
    { int64_t _r = (int64_t)(int32_t)(edx) * (int64_t)(int32_t)(0x210);
      edx = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    ecx = ZX16(MEM16(edx + 0x58A564));
    eax = ZX8(LO8(eax));
    edx = eax + eax * 4;
    edx = edx << 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    xmm1.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] / xmm0.f[0]; /* divss */

loc_00156006: ;
    xmm0 = XMM_SCALAR(MEMF(0xDB5E28)); /* movss */
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */
    eax = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    PUSH32(esp, eax);
    xmm0.f[0] = xmm0.f[0] - xmm1.f[0]; /* subss */
    PUSH32(esp, 0x4B4E50);
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    MEMF(0xDB5E28) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x00156035u); RECOMP_ABI_CALL(0x001E1D90u, sub_001E1D90); /* call 0x001E1D90 */

loc_00156035: ;
    xmm0 = XMM_SCALAR(MEMF(0xDB5E1C)); /* movss */
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fca = xmm0.f[0]; _fcb = MEMF(0x414900); /* comiss */
    if ((_fca < _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00156094; /* jb: below (unsigned <) (xmm0.f[0] vs MEMF(0x414900)) */

loc_00156049: ;
    PUSH32(esp, 0x6B);
    PUSH32(esp, 0x00156050u); RECOMP_ABI_CALL(0x001E3E80u, sub_001E3E80); /* call 0x001E3E80 */

loc_00156050: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFFFFFFu (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00156089; /* je: equal / zero */

loc_00156058: ;
    _fa = (uint32_t)(MEM8(0x4B75E8)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0x4B75E8), LO8(ebx) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00156089; /* jne: not equal / not zero */

loc_00156060: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x80000000u) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x80000000u (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_00156079; /* jb: below (unsigned <) */

loc_00156067: ;
    ecx = eax + -2147483648;
    eax = 0x4B5710;
    PUSH32(esp, 0x00156077u); RECOMP_ABI_CALL(0x001E3590u, sub_001E3590); /* call 0x001E3590 */

loc_00156077: ;
    goto loc_00156089;

loc_00156079: ;
    PUSH32(esp, 0x4B5710);
    PUSH32(esp, ebx);
    PUSH32(esp, ebx);
    PUSH32(esp, eax);
    PUSH32(esp, 0x00156086u); RECOMP_ABI_CALL(0x001E34C0u, sub_001E34C0); /* call 0x001E34C0 */

loc_00156086: ;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x10;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_00156089: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    xmm2 = XMM_SCALAR(MEMF(0xDB988C)); /* movss */

loc_00156094: ;
    POP32(esp, edi);
    xmm0.f[0] = xmm0.f[0] + xmm2.f[0]; /* addss */
    MEMF(0xDB5E1C) = xmm0.f[0]; /* movss */
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

loc_001560A4: ;
    xmm0 = XMM_SCALAR(MEMF(0xDB5E1C)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(0x4151BC); /* comiss */
    if ((_fca < _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0015614D; /* jb: below (unsigned <) (xmm0.f[0] vs MEMF(0x4151BC)) */

loc_001560B9: ;
    xmm2 = XMM_SCALAR(MEMF(0x414AE8)); /* movss */
    eax = ZX16(LO16(eax));
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    ecx = 0;
    SET_LO8(ecx, ((_fas < 0)) ? 1 : 0); /* sets */
    MEMF(0xDB5E1C) = xmm1.f[0]; /* movss */
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    ecx = ecx & eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    SET_LO16(eax, LO16(ecx));
    edx = ZX16(LO16(eax));
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    _fca = xmm2.f[0]; _fcb = xmm0.f[0]; /* ucomiss */
    MEM16(0x5895E0) = LO16(eax);
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((_fca == _fcb || (_fca != _fca || _fcb != _fcb))) ? 0x40u : 0u) | (((_fca != _fca || _fcb != _fcb)) ? 0x04u : 0u) | (((_fca < _fcb || (_fca != _fca || _fcb != _fcb))) ? 0x01u : 0u) | 0x02u) << 8); /* lahf (ucomiss) */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    if (RECOMP_PARITY8((_fa) & (_fb))) goto loc_00156122; /* jp: parity */

loc_001560F1: ;
    PUSH32(esp, 0x6B);
    PUSH32(esp, 0x001560F8u); RECOMP_ABI_CALL(0x001E3E80u, sub_001E3E80); /* call 0x001E3E80 */

loc_001560F8: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFFFFFFu (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0015611F; /* je: equal / zero */

loc_00156100: ;
    _fa = (uint32_t)(MEM8(0x4B75E8)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0x4B75E8), LO8(ebx) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0015611F; /* jne: not equal / not zero */

loc_00156108: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x80000000u) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x80000000u (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_0015612E; /* jb: below (unsigned <) */

loc_0015610F: ;
    ecx = eax + -2147483648;
    eax = 0x4B5710;
    PUSH32(esp, 0x0015611Fu); RECOMP_ABI_CALL(0x001E3590u, sub_001E3590); /* call 0x001E3590 */

loc_0015611F: ;
    xmm1 = XMM_ZERO(); /* xorps self = zero */

loc_00156122: ;
    POP32(esp, edi);
    MEMF(0xDB5E28) = xmm1.f[0]; /* movss */
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

loc_0015612E: ;
    PUSH32(esp, 0x4B5710);
    PUSH32(esp, ebx);
    PUSH32(esp, ebx);
    PUSH32(esp, eax);
    PUSH32(esp, 0x0015613Bu); RECOMP_ABI_CALL(0x001E34C0u, sub_001E34C0); /* call 0x001E34C0 */

loc_0015613B: ;
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x10;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, edi);
    MEMF(0xDB5E28) = xmm1.f[0]; /* movss */
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

loc_0015614D: ;
    POP32(esp, edi);
    xmm0.f[0] = xmm0.f[0] + xmm2.f[0]; /* addss */
    MEMF(0xDB5E1C) = xmm0.f[0]; /* movss */
    MEMF(0xDB5E28) = xmm1.f[0]; /* movss */
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

loc_00156165: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 4 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001561D7; /* jne: not equal / not zero */

loc_00156169: ;
    eax = ZX16(MEM16(0x5895E0));
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x64 (32-bit) */
    MEM16(0x5895E0) = LO16(eax);
    if (CMP_L(_fas, _fbs)) goto loc_00156187; /* jl: less (signed <) */

loc_0015617E: ;
    MEM16(0x5895E0) = 0x64;

loc_00156187: ;
    PUSH32(esp, esi);
    MEMF(0xDB5E1C) = xmm1.f[0]; /* movss */
    MEMF(0xDB5E28) = xmm1.f[0]; /* movss */
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    esi = 0x8D3ADC;
    /* nop */

loc_001561A0: ;
    edx = MEM32(esi);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x6B) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x6B (32-bit) */
    SET_LO8(eax, (CMP_EQ(_fa, _fb)) ? 1 : 0); /* sete */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001561C6; /* je: equal / zero */

loc_001561AE: ;
    PUSH32(esp, edi);
    PUSH32(esp, 0x001561B4u); RECOMP_ABI_CALL(0x00320750u, sub_00320750); /* call 0x00320750 */

loc_001561B4: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi + -4) = ebx;
    MEM32(esi) = ebx;
    MEM32(esi + 4) = 0xFFFFFFFFu;
    MEM32(esi + 0x10) = ebx;

loc_001561C6: ;
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x18;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x8D3E9C) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0x8D3E9C (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_001561A0; /* jl: less (signed <) */

loc_001561D2: ;
    POP32(esp, esi);
    POP32(esp, edi);
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

loc_001561D7: ;
    MEMF(0xDB5E1C) = xmm1.f[0]; /* movss */
    MEMF(0xDB5E28) = xmm1.f[0]; /* movss */

loc_001561E7: ;
    MEM16(0x5895E0) = 0x64;
    POP32(esp, edi);

loc_001561F1: ;
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

}


/* The level's frame count (0xD52CF0: the Mission and karma times) and the one at 0x5893EC, each frame.
 * The lifted body, kept here (src/manual_functions.json): the game holds both while its pause menu is
 * up, but the quick menu's pause is not that menu, so the times ran on behind it (#108). Neither
 * counts while that pause holds. */
/**
 * sub_002E1BA0
 * Original: 0x002E1BA0 - 0x002E1C76 (214 bytes, 55 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_002E1BA0(void)
{
    RECOMP_ENTRY(0x002E1BA0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_002E1BA0: ;
    SET_LO16(eax, MEM16(0xDB9AC2));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(MEM16(0xD52E04)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), MEM16(0xD52E04) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002E1BBD; /* je: equal / zero */

loc_002E1BAF: ;
    eax = MEM32(0xDB9890);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002E1BBD; /* jne: not equal / not zero */

loc_002E1BB8: ;
    PUSH32(esp, 0x002E1BBDu); RECOMP_ABI_CALL(0x00219D20u, sub_00219D20); /* call 0x00219D20 */

loc_002E1BBD: ;
    SET_LO16(ecx, MEM16(0xDB9AC2));
    MEM16(0xD52E04) = LO16(ecx);
    PUSH32(esp, 0x002E1BD0u); RECOMP_ABI_CALL(0x0016B3C0u, sub_0016B3C0); /* call 0x0016B3C0 */

loc_002E1BD0: ;
    _fa = (uint32_t)(MEM32(0xDA402C)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0xDA402C), 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002E1BE3; /* jne: not equal / not zero */

loc_002E1BD9: ;
    MEM32(0xDA402C) = 0;

loc_002E1BE3: ;
    _fa = (uint32_t)(MEM8(0xD52B3D)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0xD52B3D), 1 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002E1BF3; /* jne: not equal / not zero */

loc_002E1BEC: ;
    MEM8(0xD52B3D) = 0;

loc_002E1BF3: ;
    PUSH32(esp, 0x002E1BF8u); RECOMP_ABI_CALL(0x002EA260u, sub_002EA260); /* call 0x002EA260 */

loc_002E1BF8: ;
    SET_LO8(eax, MEM8(0xDB9AD5));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002E1C17; /* je: equal / zero */

loc_002E1C01: ;
    SET_LO8(eax, MEM8(0x7F1B12));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 2 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002E1C24; /* je: equal / zero */

loc_002E1C0A: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 3 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002E1C24; /* je: equal / zero */

loc_002E1C0E: ;
    _fa = (uint32_t)(MEM8(0x7F1B11)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0x7F1B11), 1 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002E1C24; /* je: equal / zero */

loc_002E1C17: ;
    if (s_pause) goto loc_002E1C24;
    eax = MEM32(0xD52CF0);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0xD52CF0) = eax;
    goto loc_002E1C29;

loc_002E1C24: ;
    eax = MEM32(0xD52CF0);

loc_002E1C29: ;
    _fa = (uint32_t)(MEM16(0xDB9C6E)) & 0xFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0xDB9C6E), 0xFFFF (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002E1C4E; /* jne: not equal / not zero */

loc_002E1C34: ;
    ecx = MEM32(0x8DB028);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002E1C4E; /* jne: not equal / not zero */

loc_002E1C3E: ;
    ecx = MEM32(0x8DB034);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002E1C4E; /* jne: not equal / not zero */

loc_002E1C48: ;
    if (!s_pause) MEM32(0x5893EC) = MEM32(0x5893EC) + 1;
    _fa = (uint32_t)(MEM32(0x5893EC)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_002E1C4E: ;
    eax = eax & 0x8000003Fu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    if ((_fas >= 0)) goto loc_002E1C5A; /* jns: not sign (positive) */

loc_002E1C55: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    eax = eax | 0xFFFFFFC0u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_002E1C5A: ;
    eax = eax + eax * 4;
    eax = eax << 0xB;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x4B4E50) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x4B4E50;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(MEM8(eax + 0x278E)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(eax + 0x278E), 0xFF (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002E1C75; /* je: equal / zero */

loc_002E1C6E: ;
    PUSH32(esp, eax);
    PUSH32(esp, 0x002E1C74u); RECOMP_ABI_CALL(0x002CD950u, sub_002CD950); /* call 0x002CD950 */

loc_002E1C74: ;
    POP32(esp, ecx);

loc_002E1C75: ;
    esp += 4; return; /* ret */

}


/* Mission Mode's per-frame step: its state and its time (0x5893D0, the Time on its HUD). The lifted body,
 * kept here (src/manual_functions.json) for the one check at its start. */
/**
 * sub_002FCE40
 * Original: 0x002FCE40 - 0x002FD07E (574 bytes, 144 insns)
 * Category: game_vtable
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_002FCE40(void)
{
    RECOMP_ENTRY(0x002FCE40u);
    /* The game leaves this out while its pause menu is up (0x7F1B12 2 or 3, below); behind the quick
     * menu's pause the Mission time ran on (#108) */
    if (s_pause) { esp += 4; return; }
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    int _cf = 0; /* carry flag */
    int _af = 0; /* auxiliary carry, for daa/das */
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_002FCE40: ;
    SET_LO8(eax, MEM8(0x7F1B12));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 2 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_002FCFC2; /* je: equal / zero */

loc_002FCE4D: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 3 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_002FCFC2; /* je: equal / zero */

loc_002FCE55: ;
    SET_LO8(eax, MEM8(0x7F1B11));
    PUSH32(esp, ebx);
    SET_LO8(ebx, 1);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_002FCFC1; /* je: equal / zero */

loc_002FCE65: ;
    ecx = 0xA;
    PUSH32(esp, 0x002FCE6Fu); RECOMP_ABI_CALL(0x000324C0u, sub_000324C0); /* call 0x000324C0 */

loc_002FCE6F: ;
    _fa = (uint32_t)(MEM8(0xD52B4C)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0xD52B4C), 0xFF (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_002FCE85; /* jne: not equal / not zero */

loc_002FCE78: ;
    MEM8(0xD52B4C) = LO8(ebx);
    PUSH32(esp, 0x002FCE83u); RECOMP_ABI_CALL(0x00206AA0u, sub_00206AA0); /* call 0x00206AA0 */

loc_002FCE83: ;
    goto loc_002FCEAB;

loc_002FCE85: ;
    ecx = ZX8(MEM8(0xDB932E));
    { int64_t _r = (int64_t)(int32_t)(ecx) * (int64_t)(int32_t)(0x78);
      ecx = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    _fa = (uint32_t)(MEM32(ecx + 0xD219A8)) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test MEM32(ecx + 0xD219A8), eax (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (TEST_Z(_fa, _fb)) goto loc_002FCEAB; /* je: equal / zero */

loc_002FCE97: ;
    _cf = 0; /* logical op clears CF */
    MEM8(0xD52B4C) = MEM8(0xD52B4C) ^ LO8(ebx);
    _fa = (uint32_t)(MEM8(0xD52B4C)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    if ((_fa == 0)) goto loc_002FCEA6; /* je: equal / zero */

loc_002FCE9F: ;
    PUSH32(esp, 0x002FCEA4u); RECOMP_ABI_CALL(0x00206AA0u, sub_00206AA0); /* call 0x00206AA0 */

loc_002FCEA4: ;
    goto loc_002FCEAB;

loc_002FCEA6: ;
    PUSH32(esp, 0x002FCEABu); RECOMP_ABI_CALL(0x00206B10u, sub_00206B10); /* call 0x00206B10 */

loc_002FCEAB: ;
    eax = ZX8(MEM8(0x5893E8));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(4) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 4 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_002FCFC1; /* ja: above (unsigned >) */

loc_002FCEBB: ;
    { uint32_t _jt = MEM32(eax * 4 + 0x2FCFC4); /* switch: 5 entries, 4 targets */
    if (_jt == 0x002FCEC2u) goto loc_002FCEC2;
    if (_jt == 0x002FCFA1u) goto loc_002FCFA1;
    if (_jt == 0x002FCFAEu) goto loc_002FCFAE;
    if (_jt == 0x002FCFB4u) goto loc_002FCFB4;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_002FCEC2: ;
    PUSH32(esp, 0x002FCEC7u); RECOMP_ABI_CALL(0x002FD080u, sub_002FD080); /* call 0x002FD080 */

loc_002FCEC7: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_002FCFC1; /* je: equal / zero */

loc_002FCECF: ;
    SET_LO8(ecx, MEM8(0xD532EA));
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(ebx) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_002FCF7D; /* jne: not equal / not zero */

loc_002FCEDF: ;
    SET_LO16(ecx, MEM16(0xD53300));
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fb = (uint32_t)(0x200) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ecx), 0x200 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_002FCEFC; /* jae: above or equal (unsigned >=) */

loc_002FCEED: ;
    edx = ZX16(LO16(ecx));
    _fa = (uint32_t)(MEM8(edx + 0x589AA4)) & 0xFFu; _fb = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(edx + 0x589AA4), LO8(eax) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_002FCF81; /* ja: above (unsigned >) */

loc_002FCEFC: ;
    edx = MEM32(0xD52B50);
    MEM32(0x5893D4) = edx;
    _cf = 0; /* logical op clears CF */
    edx = edx | 0xFFFFFFFFu;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fb = (uint32_t)(0x200) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ecx), 0x200 (16-bit) */
    _cf = (int)(_fa < _fb);
    MEM32(0x5893E0) = edx;
    MEM32(0x5893E4) = edx;
    edx = MEM32(0x5893F0);
    MEM32(0x5893D0) = eax;
    MEM32(0x5893D8) = eax;
    MEM32(0x5893DC) = eax;
    MEM8(0x5893E8) = LO8(ebx);
    MEM32(0xDB9330) = eax;
    MEM32(0xDB9324) = eax;
    MEM32(0xD69F58) = eax;
    MEM32(0xD6A120) = edx;
    MEM32(0xD52B50) = eax;
    MEM8(0xD532EA) = LO8(eax);
    if (CMP_AE(_fa, _fb)) goto loc_002FCFBB; /* jae: above or equal (unsigned >=) */

loc_002FCF58: ;
    eax = ZX16(LO16(ecx));
    MEM8(eax + 0x5898A4) = 2;
    SET_LO8(ecx, MEM8(eax + 0x589AA4));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0xFF (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_002FCFBB; /* jae: above or equal (unsigned >=) */

loc_002FCF6D: ;
    SET_LO8(ecx, LO8(ecx) + 1);
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fb = (_fa == 0x80u); /* inc result/SF/OF; CF unchanged */
    MEM8(0x7F1A95) = LO8(ebx);
    MEM8(eax + 0x589AA4) = LO8(ecx);
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_002FCF7D: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(eax) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_002FCFC1; /* je: equal / zero */

loc_002FCF81: ;
    MEM8(0xD532EA) = LO8(eax);
    MEM32(0xD52B50) = eax;
    MEM32(0xD69F58) = eax;
    MEM32(0xD6A120) = eax;
    MEM32(0xD532FC) = eax;
    MEM32(0xD6A110) = eax;
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_002FCFA1: ;
    MEM8(0x5893E8) = 2;
    MEM8(0x7F1A95) = LO8(ebx);

loc_002FCFAE: ;
    POP32(esp, ebx);
    goto loc_002FCFE0;

loc_002FCFB4: ;
    MEM8(0x5893E8) = 0;

loc_002FCFBB: ;
    MEM8(0x7F1A95) = LO8(ebx);

loc_002FCFC1: ;
    POP32(esp, ebx);

loc_002FCFC2: ;
    esp += 4; return; /* ret */

    { uint32_t _al = LO8(eax), _old = _al; int _ocf = _cf; _cf = 0;
      if ((_al & 0xFu) > 9u || _af) { _cf = _ocf || (_al < 6u); _al = (_al - 6u) & 0xFFu; _af = 1; } else _af = 0;
      if (_old > 0x99u || _ocf) { _al = (_al - 0x60u) & 0xFFu; _cf = 1; }
      SET_LO8(eax, _al); _fa = _al; _fas = (int32_t)(int8_t)_al; } /* das */
    _fb = (uint32_t)(HI8(edx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _af = (int)((((MEM8(edi + ecx * 8 + -810287057)) ^ (HI8(edx)) ^ (MEM8(edi + ecx * 8 + -810287057) + HI8(edx))) >> 4) & 1);
    _cf = (int)((((uint64_t)(MEM8(edi + ecx * 8 + -810287057)) + (uint64_t)(HI8(edx))) >> 8) & 1);
    MEM8(edi + ecx * 8 + -810287057) = MEM8(edi + ecx * 8 + -810287057) + HI8(edx);
    _fa = (uint32_t)(MEM8(edi + ecx * 8 + -810287057)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    { uint32_t _al = LO8(eax), _old = _al; int _ocf = _cf; _cf = 0;
      if ((_al & 0xFu) > 9u || _af) { _cf = _ocf || (_al < 6u); _al = (_al - 6u) & 0xFFu; _af = 1; } else _af = 0;
      if (_old > 0x99u || _ocf) { _al = (_al - 0x60u) & 0xFFu; _cf = 1; }
      SET_LO8(eax, _al); _fa = _al; _fas = (int32_t)(int8_t)_al; } /* das */
    _fb = (uint32_t)(LO8(ecx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _af = (int)((((HI8(eax)) ^ (LO8(ecx)) ^ (HI8(eax) + LO8(ecx))) >> 4) & 1);
    _cf = (int)((((uint64_t)(HI8(eax)) + (uint64_t)(LO8(ecx))) >> 8) & 1);
    SET_HI8(eax, HI8(eax) + LO8(ecx));
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    /* int3: debug-trap slide byte, stepped over */
    /* int3: debug-trap slide byte, stepped over */
    /* int3: debug-trap slide byte, stepped over */
    /* int3: debug-trap slide byte, stepped over */
    /* int3: debug-trap slide byte, stepped over */
    /* int3: debug-trap slide byte, stepped over */
    /* int3: debug-trap slide byte, stepped over */

loc_002FCFE0: ;
    SET_LO8(eax, MEM8(0xD532EA));
    SET_LO8(ecx, 3);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ecx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ecx) (8-bit) */
    _cf = (int)(_fa < _fb);
    SET_LO8(edx, 1);
    if (CMP_NE(_fa, _fb)) goto loc_002FD01A; /* jne: not equal / not zero */

loc_002FCFED: ;
    SET_LO16(eax, MEM16(0xD53300));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x200) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x200 (16-bit) */
    _cf = (int)(_fa < _fb);
    MEM8(0x5893E8) = LO8(ecx);
    MEM8(0xD532EA) = 0;
    if (CMP_AE(_fa, _fb)) goto loc_002FD051; /* jae: above or equal (unsigned >=) */

loc_002FD006: ;
    eax = ZX16(LO16(eax));
    MEM8(eax + 0x5898A4) = LO8(ecx);
    MEM16(0xD53300) = 0x200;
    goto loc_002FD051;

loc_002FD01A: ;
    SET_LO8(ecx, 4);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ecx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ecx) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_002FCFED; /* je: equal / zero */

loc_002FD020: ;
    SET_LO8(eax, MEM8(0x4B75DE));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_002FD051; /* jne: not equal / not zero */

loc_002FD029: ;
    SET_LO8(eax, MEM8(0x4B7640));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x20 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_002FD051; /* jae: above or equal (unsigned >=) */

loc_002FD032: ;
    ecx = ZX8(LO8(eax));
    { int64_t _r = (int64_t)(int32_t)(ecx) * (int64_t)(int32_t)(0xDD0);
      ecx = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    _fa = (uint32_t)(MEM8(ecx + 0x4968B1)) & 0xFFu; _fb = (uint32_t)(LO8(edx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(ecx + 0x4968B1), LO8(edx) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_002FD051; /* jne: not equal / not zero */

loc_002FD043: ;
    _fa = (uint32_t)(MEM8(0xD6A10A)) & 0xFFu; _fb = (uint32_t)(LO8(edx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0xD6A10A), LO8(edx) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_002FD051; /* jne: not equal / not zero */

loc_002FD04B: ;
    MEM32(0x5893D0) = MEM32(0x5893D0) + 1;
    _fa = (uint32_t)(MEM32(0x5893D0)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_002FD051: ;
    eax = MEM32(0x5893F0);
    _fb = (uint32_t)(MEM32(0xD6A120)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _af = (int)((((eax) ^ (MEM32(0xD6A120)) ^ (eax - MEM32(0xD6A120))) >> 4) & 1);
    _cf = (int)((uint32_t)(eax) < (uint32_t)(MEM32(0xD6A120)));
    eax = eax - MEM32(0xD6A120);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(0x5893D8) = eax;
    SET_LO8(eax, MEM8(0x58A386));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM8(0x7F1A95) = LO8(edx);
    if (CMP_EQ(_fa, _fb)) goto loc_002FD07D; /* je: equal / zero */

loc_002FD070: ;
    ecx = ZX16(MEM16(0x5895DA));
    MEM32(0x5893DC) = ecx;

loc_002FD07D: ;
    esp += 4; return; /* ret */

}

