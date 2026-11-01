#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cwchar>
#include "module_path.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, wchar_t*, int) {
    wchar_t script[MAX_PATH], folder[MAX_PATH], system[MAX_PATH];
    if (!freelook::siblingModulePath(nullptr, L"Manage-Freelook.ps1", script) ||
        !freelook::siblingModulePath(nullptr, L"", folder)) return 1;
    UINT length = GetSystemDirectoryW(system, MAX_PATH);
    if (!length || length >= MAX_PATH) return 1;
    wchar_t powershell[MAX_PATH + 80], command[1024];
    swprintf(powershell, MAX_PATH + 80, L"%ls\\WindowsPowerShell\\v1.0\\powershell.exe", system);
#ifdef FREELOOK_UNINSTALL
    const wchar_t* action = L"Uninstall";
#else
    const wchar_t* action = L"Install";
#endif
    swprintf(command, 1024, L"\"%ls\" -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"%ls\" -Action %ls",
             powershell, script, action);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    if (GetFileAttributesW(script) == INVALID_FILE_ATTRIBUTES ||
        !CreateProcessW(powershell, command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                        nullptr, folder, &startup, &process)) {
        MessageBoxW(nullptr, L"Could not open setup/uninstall. Extract the entire mod ZIP together and try again.",
                    L"Reclaimer Freelook", MB_OK | MB_ICONERROR);
        return 1;
    }
    // Exit before the uninstaller removes its own EXE. PowerShell owns the UI.
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return 0;
}
