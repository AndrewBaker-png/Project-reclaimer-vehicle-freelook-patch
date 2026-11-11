Project Reclaimer Vehicle Freelook Patch - Installer package 30.4.8

Adds Windows x64 support for Project Reclaimer 0.10.2, including its changed
camera register layout. Reclaimer 0.10.1, 0.10.0, and legacy 0.9.23 remain supported.
The loader verifies the exact client SHA-256 and live input/camera hook sites.
The project maintainer confirmed Freelook 30.4.8 works in-game on Reclaimer 0.10.2.

The latest builds have been successfully used in live Huge Grass multiplayer
matches on Project Reclaimer 0.9.23, as reported by the project maintainer.
Broader map, vehicle/seat and host/client coverage remains to be documented.
The maintainer reports working freelook in both ground and aerial vehicles.
Every vehicle and seat has not been tested; universal compatibility is unverified.

1. Fully close any old Reclaimer session. A stopped cockpit DLL still stays
   resident until the client exits. Do not launch the revision 29 helper.
   Extract this ZIP to a separate folder, not over an installed copy. Run
   Setup-Freelook.exe. Choose the detected installation to update/repair,
   select an old portable mod folder, or choose a new installation folder.
   Review the versions, destination and replacement count, then confirm.
   Setup registers Windows Installed apps, creates no desktop shortcut, and
   needs no administrator access. Use Play-Reclaimer-with-Freelook.exe in the
   DESTINATION folder to play. First time only, choose your Reclaimer EXE.
   The launcher remembers it and starts the loader before the game.
2. Join your Huge Grass match with the map/mod required by the server.
   Follow the server's mod rules and supported anti-cheat settings.
3. The loader automatically verifies and injects when the Halo 3 engine is
   ready. It can also be started after entering the game. In a source checkout
   the helper is at the repository root; in the ZIP it is beside the launcher.
   Exact client hashes and live hook sites are checked. Legacy engines also
   require the supported Halo 3 DLL hash.
4. Hold F8 in a vehicle and move the mouse to orbit the native camera.
   Release F8 to recenter. Normal third-person vehicle view stays in use.
5. Stop-Reclaimer-Freelook-Only.cmd cancels a pending load and stops an active
   runtime. Ctrl+Shift+F10 stops an active runtime only.
   Restart the client before loading this build again or switching builds.
   The stop helper waits for the loader and runtime shutdown acknowledgment.
   If completion cannot be confirmed, it reports an error: close the client
   before trying again. Old r30 DLLs cannot acknowledge shutdown; restart them.

loader-state.json reports waiting/loading/loaded/refused/cancelled/expired.
It also records loadMayBePending when a load has not completed conclusively.
loader.log records loader checks; freelook.log records actual DLL activation.
The loader does not install a Windows service or permanent startup entry.
Use Play-Reclaimer-with-Freelook.exe for each later client session. The separate
Start-Reclaimer-Freelook-Only.cmd still works if the game is already open.
To choose a different client, delete reclaimer-launcher.ini and reopen the
launcher. Keep all package files together in a writable folder. It never retries a
compatibility refusal or a load failure automatically.

No F7/F9 controls, first-person override, visor, HUD, graphics hooks, custom
mesh loader, map package or cockpit asset dependency is included. Revision 29
and its start/stop helpers remain available separately. All players still
need the same underlying map/mod normally required by the server.
Only players who want freelook need this DLL; other players can use an
unmodified client. Confirmed live multiplayer coverage is Huge Grass on the
supported Reclaimer build; other configurations may behave differently.

Suggested checks to expand multiplayer coverage:
- Observe the vehicle/aim from the second client while the tester holds F8,
  moves the view, releases it, and resumes mouse steering/aiming.
- Repeat as host and joining client. Check passenger/turret behavior too.
- Exit/re-enter, die/respawn, change vehicles and reconnect/reload the map.
- Open chat/menu/scoreboard or switch away while holding F8. Release F8
  before resuming. Check that no stale orbit follows the player on foot.
- Confirm normal controls after stopping the runtime.
Record crashes, disconnects, rubber-banding, aim discrepancies and stuck views.
Include the map, vehicle/seat, host or joining role and build in test reports.

The loader does not change server anti-cheat settings or bypass enforcement.
It refuses other resident freelook revisions.

Configuration: freelook.ini beside Halo3Freelook.exe. F8 is key 119;
InvertVertical=1 retains the requested flipped freelook pitch, sensitivity
1000 retains native sensitivity, smoothing is 40 ms and return is 180 ms.
Restart the client and reload after changing configuration.
If the launcher reports an installation path that is too long, move the entire
extracted mod folder to a shorter path, such as C:\Mods\Freelook.

Uninstall: fully close Reclaimer, then use Settings > Apps > Installed apps >
Project Reclaimer Vehicle Freelook > Uninstall, or double-click the included
Uninstall-Freelook.exe in your INSTALLED folder. The Settings entry requires
running setup. Direct uninstall also works for a portable copy without setup.
Keep the installed folder in place. You may delete the separate source folder
after successful setup. Uninstall before moving the installed copy.
The uninstaller cancels the loader and refuses removal when shutdown cannot
be confirmed or files are still in use. It removes known package files, mod
settings, logs and the Installed apps entry. Game files, maps, unrelated files
and modified package documents/binaries remain. It never recursively deletes
a folder and does not change desktop shortcuts. Linked folders are refused.

Updates: always extract the new ZIP separately and run its Setup-Freelook.exe.
Registered copies are detected; choose an older portable copy manually.
Setup compares package versions, offers Update or same-version Repair, and
refuses downgrades, damaged incoming files, and unrecognized file collisions.
Your freelook.ini and saved client location are kept, along with logs and
unrelated files. Close Reclaimer before continuing and leave it closed until
setup finishes. A waiting loader is cancelled; running games/locked files
block changes. Concurrent setup/uninstall on the same copy is refused.
Previous files and registration are backed up under freelook-backups in the
destination. A failed replacement or registration triggers automatic rollback;
setup reports if recovery could not finish. Backups remain after uninstall,
and can be deleted manually when no longer needed. Power loss/forced shutdown
may require recovery from the backup. Setup cannot recover files you already
overwrote by extracting the ZIP directly into the old installation.

Build: Build-Freelook-Only.ps1. The package manifest lists hashes and checks.
This installer package retains the original r30 runtime filename and
includes the r30 audit fixes plus versioned setup, update/repair, and uninstall.
Source runtime: src/freelook_only.cpp, with the shared verified input/camera
sites, math and MinHook stubs. Supported clients are pinned Reclaimer 0.10.0
and legacy 0.9.23 on Windows x64.
MCC requires its Anti-Cheat Disabled launch option and is not flight-tested.
No game assets or game data are included. See the GitHub repository for releases.

Download and release notes:
https://github.com/AndrewBaker-png/Project-reclaimer-vehicle-freelook-patch/releases/tag/r30-installer-30.4.1

Defender helper (30.4.2): After installing to the final folder, run
Add-Freelook-Defender-Exclusion.ps1 from administrator PowerShell if you trust
this build and Defender blocks the loader. Use -WhatIf to preview. The helper
comments explain the DLL injection detection and exclude only Halo3Freelook.exe
beside the script. Setup never runs it automatically. It does not restore
quarantine, clear history, or disable protection. Uninstall leaves the exclusion;
remove it as administrator with Remove-MpPreference -ExclusionPath followed by
the full quoted loader path. A source checkout requires -LoaderPath explicitly.

Camera fix (30.4.3): Preserve Halo's private camera calling convention,
including R11 and XMM3-5, across Freelook processing. Fixes the identified
F8 crash path in the player-camera loop; live verification is still required.
