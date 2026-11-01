#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int WINAPI wWinMain(HINSTANCE, HINSTANCE, wchar_t*, int) {
    HANDLE file = CreateFileW(L"launch-order.txt", FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 1;
    DWORD written;
    const char message[] = "client\r\n";
    BOOL ok = WriteFile(file, message, sizeof(message) - 1, &written, nullptr);
    CloseHandle(file);
    return ok ? 0 : 1;
}
