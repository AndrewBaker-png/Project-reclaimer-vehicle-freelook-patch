# Unreleased — r30 audit fixes

- Reject oversized or truncated runtime DLL paths before injection.
- Retain every outstanding camera baseline and restore all reused camera buffers after F8 release. Revisited cameras no longer compound their orbit after the original eight-slot cache would have evicted them.
- Wait for an in-progress load and runtime shutdown acknowledgment before reporting cancellation complete. Indeterminate loads remain marked in `loader-state.json` and cannot be reported successfully cancelled while no stop event is available.
- Publish the runtime's stop handles before DLL loading returns, and distinguish confirmed shutdown from a missing or timed-out acknowledgment.
- Add path, camera ownership/lifetime, native shutdown, actual DLL startup cancellation, and PowerShell loader regression tests.

Local build artifact: `Halo3-Freelook-Only-MP-r30-audit-fixes.zip`. Restart the client before loading it. Older r30 runtimes receive stop requests but cannot acknowledge shutdown; the updated helper reports this and asks for a client restart. No multiplayer certification or new public release is implied.

# r30 — F8 Freelook (Experimental)

Standalone vehicle free-look for Project Reclaimer 0.9.23 on Windows x64.

- Hold F8 in a vehicle to look around; release to recenter.
- Start the session-only loader before opening Reclaimer, or after the Halo 3 engine is ready.
- Compatibility checks pin the client/engine hashes and three hook sites.
- Includes launcher, runtime DLL, start/stop helpers, configuration, notices, and a file-hash manifest.

Download **Halo3-Freelook-Only-MP-r30.zip**, extract all files together, and follow the repository's installation instructions. GitHub's automatically generated source archives do not include built binaries.

**Experimental prerelease:** two-player multiplayer behavior remains unverified. Use a private test lobby with anti-cheat disabled through supported host settings. No anti-cheat bypass is included. Public-server compatibility is unverified.

This build uses the native third-person view. It does not include the earlier cockpit/visor/HUD experiments. Restart Reclaimer before reloading or switching DLL revisions.
