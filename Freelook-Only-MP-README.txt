Freelook-only multiplayer test - revision 30, audit fixes (unreleased)

1. Fully close any old Reclaimer session. A stopped cockpit DLL still stays
   resident until the client exits. Do not launch the revision 29 helper.
   Run Start-Reclaimer-Freelook-Only.cmd before opening Reclaimer. It arms
   a hidden one-shot loader for up to 30 minutes. Then open Reclaimer normally.
2. Use a private test lobby with anti-cheat disabled through the supported
   host settings. Join with another player for the network test.
3. The loader automatically verifies and injects when the Halo 3 engine is
   ready. It can also be started after entering the game. In a source checkout
   the helper is at the repository root; in the ZIP it is beside the launcher.
   Exact client/game hashes and three hook sites are still checked.
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
Run the start helper again for a later client session. It never retries a
compatibility refusal or a load failure automatically.

No F7/F9 controls, first-person override, visor, HUD, graphics hooks, custom
mesh loader, map package or cockpit asset dependency is included. Revision 29
and its start/stop helpers remain available separately. Both players still
need the same underlying map/mod normally required by the server.
Only the tester needs this DLL; other players can observe with an unmodified
client. This is a test build, not a verified public-server release.

Suggested two-player checks:
- Observe the vehicle/aim from the second client while the tester holds F8,
  moves the view, releases it, and resumes mouse steering/aiming.
- Repeat as host and joining client. Check passenger/turret behavior too.
- Exit/re-enter, die/respawn, change vehicles and reconnect/reload the map.
- Open chat/menu/scoreboard or switch away while holding F8. Release F8
  before resuming. Check that no stale orbit follows the player on foot.
- Confirm normal controls after stopping the runtime.
Record crashes, disconnects, rubber-banding, aim discrepancies and stuck views.

The loader does not change server anti-cheat settings or bypass enforcement.
It refuses other resident freelook revisions. Full MP behavior remains to
be verified in the actual two-player test.

Configuration: freelook.ini beside Halo3Freelook.exe. F8 is key 119;
InvertVertical=1 retains the requested flipped freelook pitch, sensitivity
1000 retains native sensitivity, smoothing is 40 ms and return is 180 ms.
Restart the client and reload after changing configuration.
If the launcher reports an installation path that is too long, move the entire
extracted mod folder to a shorter path, such as C:\Mods\Freelook.

Build: Build-Freelook-Only.ps1. The package manifest lists hashes and checks.
The local audit-fixes package retains the original r30 runtime filename and
adds camera restoration, path-boundary and loader cancellation regressions.
Source runtime: src/freelook_only.cpp, with the shared verified input/camera
sites, math and MinHook stubs. Supported client is pinned Reclaimer 0.9.23.
MCC requires its Anti-Cheat Disabled launch option and is not flight-tested.
No game assets or game data are included. See the GitHub repository for releases.
