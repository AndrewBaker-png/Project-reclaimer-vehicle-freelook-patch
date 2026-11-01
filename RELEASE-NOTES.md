# r30 Installer — Repository Name Update (package 30.4.1)

Refresh the installer ZIP with the **Project Reclaimer Vehicle Freelook Patch**
name and canonical `Project-reclaimer-vehicle-freelook-patch` repository links,
including the packaged README and Windows Installed apps information link.
The maintainer reports use in ground and aerial vehicles; every vehicle/seat
remains untested. Camera/runtime behavior is unchanged. Version 30.4.1 is offered
as an update to 30.4.0; settings and the saved game location remain preserved.

Download `Halo3-Freelook-Only-MP-r30-installer.zip` from the
[30.4.1 prerelease Assets](https://github.com/AndrewBaker-png/Project-reclaimer-vehicle-freelook-patch/releases/tag/r30-installer-30.4.1).
Extract separately and run `Setup-Freelook.exe`. The new checksum is attached.

# r30 Installer — Previous Prerelease (package 30.4.0)

Repository renamed to **Project-reclaimer-vehicle-freelook-patch** to reflect
reported use in both ground and aerial vehicles. The project maintainer has not
tested every vehicle or seat; compatibility with all vehicles remains unverified.
This documentation update does not change the published package binaries.

- Install from a separately extracted ZIP into a chosen destination; detect
  registered copies and accept existing portable mod folders for updates.
- Compare package versions and offer Update or same-version Repair. Refuse
  downgrades, damaged source packages, in-place extraction, unknown collisions,
  linked destinations, running games, locked files, and concurrent maintenance.
- Preserve freelook settings, the saved Reclaimer path, logs, and unrelated files.
- Back up previous package files and registration before replacement. Stage and
  verify incoming files; automatically restore previous files/registration when
  a replacement or registration operation fails. Keep transaction backups for
  recovery, including after uninstall. Power loss/forced termination may require
  recovery from the retained backup.
- Add regression coverage for fresh install, update, repair, legacy portable
  migration, version/overwrite checks, real running-client detection, refusal
  paths, partial-copy rollback, registration rollback, and concurrent maintenance.

Download `Halo3-Freelook-Only-MP-r30-installer.zip` from the [Installer prerelease Assets](https://github.com/AndrewBaker-png/Project-reclaimer-vehicle-freelook-patch/releases/tag/r30-installer). Extract separately, close
Reclaimer, then run setup and review the destination/version confirmation.
Launch and uninstall from the installed destination. No desktop shortcuts.

# r30 Uninstaller — Previous local package

- Add `Setup-Freelook.exe` to register this extracted copy in Windows Settings
  under Apps > Installed apps for the current user, without administrator access.
- Add `Uninstall-Freelook.exe` for direct removal from the extracted ZIP contents.
  The Settings entry uses the same uninstaller. No desktop shortcuts are created
  or removed.
- Cancel the loader and confirm shutdown before removing known package files,
  mod configuration, logs, and the installed-app entry. Refuse locked files and
  linked folders. Preserve game files, maps, unrelated files, and modified
  package documents/binaries. Remove the package folder only when empty.
- Test registration/repeated setup, separate installed copies, quoted uninstall
  commands, removal boundaries, shutdown failures, locked runtime/refusal/retry,
  linked folders, drive roots, and invalid packages using isolated fixtures.

Package: `Halo3-Freelook-Only-MP-r30-uninstaller.zip`. After extracting into its
permanent folder, run setup once to enable removal through Windows Settings.
Fully close Reclaimer before uninstalling. Direct uninstall works without setup.
Existing portable releases do not have an Installed apps entry.

# r30 Auto Launch — Previous Prerelease

Includes `Play-Reclaimer-with-Freelook.exe`. Double-click,
choose the Reclaimer client once, then use this launcher for future sessions.
It remembers the client path, arms the existing loader, and opens the game.
No command entry is required. A packaged-launcher integration test verifies
launch order, repeated launches, paths with spaces, and client working directory.

Download `Halo3-Freelook-Only-MP-r30-auto-launch.zip` from the
[Auto Launch prerelease Assets](https://github.com/AndrewBaker-png/Project-reclaimer-vehicle-freelook-patch/releases/tag/r30-auto-launch).
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

Download `Halo3-Freelook-Only-MP-r30-audit-fixes.zip` from the [prerelease Assets](https://github.com/AndrewBaker-png/Project-reclaimer-vehicle-freelook-patch/releases/tag/r30-audit-fixes). Extract all files together, fully exit Reclaimer, run `Start-Reclaimer-Freelook-Only.cmd`, and reopen Reclaimer. Older r30 runtimes receive stop requests but cannot acknowledge shutdown; the updated helper reports this and asks for a client restart.

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
