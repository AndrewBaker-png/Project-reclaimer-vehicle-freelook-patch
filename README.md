# Project Reclaimer Vehicle Freelook Patch

Hold **F8** in a Halo 3 vehicle to look around independently of the native camera. Release F8 to recenter. Build **Freelook 30.4.6** from `main` for **Project Reclaimer v0.10.0** on Windows x64. The project maintainer confirmed that Freelook works in-game on this Reclaimer version. The legacy **0.9.23** path is also supported.

The project maintainer reports freelook working in both **ground and aerial vehicles**. It is intended for vehicle freelook generally, but every ground/aerial vehicle and seat has not been tested; universal compatibility remains unverified.

The project maintainer also reported successful live **Huge Grass** multiplayer tests on Reclaimer 0.9.23. Broader map and seat coverage remains unverified.

## Download and install

For Reclaimer v0.10.0, use the 30.4.6 ZIP produced by the [build from source](#build-from-source) steps below. The published 30.4.1 prerelease is for Reclaimer 0.9.23.

1. Open the [r30 Installer prerelease](https://github.com/AndrewBaker-png/Project-reclaimer-vehicle-freelook-patch/releases/tag/r30-installer-30.4.1) and download **`Halo3-Freelook-Only-MP-r30-installer.zip`** under **Assets**. The source-code archives do not contain built binaries. A SHA-256 checksum is also attached.
2. Fully close Reclaimer. Extract the ZIP to a **separate folder**, such as `C:\Mods\Freelook-Setup`. Do not extract updates over an installed copy.
3. Double-click **`Setup-Freelook.exe`**. Accept the detected installation to update, select an older portable mod folder manually, or choose a new writable destination. Review the installed/incoming versions and replacement count, then confirm.
4. Double-click **`Play-Reclaimer-with-Freelook.exe` in the installed destination**. On first use, choose your Project Reclaimer client EXE. The launcher remembers it and starts the loader followed by the game on later launches. No commands need to be typed.
5. Enter a vehicle, hold **F8**, and move the mouse. Release **F8** to recenter.

Setup adds the mod to **Windows Settings > Apps > Installed apps**, creates no desktop shortcut, and needs no administrator access. See [install, update, and uninstall](#install-update-and-uninstall) below for backups, repair, and removal.

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

The r30 Installer prerelease includes setup, update/repair, uninstall, the combined game launcher, and the camera, cancellation, and path-safety fixes. Stop waits for a pending load to finish and for the runtime to acknowledge that its hooks are disabled. If confirmation fails, the helper returns an error and asks you to close the client. Older r30 runtimes cannot acknowledge shutdown; fully exit the old client before upgrading. Installation paths that would exceed the launcher's path buffer are refused with an instruction to use a shorter folder.

The loader is session-only. It does not install a service or startup entry; use `Play-Reclaimer-with-Freelook.exe` for each new client session. Opening the original Reclaimer EXE directly still requires the separate start helper.

## Install, update, and uninstall

The [r30 Installer prerelease](https://github.com/AndrewBaker-png/Project-reclaimer-vehicle-freelook-patch/releases/tag/r30-installer-30.4.1) is package version **30.4.1**. Its ZIP includes setup, update/repair, launch, and uninstall helpers.

1. Fully close Reclaimer. Extract the new ZIP to a **separate folder**, not over your installed copy.
2. Double-click **`Setup-Freelook.exe`** in the newly extracted folder. It detects registered installations. For an older portable copy, choose its existing mod folder manually. For a new installation, accept the default folder or select another writable destination.
3. Review the installed and incoming versions, destination, and number of existing mod files to replace. Setup offers **Update** for a newer version and **Repair** for the same version. Downgrades, unrecognized file collisions, and damaged incoming packages are refused.
4. Confirm. Setup stops a waiting loader, checks for running Reclaimer/MCC processes and locked files, backs up the installed files, and copies the new package. It preserves `freelook.ini`, `reclaimer-launcher.ini`, logs, and unrelated files. If replacement or registration fails, it attempts to restore the previous files and Windows Settings entry automatically and reports the result.
5. Launch **`Play-Reclaimer-with-Freelook.exe` from the destination folder** for each session. Setup registers **Project Reclaimer Vehicle Freelook** in **Settings > Apps > Installed apps** for the current account. No administrator access or desktop shortcut is required. Keep the installation folder in place; the separately extracted source folder can be removed after successful setup.

Updates follow these same steps each time. Do not let ZIP extraction overwrite the old installation first: setup cannot recover files that were replaced before it ran. Unregistered older packages must retain their original `manifest.json` so setup can recognize the version. Known r30, Audit Fixes, Auto Launch, and Uninstaller packages are supported.

Backups are kept under **`freelook-backups`** in the destination. Each dated transaction contains the previous files, incoming package, registration snapshot, and a transaction record. Keep these for recovery until satisfied with the update. Rollback handles errors reported during setup; interruption by power loss or forced termination may require recovery from the retained backup. Do not open the game until setup finishes.

To uninstall, fully close Reclaimer, then choose **Uninstall** in Windows Settings or double-click **`Uninstall-Freelook.exe` in the installed folder**. Direct uninstall also works for a portable copy that skipped setup. The uninstaller cancels the loader, confirms shutdown, and refuses locked files or linked folders. It removes known package files, mod settings, logs, and its registration. Reclaimer, maps, unrelated files, modified package documents/binaries, and update backups remain. The folder is removed only if empty; desktop shortcuts are not changed. Delete retained backups manually when no longer needed.

For an older portable package without uninstall support, stop the loader, close Reclaimer, and remove only its mod files. Do not delete a shared game folder.

## Compatibility and limitations

- The loader checks the exact supported Reclaimer executable and live hook sites. Version 0.10.0 hooks native mouse input and the translated camera. The legacy path checks the Halo 3 DLL hash and three hook sites.
- Revision 30 retains the native third-person vehicle view. Cockpit models, visor/HUD overlays, F7/F9 controls, graphics hooks, and custom map assets are outside this build.
- MCC support exists in the underlying launcher but has not been flight-tested for this revision. This package targets Halo 3 vehicles in Reclaimer 0.9.23 and 0.10.0.
- Live multiplayer testing covers Huge Grass on the supported Reclaimer build. This does not establish compatibility with every map, server configuration, vehicle, or seat. Follow the server's mod rules and supported anti-cheat settings.

`loader-state.json` records loader status. `loader.log` records compatibility and load checks; `freelook.log` records DLL activation. If loading is refused, check these files and the supported version before restarting.

## Validation

The project maintainer reports successful use of the latest builds in live Huge Grass multiplayer matches. This is gameplay evidence alongside the automated checks below; a complete host/client, vehicle, and seat test matrix has not been recorded.

The included automated tests cover camera math, native hook preservation, private camera copying, and the actual F8 runtime's hold/release, orbit/FOV isolation, exit, death, focus, menus, vehicle identity, and camera turnover. The binary inspection checks for graphics/network imports and cockpit/HUD file dependencies.

Audit regressions revisit 128 camera buffers, restore every pending pose after release, preserve newer native poses, and exercise retired camera addresses without accessing them outside their callbacks. Separate tests cover path boundaries, shutdown acknowledgment, loader cancellation during injection, pending-load failures, and stopping the actual DLL during startup in a test process without the game engine. Loader tests use a fake executable and isolated event names. The combined launcher test uses a fake client to verify saved locations, loader-before-game ordering, paths with spaces, repeated launches, and the client working directory.

To expand multiplayer coverage, test as host and joining client; observe aiming/steering from another client; check passenger/turret seats, exit/re-entry, death/respawn, map reload/reconnect, menus, focus changes, and stopping the runtime. Include the map, vehicle/seat, host or joining role, and build when reporting results. Report crashes, disconnects, rubber-banding, aim discrepancies, and stuck views with reproduction steps. Review logs for personal information before posting them.

## Build from source

The build includes setup/uninstall GUI helpers and regression tests using synthetic packages and temporary registry keys. Tests cover fresh install, update, repair, older portable migration, settings preservation, downgrade/collision refusal, a real fake-client process, file locks, concurrent maintenance, partial-copy rollback, registration rollback, and uninstall boundaries.

Requirements: Windows x64, PowerShell, Node.js, and **Zig 0.14.1**. The MinHook source required by this build is included under `third_party/minhook`.

Download the [official Zig 0.14.1 Windows x64 archive](https://ziglang.org/download/0.14.1/zig-x86_64-windows-0.14.1.zip). Verify its SHA-256 against `build-manifest.json`, then extract it under `tools/zig-x86_64-windows-0.14.1/`, or pass its executable path:

```powershell
.\Build-Freelook-Only.ps1
# Or use an existing installation:
.\Build-Freelook-Only.ps1 -ZigPath 'C:\path\to\zig.exe'
```

The script compiles and runs six native test programs plus the PowerShell loader scenarios and an actual DLL lifecycle test, builds the setup/uninstall helpers, combined game launcher, loader, and DLL, checks binary isolation, and creates `release/Halo3-Freelook-Only-MP-r30-installer.zip` with a file-hash manifest. Earlier release ZIPs are retained. This repository started with the standalone free-look files from local source revision `073ff42`; the current source adds the audit fixes described in `RELEASE-NOTES.md`. Building does not publish a GitHub release or start the game.

## Credits

Uses [MinHook](https://github.com/TsudaKageyu/minhook) and reference information from [HaloCheckpointManager](https://github.com/Burnt-o/HaloCheckpointManager). Their notices are included in `MINHOOK-LICENSE.txt` and `HCM-LICENSE.txt`. No game assets or game data are distributed. This is an unofficial mod and is not affiliated with Project Reclaimer, Microsoft, or Halo's developers.

## Defender helper (local package 30.4.2)

The package includes `Add-Freelook-Defender-Exclusion.ps1`. After installing to the final folder, open PowerShell as administrator and run the helper from that installed folder. Use `-WhatIf` to preview it. From a source checkout, pass `-LoaderPath` with the full installed `Halo3Freelook.exe` path.

Defender has flagged the loader's DLL injection as `Behavior:Win32/DefenseEvasion.A!ml`. The helper documents why the mod uses those operations and adds only the exact loader file path to Defender exclusions. Run it only for a build you trust. Setup does not run it automatically. It does not restore quarantine, clear detection history, or disable real-time protection. Re-run it for a new installation location if needed. Uninstall does not remove the exclusion; undo it as administrator with `Remove-MpPreference -ExclusionPath 'FULL PATH TO Halo3Freelook.exe'`.
