# GTA Trilogy DE Trainer

In-game trainer menus for **GTA III**, **GTA Vice City** and **GTA San Andreas** — The Definitive Edition.
Each game gets its own trainer that draws a menu on top of the game, in the style of the classic GTA IV
trainers. There's no separate app to run: you press **F11** in game and the menu appears.

> **Supported version: 1.112 (the latest Steam update, build 1.0.112.48699928).**
> On any other build the trainer detects the mismatch, writes a note to its log and stays switched off,
> so it can't crash an unsupported version.

## Download and install

1. Download `TrilogyTrainer-vX.Y.zip` from the [Releases](../../releases) page.
2. The zip has one folder per game. Copy the **contents** of each folder into that game's
   `Gameface\Binaries\Win64` folder, next to the game's exe:

   | Game | Copy into |
   |---|---|
   | GTA III | `GTA III - The Definitive Edition\Gameface\Binaries\Win64\` |
   | GTA Vice City | `GTA Vice City - The Definitive Edition\Gameface\Binaries\Win64\` |
   | GTA San Andreas | `GTA San Andreas - The Definitive Edition\Gameface\Binaries\Win64\` |

   Each folder contains `version.dll` (a small loader for `.asi` mods) and `TrilogyTrainer.<game>.asi`.
3. Start the game, load a save and press **F11**.

If you already use another ASI loader, skip our `version.dll` and just copy the `.asi` file to wherever your
loader reads mods from.

**To uninstall**, delete `version.dll`, `TrilogyTrainer.*.asi`, and the `.ini` / `.log` files the trainer created.

> **Antivirus warnings:** mod loaders work by being loaded into the game in place of a Windows DLL, which some
> antivirus programs flag as suspicious. The full source is in this repository if you'd rather build it yourself.

## Controls

| Key | Action |
|---|---|
| **F11** | Open / close the menu |
| Up / Down (or Num 8 / Num 2) | Move |
| Enter (or Num 5) | Select / toggle |
| Left / Right (or Num 4 / Num 6) | Change a value or category |
| Backspace (or Num 0) | Back |
| **F5** | Teleport to the waypoint marked on the map |
| **F6** | No-clip (fly) on / off: W A S D to move, Space / Ctrl up / down, Shift faster, Alt slower |
| **Num +** / **Num -** / **Num \*** | In a vehicle: speed boost / stop dead / jump |

Every key can be changed in `TrilogyTrainer.<game>.ini`, which the trainer creates next to itself the first
time it runs. Use names like `F7`, `Insert`, `Home`, `Num5`, `ADD`, `SUBTRACT`, a letter, or `NONE` to disable.

## Features

**All three games**

- **Player**: god mode, infinite armor, infinite stamina, never wanted, set wanted level, refill health and armor,
  add money, the game's player cheats, on-screen coordinates
- **Vehicles**: spawn any vehicle by category, vehicle god mode, auto-repair, repair, flip upright, repaint,
  speed boost / stop / jump keys, speedometer, the game's vehicle cheats
- **Weapons**: give any single weapon with ammo, infinite ammo, fast reload, the weapon-set cheats
- **Teleport**: teleport to the map waypoint, no-clip flying, preset places, three saved positions that are
  kept between sessions
- **World**: weather (set or freeze), time (set or freeze), game speed, the game's world cheats
- **Peds**: riot mode and "everyone attacks you" as real on/off switches (the original cheats in III and Vice City
  can't be undone), plus a "reset peds to normal" button for saves that still have riot mode in them

**GTA San Andreas**

- Stats editor: muscle, fat, stamina, lung capacity, sex appeal, respect, gambling, driving / bike / cycling /
  flying skill and every weapon skill
- Fireproof, plus all the San Andreas cheats sorted into menus (traffic, gangs, themes, stats, mega jump,
  mega punch, jetpack, parachute and more)

**GTA Vice City**

- Fireproof
- Change skin to any regular character model, or back to Tommy

## Notes

- This is for single-player only.
- Saving while cheats are active works, but the game's own cheats can still be stored in your save, as they
  are without the trainer.
- Each trainer writes a log (`TrilogyTrainer.<game>.log`) next to itself. If something goes wrong, include it
  when you report the problem.

## Building from source

You need the free [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
(or any Visual Studio 2022) with the **Desktop development with C++** workload.

```bat
git clone --recursive https://github.com/VoxelNative/gta-trilogy-de-trainer.git
cd gta-trilogy-de-trainer
build.bat
```

The output goes to `build\`: `version.dll` and `TrilogyTrainer.SA.asi`, `.VC.asi`, `.III.asi`.
`package.ps1` builds the release zip.

### How it works

- `src/loader`: `version.dll` forwards every call to the real Windows DLL and loads `*.asi` files from the
  game folder.
- `src/core`: hooks DirectX 11 / 12 Present to draw the menu with Dear ImGui, reads keys from the game window,
  and runs trainer actions on the game's own thread through a hook on `CTimer::Update`.
- `src/sa` and `src/classic`: the San Andreas trainer and the Vice City / GTA III trainer (one source file built
  twice). Every address and structure offset was found by reverse-engineering the 1.112 executables; they're
  listed at the top of each file.

## Credits

- Made by **VoxelNative**.
- [Dear ImGui](https://github.com/ocornut/imgui) by Omar Cornut (MIT licence).
- [MinHook](https://github.com/TsudaKageyu/minhook) by Tsuda Kageyu (BSD 2-clause licence).

Not affiliated with or endorsed by Rockstar Games or Take-Two Interactive. Grand Theft Auto and all related names
are trademarks of their respective owners.

## Licence

[MIT](LICENSE)
