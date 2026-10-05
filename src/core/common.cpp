#include "common.h"
#include <cstdio>
#include <cwctype>

namespace core
{
static wchar_t g_ini[MAX_PATH];

bool CheckBytes(uintptr_t rva, const char* hex)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(g_base + rva);
    size_t i = 0;
    for (const char* s = hex; *s;)
    {
        while (*s == ' ') s++;
        if (!s[0] || !s[1]) break;
        char byteText[3] = {s[0], s[1], 0};
        if (p[i++] != (uint8_t)strtoul(byteText, nullptr, 16)) return false;
        s += 2;
    }
    return true;
}

void SettingsInit(HMODULE self)
{
    GetModuleFileNameW(self, g_ini, MAX_PATH);
    if (wchar_t* dot = wcsrchr(g_ini, L'.')) wcscpy_s(dot, MAX_PATH - (dot - g_ini), L".ini");
}

std::wstring SettingsGet(const wchar_t* section, const wchar_t* key, const wchar_t* fallback)
{
    wchar_t buf[256];
    GetPrivateProfileStringW(section, key, fallback, buf, 256, g_ini);
    return buf;
}

void SettingsSet(const wchar_t* section, const wchar_t* key, const std::wstring& value)
{
    WritePrivateProfileStringW(section, key, value.c_str(), g_ini);
}

int ParseKeyName(const std::wstring& raw, int fallback)
{
    std::wstring n;
    for (wchar_t c : raw)
        if (!iswspace(c)) n += (wchar_t)towupper(c);
    if (n.size() >= 2 && n[0] == L'F')
    {
        int f = _wtoi(n.c_str() + 1);
        if (f >= 1 && f <= 24) return VK_F1 + f - 1;
    }
    if (n == L"INSERT" || n == L"INS") return VK_INSERT;
    if (n == L"HOME") return VK_HOME;
    if (n == L"END") return VK_END;
    if (n == L"DELETE" || n == L"DEL") return VK_DELETE;
    if (n == L"PAGEUP") return VK_PRIOR;
    if (n == L"PAGEDOWN") return VK_NEXT;
    if (n == L"MULTIPLY" || n == L"NUM*") return VK_MULTIPLY;
    if (n == L"DIVIDE" || n == L"NUM/") return VK_DIVIDE;
    if (n == L"SUBTRACT" || n == L"NUM-") return VK_SUBTRACT;
    if (n == L"ADD" || n == L"NUM+") return VK_ADD;
    if (n == L"NONE" || n == L"OFF") return 0;
    if (n.size() == 4 && n.compare(0, 3, L"NUM") == 0 && iswdigit(n[3])) return VK_NUMPAD0 + (n[3] - L'0');
    if (n.size() == 1 && iswalnum(n[0])) return n[0];
    return fallback;
}

std::string KeyName(int vk)
{
    char buf[16];
    if (vk == 0) return "none";
    if (vk >= VK_F1 && vk <= VK_F24) snprintf(buf, sizeof(buf), "F%d", vk - VK_F1 + 1);
    else if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) snprintf(buf, sizeof(buf), "Num %d", vk - VK_NUMPAD0);
    else if (vk == VK_ADD) return "Num +";
    else if (vk == VK_SUBTRACT) return "Num -";
    else if (vk == VK_MULTIPLY) return "Num *";
    else if (vk == VK_DIVIDE) return "Num /";
    else if (vk == VK_INSERT) return "Insert";
    else if (vk == VK_HOME) return "Home";
    else if (vk == VK_END) return "End";
    else if (vk == VK_DELETE) return "Delete";
    else if (vk == VK_PRIOR) return "Page Up";
    else if (vk == VK_NEXT) return "Page Down";
    else if ((vk >= '0' && vk <= '9') || (vk >= 'A' && vk <= 'Z')) snprintf(buf, sizeof(buf), "%c", vk);
    else snprintf(buf, sizeof(buf), "0x%02X", vk);
    return buf;
}

bool GameFocused()
{
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid == GetCurrentProcessId();
}

bool KeyDown(int vk) { return vk > 0 && vk < 256 && (GetAsyncKeyState(vk) & 0x8000) && GameFocused(); }

bool KeyPressed(int vk)
{
    static bool wasDown[256];
    if (vk <= 0 || vk >= 256) return false;
    bool down = KeyDown(vk);
    bool hit = down && !wasDown[vk];
    wasDown[vk] = down;
    return hit;
}
}
