/* Hellnight cheats: trusted mod plugins for mods/packages/hellnight.cheats.
 *
 * Each feature is a GameShark type-80 code (16-bit constant write) for the
 * European disc SLES-01562, re-applied on every guest VBlank the way a cheat
 * cartridge does. Codes by DAVIN THE RAVEN (psxdatacenter.com). */

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

PSX_MOD_CONSTRUCTOR(hellnight_register_cheats) {
    psx_mod_register_vblank_plugin("hellnight.cheat.ok-status", hn_ok_status);
    psx_mod_register_vblank_plugin("hellnight.cheat.infinite-ammo", hn_infinite_ammo);
    psx_mod_register_vblank_plugin("hellnight.cheat.companion", hn_companion);
}
