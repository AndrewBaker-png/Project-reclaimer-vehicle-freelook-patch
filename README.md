# Vehicle Freelook 30.7.1 for Project Reclaimer 0.10.8

Download the [Windows test build](release/Halo3-Freelook-30.7.1-Reclaimer-0.10.8-TEST.zip?raw=true) and extract it to a regular local
folder. Open `Play-Reclaimer-with-Freelook.exe` and select the Windows 0.10.8
client. Hold F8 in a vehicle to look around and release to recenter.

The 0.10.8 path verifies the client checksum, input instructions, render
camera entry and static TLS lookup. It diverts local steering while freelook
is held and applies the orbit to the primary player's render camera.
The automated fixture exercises hold and release, projection preservation,
menu and focus changes, and vehicle exit. Interactive and multiplayer
validation are pending.

Build with `Build-Freelook-Only.ps1 -ZigPath <path-to-Zig-0.14.1>`.
Run `analysis/Smoke-0108.ps1 -Client <client-exe> -MccPath <MCC-install-directory>` for the timed local Hugegrass load/stop test.
The ZIP SHA-256 is

    dc8585f8180d4db9228881c5e6eafbeca85ad6e7d8e84d2d94396c1f7d30bbc7

A prior 0.10.2 multiplayer session was kicked. The cause remains unresolved. Multiplayer validation for 0.10.8 is pending.

Rebinding: open Configure-Freelook.exe, choose Change binding, press one key
or middle/side mouse button, then Save. The running game picks up the new binding within half a second. This
only edits HoldVirtualKey; sensitivity and other options remain unchanged.
