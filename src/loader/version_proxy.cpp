// Minimal ASI loader disguised as version.dll.
// The DE executables import VERSION.dll, and Windows looks next to the exe first, so this file gets
// loaded with the game. It forwards the real version.dll API and then loads every *.asi plugin
// found next to the exe (and in a "scripts" subfolder), like GTA IV's ASI loader does.
#include <windows.h>
#include <string>

extern "C" FARPROC g_versionProcs[17] = {};

static const char* const kExports[17] = {
    "GetFileVersionInfoA",      "GetFileVersionInfoByHandle", "GetFileVersionInfoExA",
    "GetFileVersionInfoExW",    "GetFileVersionInfoSizeA",    "GetFileVersionInfoSizeExA",
    "GetFileVersionInfoSizeExW", "GetFileVersionInfoSizeW",   "GetFileVersionInfoW",
    "VerFindFileA",             "VerFindFileW",               "VerInstallFileA",
    "VerInstallFileW",          "VerLanguageNameA",           "VerLanguageNameW",
    "VerQueryValueA",           "VerQueryValueW",
};

static void LoadRealVersionDll()
{
    wchar_t path[MAX_PATH];
    GetSystemDirectoryW(path, MAX_PATH);
    wcscat_s(path, L"\\version.dll");
    HMODULE real = LoadLibraryW(path);
    if (!real) return;
    for (int i = 0; i < 17; i++)
        g_versionProcs[i] = GetProcAddress(real, kExports[i]);
}

static void LoadAsisIn(const std::wstring& dir)
{
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*.asi").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        std::wstring full = dir + L"\\" + fd.cFileName;
        if (!LoadLibraryW(full.c_str()))
        {
            wchar_t msg[600];
            swprintf_s(msg, L"Failed to load %s (error %lu).", fd.cFileName, GetLastError());
            OutputDebugStringW(msg);
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static DWORD WINAPI LoadPlugins(LPVOID)
{
    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    std::wstring dir = exe;
    dir = dir.substr(0, dir.find_last_of(L"\\/"));
    LoadAsisIn(dir);
    LoadAsisIn(dir + L"\\scripts");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        LoadRealVersionDll();
        // Plugins load on their own thread, after the loader lock is released.
        if (HANDLE t = CreateThread(nullptr, 0, LoadPlugins, nullptr, 0, nullptr)) CloseHandle(t);
    }
    return TRUE;
}
