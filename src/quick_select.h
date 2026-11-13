/* Quick selection: guest-owned inventory snapshots, host-owned carousel input.
 */
#ifndef NGB_QUICK_SELECT_H
#define NGB_QUICK_SELECT_H
#include <stdint.h>
#define NGB_QUICK_CAP 128
enum { NGB_QUICK_MELEE = 1, NGB_QUICK_PROJECTILES, NGB_QUICK_NINPO, NGB_QUICK_ITEMS,
       NGB_QUICK_CATEGORIES = 4 };
enum { Q_UP = 1, Q_DOWN = 2, Q_LEFT = 4, Q_RIGHT = 8, Q_CONFIRM = 16, Q_CANCEL = 32,
       Q_DIRECTIONS = 15 };
typedef struct
{
    uint16_t id;
    uint8_t count, usable, quantity, level;
    uint8_t other_slot;   /* listed with the melee weapons but equipped in another slot (the Spear Gun) */
    char art[24], name[64];
    char badge[8];   /* a bow's arrows, drawn in the card's corner: "plain", "fire", "core"; else empty */
} NgbQuickEntry;
typedef struct
{
    int category, freeze, count, index;
    uint16_t equipped;   /* the equipped weapon or Ninpo, the last item used: index's when made; 0xFFFF none */
    NgbQuickEntry entries[NGB_QUICK_CAP];
} NgbQuickView;
typedef struct
{
    int category, ready;
    NgbQuickView views[NGB_QUICK_CATEGORIES];
} NgbQuickMenu;
void ngb_quick_set_game_dir(const char *path);
void ngb_quick_open(const NgbQuickMenu *menu);
int ngb_quick_category(void);
void ngb_quick_poll(void);
int ngb_quick_event(const void *event);
int ngb_quick_take_open(void);
int ngb_quick_controls_active(void);
int ngb_quick_available(void);
void ngb_quick_refresh(const NgbQuickMenu *menu);
void ngb_quick_close(void);
int ngb_quick_wanted(void);
int ngb_quick_frozen(void);
#define NGB_QUICK_WIDTH 384
#define NGB_QUICK_HEIGHT 312
/* A small transparent, premultiplied BGRA layer; no game-frame readback. */
int ngb_quick_layer(uint32_t *pixels);
int ngb_quick_take_command(uint16_t *item, int *category);
int ngb_quick_take_navigation(void);
void ngb_quick_tick(void);
#endif
