#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <cwchar>
#include "module_path.h"

namespace {
bool fileExists(const wchar_t* path) {
    DWORD attrs = GetFileAttributesW(path);
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}
int error(const wchar_t* message) {
    MessageBoxW(nullptr, message, L"Reclaimer + Freelook", MB_OK | MB_ICONERROR);
    return 1;
}
bool start(const wchar_t* exe, const wchar_t* args, const wchar_t* directory,
           bool hidden, bool wait, DWORD& exitCode) {
    wchar_t command[2048];
    if (swprintf(command, 2048, L"\"%ls\" %ls", exe, args) < 0) return false;
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    if (hidden) { startup.dwFlags = STARTF_USESHOWWINDOW; startup.wShowWindow = SW_HIDE; }
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(exe, command, nullptr, nullptr, FALSE,
                        hidden ? CREATE_NO_WINDOW : 0, nullptr, directory, &startup, &process)) return false;
    CloseHandle(process.hThread);
    bool ok = true;
    if (wait) {
        ok = WaitForSingleObject(process.hProcess, INFINITE) == WAIT_OBJECT_0 &&
             GetExitCodeProcess(process.hProcess, &exitCode);
    }
    CloseHandle(process.hProcess);
    return ok;
}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, wchar_t* commandLine, int) {
    wchar_t config[MAX_PATH], helper[MAX_PATH], nativeLoader[MAX_PATH], folder[MAX_PATH];
    if (!freelook::siblingModulePath(nullptr, L"reclaimer-launcher.ini", config) ||
        !freelook::siblingModulePath(nullptr, L"Start-Reclaimer-Freelook-Only.ps1", helper) ||
        !freelook::siblingModulePath(nullptr, L"Halo3Freelook.exe", nativeLoader) ||
        !freelook::siblingModulePath(nullptr, L"", folder))
        return error(L"Move the entire extracted mod folder to a shorter path, then try again.");
    if (!fileExists(helper) || !fileExists(nativeLoader))
        return error(L"Mod files are missing. Extract the entire mod ZIP into one folder before launching.");

    wchar_t client[MAX_PATH]{};
    GetPrivateProfileStringW(L"Reclaimer", L"Client", L"", client, MAX_PATH, config);
    if (!fileExists(client) || wcscmp(commandLine, L"--choose-client") == 0) {
        client[0] = 0;
        OPENFILENAMEW picker{};
        picker.lStructSize = sizeof(picker);
        picker.lpstrFilter = L"Project Reclaimer client (*.exe)\0*.exe\0\0";
        picker.lpstrFile = client;
        picker.nMaxFile = MAX_PATH;
        picker.lpstrTitle = L"Choose your Project Reclaimer client EXE (remembered for future launches)";
        picker.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (!GetOpenFileNameW(&picker)) {
            if (CommDlgExtendedError()) return error(L"Could not open the client picker. Try a shorter installation path.");
            return 0;
        }
        if (!WritePrivateProfileStringW(L"Reclaimer", L"Client", client, config))
            return error(L"Could not save the client location. Extract the mod to a writable folder and try again.");
    }
    wchar_t system[MAX_PATH];
    UINT systemLength = GetSystemDirectoryW(system, MAX_PATH);
    if (!systemLength || systemLength >= MAX_PATH) return error(L"Could not locate Windows PowerShell.");
    wchar_t powershell[MAX_PATH + 80], arguments[MAX_PATH + 128];
    swprintf(powershell, MAX_PATH + 80, L"%ls\\WindowsPowerShell\\v1.0\\powershell.exe", system);
    swprintf(arguments, MAX_PATH + 128, L"-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"%ls\"", helper);
    DWORD result = 0;
    if (!start(powershell, arguments,
               folder, true, true, result) || result != 0)
        return error(L"Freelook loader could not start. Check loader-error.log and loader.log in the mod folder.");
    wchar_t clientFolder[MAX_PATH];
    wcscpy(clientFolder, client);
    wchar_t* slash = wcsrchr(clientFolder, L'\\');
    if (!slash) return error(L"The saved client location is invalid. Delete reclaimer-launcher.ini and try again.");
    slash[1] = 0;
    if (!start(client, L"", clientFolder, false, false, result))
        return error(L"Reclaimer could not open. The loader will wait up to 30 minutes. Try again or use the stop helper to cancel it.");
    return 0;
}

