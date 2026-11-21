Freelook 30.7.1 - Reclaimer 0.10.8 TEST BUILD

This local test build adds exact-hash support for Windows Reclaimer 0.10.8.
On 0.10.3, 0.10.4 and 0.10.8, freelook changes only the separate render camera after Reclaimer
builds it. It does not write the orbit into the shared observer camera.
The older 0.9.23 through 0.10.2 paths retain their previous behavior.

PREPARE
Fully close Reclaimer before switching builds. Extract this ZIP to a new,
separate folder. Keep all its files together. To test it portably, open
Play-Reclaimer-with-Freelook.exe and select your Reclaimer 0.10.8 client.
Setup-Freelook.exe is optional; do not overwrite an installed folder by
extracting this ZIP over it. Retain your previous release for rollback.
Use a regular local folder outside OneDrive; setup refuses linked folders.

CONTROLS
Open Configure-Freelook.exe, click Change binding, press the desired key or
middle/side mouse button, then Save. The running game picks up the new binding within half a second. Reset to F8 restores the default. Cancel saves nothing.
Use a single control, not a combination; avoid existing driving/aim binds.
Hold your selected control (default F8) in a vehicle and move the mouse to orbit; release to recenter.
While held, mouse steering/aim stays still. Other vehicle controls remain
native. Defaults: inverted pitch, native sensitivity, 40 ms smoothing,
180 ms recenter. Restart the client after changing sensitivity, inversion or timing.
Ctrl+Shift+F10 stops the active runtime. Stop-Reclaimer-Freelook-Only.cmd
also cancels a waiting loader. Restart Reclaimer before loading again.

VALIDATION LIMITS
The automated suite exercises the actual detour on synthetic engine data,
including unchanged observer/aim fixtures, projection/FOV preservation,
alias refusal, release, menus, focus loss, death, exit and disabled state.
These checks do not establish two-player or public-server compatibility.
The earlier 30.4.8/0.10.2 multiplayer kick remains unexplained. This build
does not bypass server validation or change server settings.

PRIVATE HUGEGRASS TEST
Use the original installed Hugegrass first. The custom Pelican proof map
has a separately repaired audio package; validate it separately from Freelook.
1. Compare unmodified and patched sessions using the same map and vehicle.
2. Check Pelican, ground vehicles, Mega Mantis and Scarab, including seats.
3. A second, unmodified client observes aim/steering during hold and release.
4. Repeat as host and joining client; check exit/re-entry, death, reconnect,
   map reload, chat/menu/scoreboard and focus changes.
5. Record any kick with build, time, vehicle/seat, role and reproduction steps.
   Preserve loader.log, freelook.log and corresponding private server logs.

Only players using freelook need this helper. It is not automatically sent
or executed with a map. No map assets are included. Nothing is published.
