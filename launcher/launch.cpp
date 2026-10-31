// Launch.exe: tiny Win32 launcher. Starts bin\AetherWorkbench.exe (next to this file) with the working
// directory set to the install folder. No console, no admin rights, no network, no installation.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    wchar_t self[MAX_PATH];
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring dir(self);
    dir = dir.substr(0, dir.find_last_of(L"\\/"));
    const std::wstring exe = dir + L"\\bin\\AetherWorkbench.exe";
    if (GetFileAttributesW(exe.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(nullptr, (L"Cannot find:\n" + exe + L"\n\nKeep Launch.exe next to the 'bin' folder (do not move it out of the unzipped package).").c_str(),
                    L"Aether Engineering Workbench", MB_OK | MB_ICONERROR);
        return 1;
    }
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring cmd = L"\"" + exe + L"\"";
    if (!CreateProcessW(exe.c_str(), &cmd[0], nullptr, nullptr, FALSE, 0, nullptr, dir.c_str(), &si, &pi)) {
        MessageBoxW(nullptr, L"Could not start the application (CreateProcess failed).", L"Aether Engineering Workbench", MB_OK | MB_ICONERROR);
        return 2;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}
