/* Hellnight cheats: trusted mod plugins for mods/packages/hellnight.cheats.
 *
 * Each feature is a GameShark type-80 code (16-bit constant write) for the
 * European disc SLES-01562, re-applied on every guest VBlank the way a cheat
 * cartridge does. Codes by DAVIN THE RAVEN (psxdatacenter.com); the code
 * patches further down are from the DuckStation cheat database. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mod_plugins.h"

#define HN_STATUS_ADDR     0x800AD840u
#define HN_AMMO_ADDR       0x800ADD54u
#define HN_COMPANION_ADDR  0x800ADD50u
#define HN_COMPANION_AUX   0x800ADD58u

/* Set HN_CHEAT_LOG=<file path> to append, every ~5 s, the value each cheat
 * found before overwriting it. Off by default. */
static void hn_poke(unsigned slot, const char* what, uint32_t addr,
                    uint16_t value) {
    static int s_log = -1;
    static unsigned s_tick[4];
    if (s_log < 0) s_log = getenv("HN_CHEAT_LOG") ? 1 : 0;
    if (s_log && (s_tick[slot & 3u]++ % 300u) == 0u) {
        FILE* f = fopen(getenv("HN_CHEAT_LOG"), "a");
        if (f) {
            fprintf(f, "[hellnight-cheat] %s @%08X was %04X -> %04X\n", what,
                    (unsigned)addr, (unsigned)psx_mod_read_half(addr),
                    (unsigned)value);
            fclose(f);
        }
    }
    psx_mod_write_half(addr, value);
}

static void hn_ok_status(void) {
    if (!psx_mod_game_started()) return;
    hn_poke(0, "ok-status", HN_STATUS_ADDR, 0x0000);
}

static void hn_infinite_ammo(void) {
    if (!psx_mod_game_started()) return;
    hn_poke(1, "infinite-ammo", HN_AMMO_ADDR, 0x0001);
}

static void hn_companion(void) {
    static const struct { const char* name; uint16_t id; } kWho[] = {
        { "naomi", 1 }, { "kamiya", 2 }, { "ivanoff", 3 },
        { "rene", 4 }, { "monster", 5 },
    };
    char who[16];
    uint16_t id = 1;
    if (!psx_mod_game_started()) return;
    if (psx_mod_option_value("hellnight.cheats", "companion", "who",
                             who, sizeof(who))) {
        for (unsigned i = 0; i < sizeof(kWho) / sizeof(kWho[0]); i++)
            if (strcmp(who, kWho[i].name) == 0) id = kWho[i].id;
    }
    hn_poke(2, "companion", HN_COMPANION_ADDR, id);
    hn_poke(3, "companion-aux", HN_COMPANION_AUX, 0x0000);
}

/* Code patches from the DuckStation cheat database entry for SLES-01562.
 * Each replaces one branch in the boot executable. They go through
 * psx_mod_write_code_word so the recompiled function picks the change up, and
 * are only applied while the original instruction is still in place. */
#define HN_FULLMAP_PC      0x80025724u
#define HN_FULLMAP_ORIG    0x10600056u   /* beq v1, zero, +0x56 */
#define HN_FULLMAP_PATCH   0x24000056u   /* never taken */
#define HN_WALLS_PC        0x800598ACu
#define HN_WALLS_ORIG      0x14400006u   /* bne v0, zero, +6 */
#define HN_WALLS_PATCH     0x10000006u   /* always taken */
#define HN_MONSTER1_PC      0x80056258u
#define HN_MONSTER1_ORIG    0x10400006u   /* beq v0, zero, +6 */
#define HN_MONSTER1_PATCH   0x10000006u   /* always taken */
#define HN_MONSTER2_PC      0x80055E68u
#define HN_MONSTER2_ORIG    0x1440004Du   /* bne v0, zero, +0x4D */
#define HN_MONSTER2_PATCH   0x1000004Du   /* always taken */

static void hn_patch_code(const char* what, uint32_t pc, uint32_t orig,
                          uint32_t patch) {
    if (!psx_mod_game_started()) return;
    if (psx_mod_read_word(pc) != orig) return;
    psx_mod_write_code_word(pc, patch);
    if (getenv("HN_CHEAT_LOG")) {
        FILE* f = fopen(getenv("HN_CHEAT_LOG"), "a");
        if (f) {
            fprintf(f, "[hellnight-cheat] %s @%08X %08X -> %08X\n", what,
                    (unsigned)pc, (unsigned)orig, (unsigned)patch);
            fclose(f);
        }
    }
}

static void hn_full_map(void) {
    hn_patch_code("full-map", HN_FULLMAP_PC, HN_FULLMAP_ORIG, HN_FULLMAP_PATCH);
}

static void hn_walk_through_walls(void) {
    hn_patch_code("walk-through-walls", HN_WALLS_PC, HN_WALLS_ORIG,
                  HN_WALLS_PATCH);
}

static void hn_monster_harmless(void) {
    hn_patch_code("monster-harmless", HN_MONSTER1_PC, HN_MONSTER1_ORIG,
                  HN_MONSTER1_PATCH);
    hn_patch_code("monster-harmless", HN_MONSTER2_PC, HN_MONSTER2_ORIG,
                  HN_MONSTER2_PATCH);
}

/* Widescreen (mods/packages/hellnight.widescreen). The framework keeps the
 * generic Settings aspect at 4:3 on PSX and leaves widescreen to a mod, so this
 * activation callback selects the wide display aspect before the window opens.
 * The runtime then squashes the GTE projection to widen the field of view. */
static void hn_widescreen(void) {
    char aspect[8];
    if (psx_mod_option_value("hellnight.widescreen", "widescreen", "aspect",
                             aspect, sizeof(aspect)) &&
        strcmp(aspect, "21:9") == 0) {
        psx_mod_set_fixed_display_aspect(21u, 9u);
        return;
    }
    psx_mod_set_fixed_display_aspect(16u, 9u);
}

PSX_MOD_CONSTRUCTOR(hellnight_register_cheats) {
    psx_mod_register_vblank_plugin("hellnight.cheat.ok-status", hn_ok_status);
    psx_mod_register_vblank_plugin("hellnight.cheat.infinite-ammo", hn_infinite_ammo);
    psx_mod_register_vblank_plugin("hellnight.cheat.companion", hn_companion);
    psx_mod_register_activation_plugin("hellnight.widescreen", hn_widescreen);
    psx_mod_register_vblank_plugin("hellnight.cheat.full-map", hn_full_map);
    psx_mod_register_vblank_plugin("hellnight.cheat.walk-through-walls",
                                   hn_walk_through_walls);
    psx_mod_register_vblank_plugin("hellnight.cheat.monster-harmless",
                                   hn_monster_harmless);
}
