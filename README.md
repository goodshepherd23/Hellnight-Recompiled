# Hell Night: Dark Messiah Recompiled

A native PC build of **Hellnight** (PlayStation, 1999; *Dark Messiah* in Japan),
made with the [PSXRecomp](https://github.com/mstan/psxrecomp) static recompiler
and the shared [recomp-ui](https://github.com/mstan/recomp-ui) launcher.

You must supply your own **Hellnight (Europe)** disc, serial **SLES-01562**, as a
CUE/BIN dump. This repository and its release packages contain **no game disc, no
BIOS dump, no generated game code and no saves**. The game code is generated on
your machine from your disc.

No retail BIOS is needed: the build uses the bundled
[OpenBIOS](https://github.com/grumpycoders/pcsx-redux) image.

## Setup

1. Download the release ZIP for your platform and extract all of it into a
   writable folder.
2. Start `Hellnight_Recompiled` (`.exe` on Windows). The program and the repository
   keep the original short name. The launcher lists the game as **Dark Messiah**
   and the game window is titled *Hell Night: Dark Messiah Recompiled*.
3. Select your `Hellnight (Europe).cue` in the setup wizard. Keep the CUE and its
   BIN together.
4. Run **Generate & rebuild** and wait. The game starts when it finishes.

On Windows the setup wizard can download portable build tools. On Linux and macOS,
install CMake, Ninja, Python 3 and a C/C++ compiler first.

Expected disc (data track 01): 633,786,384 bytes, MD5
`01ce6367485ab848973be612df58b5b9`.

## Building from source

```bash
git clone --recursive https://github.com/goodshepherd23/Hellnight-Recompiled
cd Hellnight-Recompiled
cp bios/openbios.bin psxrecomp/bios/openbios.bin
for p in patches/*.patch; do git -C psxrecomp apply "../$p"; done
python psxrecomp/psxrecomp_cli.py generate --config game.toml --project-root . --disc "/path/to/Hellnight (Europe).cue" --force-emitters
python psxrecomp/psxrecomp_cli.py rebuild --config game.toml --project-root . --build-dir build-release --target psx-runtime --exe-basename Hellnight_Recompiled --no-pgo
```

The `cp` step is needed because the pinned framework revision does not include
the OpenBIOS image; `bios/openbios.bin` here is the MIT-licensed build pinned in
`psxrecomp/bios/OpenBIOS.toml` (SHA-256 `fabe498f…1c57`), with its notice in
`bios/OpenBIOS.LICENSE`.

The patches fix two things in the pinned framework:

- `0001` keeps FMVs from sitting low in the frame. The European disc runs in NTSC
  mode but keeps a PAL-tuned vertical display range.
- `0002` makes the launcher's **PGO optimize** work on Windows. Without it, the
  training run fails with `[WinError 5] Access is denied` and writes no profile.
  `CMakeLists.txt` also wires up `PSX_PGO`, which this framework revision ignores.

`--force-emitters` is required. Without it, `generate` can pick up stale prebuilt
emitters that leave the build with no BIOS backend.

## Status

Version 0.1.3 is an early release. What was tested on Windows x64 (Intel Iris Xe):

- Boots through the intro FMV to the title screen
- New Game starts. In-game dialogue, the examine cursor and room transitions work
- Holds about 100% speed at 60 fps (the European disc runs its GPU in 60 Hz mode)
- FMVs fill the frame. The European disc keeps a PAL-tuned vertical range in NTSC
  mode, which on a real NTSC console shows the picture low. This build presents
  the range directly instead. The Konami logo screen is taller than that range,
  so its bottom edge is cut off, the same as on an NTSC console. The save menu
  is cut off at the bottom the same way.
- Saving to a memory card and loading the save back both work. The save file
  (`BESLES-01562-DM0`) is written to `saves/card1.mcd`.

**Not tested yet:** a full playthrough, and the Linux and macOS packages. Please open an issue if you hit a crash, a hang or a
graphical bug, and include the `psx_last_run_report.json` written next to the
executable.

**PGO optimize** (in the launcher) works on Windows as of 0.1.1. It rebuilds the
game with profile-guided optimization: two 60-second training runs, then an
optimized build. Leave the game window alone while it trains.

## Cheats

The launcher's **Mods** page has a **Cheats** group. Tick what you want, then press
PLAY. All are off by default.

| Cheat | Effect | GameShark code |
|---|---|---|
| OK Status | Keeps your status at OK | `800AD840 0000` |
| Infinite Ammo | Your companion's ammunition never runs out | `800ADD54 0001` |
| Choose Companion | Forces who follows you: Naomi, Kamiya, Ivanoff, Rene or The Monster | `800ADD50 000N` + `800ADD58 0000` |

The codes are for the European disc (SLES-01562) and are credited to DAVIN THE RAVEN
on [psxdatacenter](https://psxdatacenter.com/games/P/H/SLES-01562.html). They are
applied every frame by a small plugin (`src/hellnight_mods.c`) through the
framework's mod system (`mods/packages/hellnight.cheats`).

Tested on Windows: OK Status and Infinite Ammo write their values every frame and
the game plays normally with them on. Choose Companion has not been tried in play.
It overrides the story's own choice, so save before using it.

## Walkthrough

Stuck? [Hellnight PS1 Longplay (Naomi Ending)](https://www.youtube.com/watch?v=CheV_hI2-xE)
by LaTanaDiMrX is a full playthrough of the original PlayStation game. It was not
recorded on this build, so use it as a guide to the game, not as proof that every
area works here.

## Credits and licenses

- Framework: [PSXRecomp](https://github.com/mstan/psxrecomp), PolyForm
  Noncommercial 1.0.0. This project is free and noncommercial.
- Launcher: [recomp-ui](https://github.com/mstan/recomp-ui), MIT.
- Framework revision and release pipeline: the
  [Alexbeav](https://github.com/Alexbeav/psxrecomp) fork used by
  [Alexbeav's PS1 ports](https://github.com/Alexbeav/psxrecomp-ports).
- Their licenses and dependency notices are in the `psxrecomp/` and `recomp-ui/`
  directories.

This project was made with AI assistance (Claude).

Hellnight is © Atlus 1999. The European release was
published by Konami. All trademarks belong to their owners. This project is not
affiliated with or endorsed by any of them.
