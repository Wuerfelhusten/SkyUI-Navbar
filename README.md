# Navigation Bar for SkyUI

Navigation Bar for SkyUI adds a compact right-side navigation bar for Skyrim menus. It supports mouse, keyboard, and controller input.

## Requirements

- [SKSE64](https://www.nexusmods.com/skyrimspecialedition/mods/30379)
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SkyUI](https://www.nexusmods.com/skyrimspecialedition/mods/12604)

The release uses FLATRIM for Skyrim SE and AE in one DLL. It was tested on 1.6.1170 and 1.7.104. Version 1.5.97 is supported but not actively tested. VR is not supported.

## Installation

Install the release archive as a normal Data mod with your mod manager and launch Skyrim through SKSE. No original menu SWFs are replaced.

## Supported menus

Inventory, Magic, Map, and Skills are included. Optional integrations are available for [The Dragonborn's Bestiary](https://www.nexusmods.com/skyrimspecialedition/mods/123521), [Custom Skills Menu](https://www.nexusmods.com/skyrimspecialedition/mods/62423), [Character Menu SE](https://www.nexusmods.com/skyrimspecialedition/mods/173074), and [Achievement Injector](https://www.nexusmods.com/skyrimspecialedition/mods/126220). Missing optional mods are skipped automatically. The Custom Skills tab opens its selector before the selected skill tree.

## Controls

| Device | Switch menus | Selection view | Confirm | Cancel |
| --- | --- | --- | --- | --- |
| Mouse | Click a tab | Not applicable | Click a tab | Not applicable |
| Keyboard | Tap Left Alt | Hold Left Alt | E or Enter | Alt or Escape |
| Controller | Tap Back or View | Hold Back or View | D-pad or Left Stick, then A | Back or View or B |

The controller and keyboard switch keys follow SkyUI's Inventory and Magic switch binding. Menu transitions remain active during normal opening and closing.

## Configuration

Edit `Data/SKSE/Plugins/SkyUINavbar.json` with Skyrim closed. JSON comments are supported. The configuration controls menu order and visibility, sidebar scale, button hints, and fade settings.

## Build

The build requires Visual Studio, CMake, vcpkg, Java, the [FFDec CLI](https://github.com/jindrapetrik/jpexs-decompiler), Node.js, and PowerShell. Adobe Flash is not required.

```powershell
cmake --preset FLATRIM -DFFDEC_ROOT="C:/Tools/FFDec"
cmake --build --preset Flatrim-Release --parallel
ctest --test-dir build/FLATRIM -C Release --output-on-failure
```

## Support

Report issues on [GitHub](https://github.com/Wuerfelhusten/SkyUI-Navbar/issues) or join the [Discord server](https://discord.gg/pqEHdWDf8z). Include your Skyrim runtime, affected menu, input device, and `SkyUINavbar.log`.

## Credits and license

Icon attribution is listed in [THIRD_PARTY_NOTICE.md](THIRD_PARTY_NOTICE.md). The source code is licensed under [GPL v3](LICENSE).
