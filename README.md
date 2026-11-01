# Project Reclaimer Aerial Vehicle Freelook Patch

Hold **F8** in a Halo 3 vehicle to look around independently of the native camera. Release F8 to recenter. This is the standalone **revision 30** free-look build for **Project Reclaimer 0.9.23** on Windows x64.

**Tested in live multiplayer:** the latest builds have been successfully used in live **Huge Grass** multiplayer matches on Project Reclaimer 0.9.23. The release remains a prerelease as broader compatibility testing continues. The loader does not change anti-cheat settings or bypass enforcement.

## Download and install

1. Open the [r30 Auto Launch prerelease](https://github.com/AndrewBaker-png/Project-reclaimer-aerial-vehicle-freelook-patch/releases/tag/r30-auto-launch) and download `Halo3-Freelook-Only-MP-r30-auto-launch.zip` under **Assets**. The GitHub source-code ZIP is for developers.
2. Extract the entire mod ZIP to a short, writable folder, such as `C:\Mods\Freelook`. Keep all included files together.
3. Fully exit any previous Reclaimer session. A stopped DLL remains resident until the client exits.
4. Double-click **`Play-Reclaimer-with-Freelook.exe`**. On first use, choose your Project Reclaimer client EXE in the file picker. It remembers the location in `reclaimer-launcher.ini` and starts the freelook loader followed by Reclaimer on every launch. You do not need to type commands. You can create a normal Windows desktop shortcut to this EXE.
5. Enter a vehicle, hold **F8**, and move the mouse. Release **F8** to recenter.

Only players who want freelook need this DLL. Other players can use an unmodified client, but everyone still needs the map/mod required by the server.

Keep the entire extracted package together. If you move Reclaimer and the saved EXE no longer exists, the launcher asks for its location again. To choose a different installed copy, delete `reclaimer-launcher.ini` and reopen the launcher. The launcher also accepts `--choose-client`. The existing `Start-Reclaimer-Freelook-Only.cmd` remains available to arm the loader separately, including after entering the game. The loader waits up to 30 minutes for the Halo 3 engine.

## Controls and configuration

| Control | Action |
| --- | --- |
| Hold F8 | Look around in a vehicle |
| Release F8 | Recenter the view |
| Ctrl+Shift+F10 | Stop the active runtime |
| `Stop-Reclaimer-Freelook-Only.cmd` | Cancel a pending loader and stop an active runtime |

Edit `freelook.ini` beside `Halo3Freelook.exe` to change the settings. The defaults use inverted vertical look, native sensitivity, 40 ms smoothing, and a 180 ms return. Restart the client and reload the mod after changing settings or switching builds.

The r30 Auto Launch prerelease includes the combined game launcher plus the camera, cancellation, and path-safety fixes. Stop waits for a pending load to finish and for the runtime to acknowledge that its hooks are disabled. If confirmation fails, the helper returns an error and asks you to close the client. Older r30 runtimes cannot acknowledge shutdown; fully exit the old client before upgrading. Installation paths that would exceed the launcher's path buffer are refused with an instruction to use a shorter folder.

The loader is session-only. It does not install a service or startup entry; use `Play-Reclaimer-with-Freelook.exe` for each new client session. Opening the original Reclaimer EXE directly still requires the separate start helper. To uninstall, stop the loader, exit Reclaimer, and remove the extracted mod folder.

## Compatibility and limitations

- The loader checks the supported Reclaimer executable, Halo 3 engine hashes, and three hook sites. It refuses incompatible builds and other resident free-look revisions. It does not automatically retry a compatibility refusal or failed load.
- Revision 30 retains the native third-person vehicle view. Cockpit models, visor/HUD overlays, F7/F9 controls, graphics hooks, and custom map assets are outside this build.
- MCC support exists in the underlying launcher but has not been flight-tested for this revision. This prerelease targets Reclaimer 0.9.23.
- Live multiplayer testing covers Huge Grass on the supported Reclaimer build. This does not establish compatibility with every map, server configuration, vehicle, or seat. Follow the server's mod rules and supported anti-cheat settings.

`loader-state.json` records loader status. `loader.log` records compatibility and load checks; `freelook.log` records DLL activation. If loading is refused, check these files and the supported version before restarting.

## Validation

The project maintainer reports successful use of the latest builds in live Huge Grass multiplayer matches. This is gameplay evidence alongside the automated checks below; a complete host/client, vehicle, and seat test matrix has not been recorded.

The included automated tests cover camera math, native hook preservation, private camera copying, and the actual F8 runtime's hold/release, orbit/FOV isolation, exit, death, focus, menus, vehicle identity, and camera turnover. The binary inspection checks for graphics/network imports and cockpit/HUD file dependencies.

Audit regressions revisit 128 camera buffers, restore every pending pose after release, preserve newer native poses, and exercise retired camera addresses without accessing them outside their callbacks. Separate tests cover path boundaries, shutdown acknowledgment, loader cancellation during injection, pending-load failures, and stopping the actual DLL during startup in a test process without the game engine. Loader tests use a fake executable and isolated event names. The combined launcher test uses a fake client to verify saved locations, loader-before-game ordering, paths with spaces, repeated launches, and the client working directory.

To expand multiplayer coverage, test as host and joining client; observe aiming/steering from another client; check passenger/turret seats, exit/re-entry, death/respawn, map reload/reconnect, menus, focus changes, and stopping the runtime. Include the map, vehicle/seat, host or joining role, and build when reporting results. Report crashes, disconnects, rubber-banding, aim discrepancies, and stuck views with reproduction steps. Review logs for personal information before posting them.

## Build from source

Requirements: Windows x64, PowerShell, Node.js, and **Zig 0.14.1**. The MinHook source required by this build is included under `third_party/minhook`.

Download the [official Zig 0.14.1 Windows x64 archive](https://ziglang.org/download/0.14.1/zig-x86_64-windows-0.14.1.zip). Verify its SHA-256 against `build-manifest.json`, then extract it under `tools/zig-x86_64-windows-0.14.1/`, or pass its executable path:

```powershell
.\Build-Freelook-Only.ps1
# Or use an existing installation:
.\Build-Freelook-Only.ps1 -ZigPath 'C:\path\to\zig.exe'
```

The script compiles and runs six native test programs plus the PowerShell loader scenarios and an actual DLL lifecycle test, builds the combined game launcher, loader, and DLL, checks binary isolation, and creates `release/Halo3-Freelook-Only-MP-r30-auto-launch.zip` with a file-hash manifest. Earlier release ZIPs are retained. This repository started with the standalone free-look files from local source revision `073ff42`; the current source adds the audit fixes described in `RELEASE-NOTES.md`. Building does not publish a GitHub release or start the game.

## Credits

Uses [MinHook](https://github.com/TsudaKageyu/minhook) and reference information from [HaloCheckpointManager](https://github.com/Burnt-o/HaloCheckpointManager). Their notices are included in `MINHOOK-LICENSE.txt` and `HCM-LICENSE.txt`. No game assets or game data are distributed. This is an unofficial mod and is not affiliated with Project Reclaimer, Microsoft, or Halo's developers.
