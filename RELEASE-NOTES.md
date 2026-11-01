# r30 Auto Launch — Prerelease

Includes `Play-Reclaimer-with-Freelook.exe`. Double-click,
choose the Reclaimer client once, then use this launcher for future sessions.
It remembers the client path, arms the existing loader, and opens the game.
No command entry is required. A packaged-launcher integration test verifies
launch order, repeated launches, paths with spaces, and client working directory.

Download `Halo3-Freelook-Only-MP-r30-auto-launch.zip` from the
[Auto Launch prerelease Assets](https://github.com/AndrewBaker-png/Project-reclaimer-aerial-vehicle-freelook-patch/releases/tag/r30-auto-launch).
Extract all files to a writable folder, fully exit Reclaimer, and double-click
`Play-Reclaimer-with-Freelook.exe`. Choose the Reclaimer client EXE once, then use
this launcher for every session. Create a Windows desktop shortcut to it if desired.
To choose a different client, delete `reclaimer-launcher.ini` and reopen it.
The separate start/stop helpers remain included. This package includes all r30
Audit Fixes below and retains the supported Reclaimer 0.9.23 runtime checks.

# r30 Audit Fixes — Previous Prerelease

The latest builds have been successfully used in live **Huge Grass** multiplayer matches on Project Reclaimer 0.9.23, as reported by the project maintainer. Broader map, vehicle/seat, and host/client coverage remains to be documented.

- Reject oversized or truncated runtime DLL paths before injection.
- Retain every outstanding camera baseline and restore all reused camera buffers after F8 release. Revisited cameras no longer compound their orbit after the original eight-slot cache would have evicted them.
- Wait for an in-progress load and runtime shutdown acknowledgment before reporting cancellation complete. Indeterminate loads remain marked in `loader-state.json` and cannot be reported successfully cancelled while no stop event is available.
- Publish the runtime's stop handles before DLL loading returns, and distinguish confirmed shutdown from a missing or timed-out acknowledgment.
- Add path, camera ownership/lifetime, native shutdown, actual DLL startup cancellation, and PowerShell loader regression tests.

Download `Halo3-Freelook-Only-MP-r30-audit-fixes.zip` from the [prerelease Assets](https://github.com/AndrewBaker-png/Project-reclaimer-aerial-vehicle-freelook-patch/releases/tag/r30-audit-fixes). Extract all files together, fully exit Reclaimer, run `Start-Reclaimer-Freelook-Only.cmd`, and reopen Reclaimer. Older r30 runtimes receive stop requests but cannot acknowledge shutdown; the updated helper reports this and asks for a client restart.

# Original r30 — Initial Prerelease

The notes below describe the original r30 release at publication. Use the r30 Auto Launch prerelease above for the latest package and installation instructions.

Standalone vehicle free-look for Project Reclaimer 0.9.23 on Windows x64.

- Hold F8 in a vehicle to look around; release to recenter.
- Start the session-only loader before opening Reclaimer, or after the Halo 3 engine is ready.
- Compatibility checks pin the client/engine hashes and three hook sites.
- Includes launcher, runtime DLL, start/stop helpers, configuration, notices, and a file-hash manifest.

Download **Halo3-Freelook-Only-MP-r30.zip**, extract all files together, and follow the repository's installation instructions. GitHub's automatically generated source archives do not include built binaries.

At the original r30 release, two-player multiplayer testing had not yet been recorded. Subsequent live Huge Grass testing is documented above. No anti-cheat bypass is included.

This build uses the native third-person view. It does not include the earlier cockpit/visor/HUD experiments. Restart Reclaimer before reloading or switching DLL revisions.
