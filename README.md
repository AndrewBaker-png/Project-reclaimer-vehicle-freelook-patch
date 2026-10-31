# Project Reclaimer Freelook

Hold **F8** in a Halo 3 vehicle to look around independently of the native camera. Release F8 to recenter. This is the standalone **revision 30** free-look build for **Project Reclaimer 0.9.23** on Windows x64.

**Experimental prerelease:** multiplayer behavior has not yet been verified with a second player. Use a private test lobby with anti-cheat disabled through the host's supported settings. The loader does not change anti-cheat settings or bypass enforcement.

## Download and install

1. Open [Releases](https://github.com/AndrewBaker-png/Project-Reclaimer-Freelook/releases) and download `Halo3-Freelook-Only-MP-r30.zip`. The GitHub source-code ZIP is for developers.
2. Extract the entire mod ZIP to a writable folder. Keep all included files together.
3. Fully exit any previous Reclaimer session. A stopped DLL remains resident until the client exits.
4. Run `Start-Reclaimer-Freelook-Only.cmd`, then open Reclaimer normally. The one-shot loader waits up to 30 minutes for the Halo 3 engine. It can also be started after entering the game.
5. Enter a vehicle, hold **F8**, and move the mouse. Release **F8** to recenter.

Only the tester needs this DLL. Other players can observe with an unmodified client, but everyone still needs the map/mod required by the server.

## Controls and configuration

| Control | Action |
| --- | --- |
| Hold F8 | Look around in a vehicle |
| Release F8 | Recenter the view |
| Ctrl+Shift+F10 | Stop the active runtime |
| `Stop-Reclaimer-Freelook-Only.cmd` | Cancel a pending loader and stop an active runtime |

Edit `freelook.ini` beside `Halo3Freelook.exe` to change the settings. The defaults use inverted vertical look, native sensitivity, 40 ms smoothing, and a 180 ms return. Restart the client and reload the mod after changing settings or switching builds.

The loader is session-only. It does not install a service or startup entry; run the start helper again for each new client session. To uninstall, stop the loader, exit Reclaimer, and remove the extracted mod folder.

## Compatibility and limitations

- The loader checks the supported Reclaimer executable, Halo 3 engine hashes, and three hook sites. It refuses incompatible builds and other resident free-look revisions. It does not automatically retry a compatibility refusal or failed load.
- Revision 30 retains the native third-person vehicle view. Cockpit models, visor/HUD overlays, F7/F9 controls, graphics hooks, and custom map assets are outside this build.
- MCC support exists in the underlying launcher but has not been flight-tested for this revision. This prerelease targets Reclaimer 0.9.23.
- Public-server compatibility and enforcement behavior have not been verified. Do not treat this prerelease as a verified public-server mod.

`loader-state.json` records loader status. `loader.log` records compatibility and load checks; `freelook.log` records DLL activation. If loading is refused, check these files and the supported version before restarting.

## Validation

The included tests cover camera math, native hook preservation, private camera copying, and the actual F8 runtime's hold/release, orbit/FOV isolation, exit, death, focus, menus, vehicle identity, and camera turnover. The binary inspection checks for graphics/network imports and cockpit/HUD file dependencies. These checks do not establish multiplayer compatibility.

For two-player testing, repeat as host and joining client; observe aiming/steering from the other client; check passenger/turret seats, exit/re-entry, death/respawn, map reload/reconnect, menus, focus changes, and stopping the runtime. Report crashes, disconnects, rubber-banding, aim discrepancies, and stuck views with reproduction steps. Review logs for personal information before posting them.

## Build from source

Requirements: Windows x64, PowerShell, Node.js, and **Zig 0.14.1**. The MinHook source required by this build is included under `third_party/minhook`.

Download the [official Zig 0.14.1 Windows x64 archive](https://ziglang.org/download/0.14.1/zig-x86_64-windows-0.14.1.zip). Verify its SHA-256 against `build-manifest.json`, then extract it under `tools/zig-x86_64-windows-0.14.1/`, or pass its executable path:

```powershell
.\Build-Freelook-Only.ps1
# Or use an existing installation:
.\Build-Freelook-Only.ps1 -ZigPath 'C:\path\to\zig.exe'
```

The script compiles and runs four test programs, builds the launcher and DLL, checks binary isolation, and creates `release/Halo3-Freelook-Only-MP-r30.zip` with a file-hash manifest. Runtime and launcher sources are unchanged from local source revision `073ff42`; this repository contains the standalone free-look files and a portable compiler-path option in the build script.

## Credits

Uses [MinHook](https://github.com/TsudaKageyu/minhook) and reference information from [HaloCheckpointManager](https://github.com/Burnt-o/HaloCheckpointManager). Their notices are included in `MINHOOK-LICENSE.txt` and `HCM-LICENSE.txt`. No game assets or game data are distributed. This is an unofficial mod and is not affiliated with Project Reclaimer, Microsoft, or Halo's developers.
