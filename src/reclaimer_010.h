#pragma once

#include <windows.h>
#include <cstdint>
#include <cstring>

namespace freelook {
inline constexpr char reclaimer010Hash[] =
    "8246e8a4a6e90dcd2305ca402a5de3f6b2ab295fc86e3987ac100d8dcd38312e";

// Windows x64 machine-code cache for the supported Halo 3 build.
inline constexpr uintptr_t liftedInput = 0x5a31a0;
inline constexpr uintptr_t liftedCamera = 0x584d20;
inline constexpr uintptr_t cameraEnd = liftedCamera + 0x1de;
inline constexpr uintptr_t liftedInputX = 0x222a054;
inline constexpr uintptr_t liftedInputY = 0x222a058;
inline constexpr uintptr_t liftedFlightY = 0x222a05c;

// RVAs in the Reclaimer client PE. The native input bridge writes the
// processed mouse axes into the guest image before the translated game reads them.
inline constexpr uintptr_t nativeInputEnd = 0x2f45f05;
inline constexpr uintptr_t nativeContextKey = 0x4936218;
inline constexpr unsigned char nativeInputEndBytes[] = {
    0x0f,0x28,0xb4,0x24,0x90,0x00,0x00,0x00,0x0f,0x28,0xbc,0x24,0xa0,0x00,0x00,0x00
};
inline constexpr unsigned char cameraEndBytes[] = {
    0x48,0x8b,0x5c,0x24,0x20,0x4c,0x8b,0x64,0x24,0x28,0x48,0x83,0xc4,0x30,0x48,0x89
};

struct LiftedEngine {
    uintptr_t code = 0;
    uintptr_t image = 0;
    uintptr_t tlsVector = 0;
    uintptr_t host = 0;
};

template<class T>
inline bool readRemote(HANDLE process, uintptr_t address, T& value) {
    SIZE_T bytes = 0;
    return address && ReadProcessMemory(process, reinterpret_cast<const void*>(address),
                                       &value, sizeof value, &bytes) && bytes == sizeof value;
}

template<size_t N>
inline bool matchRemote(HANDLE process, uintptr_t address, const unsigned char (&expected)[N]) {
    unsigned char bytes[N];
    return readRemote(process, address, bytes) && std::memcmp(bytes, expected, N) == 0;
}

inline LiftedEngine findLiftedEngine(HANDLE process) {
    constexpr unsigned char storeX[] = {0xc4,0xc3,0x79,0x16,0x84,0x24,0x54,0xa0,0x22,0x02,0x00};
    constexpr unsigned char storeY[] = {0xc4,0xc3,0x79,0x16,0x84,0x24,0x58,0xa0,0x22,0x02,0x00};
    constexpr unsigned char imageLoad[] = {0x48,0xb9};
    constexpr unsigned char tlsLoad[] = {0x48,0xb8};
    constexpr unsigned char tlsCall[] = {
        0x4c,0x89,0xe1,0xff,0xd0,0x8b,0xce,0x48,0x8b,0x04,0xc8,0x48,0x8b,0x88,0x78,0x05,0x00,0x00
    };
    constexpr unsigned char tlsEntry[] = {0x56,0x48,0x83,0xec,0x20,0x48,0x8b,0x81,0xd8,0x02,0x00,0x00};
    LiftedEngine found{};
    MEMORY_BASIC_INFORMATION region{};
    for (uintptr_t address = 0; VirtualQueryEx(process, reinterpret_cast<void*>(address), &region, sizeof region);) {
        const auto base = reinterpret_cast<uintptr_t>(region.BaseAddress);
        address = base + region.RegionSize;
        if (address <= base) break;
        if (region.State != MEM_COMMIT || region.Type != MEM_PRIVATE ||
            region.AllocationBase != region.BaseAddress || region.RegionSize < 0x17d0400 ||
            (region.Protect != PAGE_EXECUTE_READWRITE && region.Protect != PAGE_EXECUTE_READ)) continue;
        if (!matchRemote(process, base + liftedInput + 0x12db, storeX) ||
            !matchRemote(process, base + liftedInput + 0x1369, storeY) ||
            !matchRemote(process, base + liftedCamera + 0x176, imageLoad) ||
            !matchRemote(process, base + liftedCamera + 0x197, tlsLoad) ||
            !matchRemote(process, base + liftedCamera + 0x1a1, tlsCall)) continue;
        LiftedEngine candidate{base};
        uintptr_t inputImage = 0;
        uint32_t tlsIndex = 0;
        MEMORY_BASIC_INFORMATION helper{};
        if (!readRemote(process, base + liftedCamera + 0x178, candidate.image) || !candidate.image ||
            !readRemote(process, base + liftedInput + 0x212, inputImage) || inputImage != candidate.image ||
            !readRemote(process, base + liftedCamera + 0x199, candidate.tlsVector) ||
            !VirtualQueryEx(process, reinterpret_cast<void*>(candidate.tlsVector), &helper, sizeof helper) ||
            helper.Type != MEM_IMAGE || helper.State != MEM_COMMIT ||
            !matchRemote(process, candidate.tlsVector, tlsEntry) ||
            !readRemote(process, candidate.image + 0xa39f9c, tlsIndex) || tlsIndex > 1023) continue;
        if (found.code) return {};
        candidate.host = reinterpret_cast<uintptr_t>(helper.AllocationBase);
        found = candidate;
    }
    return found;
}

inline bool liftedHookSitesMatch(HANDLE process, const LiftedEngine& engine) {
    constexpr unsigned char nativeAxisStores[] = {
        0xf3,0x44,0x0f,0x11,0xa4,0x01,0x54,0xa0,0x22,0x02,
        0xf3,0x0f,0x11,0xb4,0x01,0x58,0xa0,0x22,0x02,
        0xf3,0x0f,0x11,0x84,0x01,0x5c,0xa0,0x22,0x02
    };
    constexpr unsigned char contextLookup[] = {
        0x48,0x8b,0x05,0x67,0xbc,0x5a,0x01,0x8b,0x70,0x18
    };
    return matchRemote(process, engine.host + nativeInputEnd - sizeof nativeAxisStores, nativeAxisStores) &&
           matchRemote(process, engine.host + nativeInputEnd, nativeInputEndBytes) &&
           matchRemote(process, engine.host + 0x338a5aa, contextLookup) &&
           matchRemote(process, engine.code + cameraEnd, cameraEndBytes);
}
}
