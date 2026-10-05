#pragma once
#include <windows.h>
#include <cstdint>
#include <string>

// Helpers shared by the per-game trainers.
namespace core
{
inline uintptr_t g_base = 0; // game exe base address

template <class T> inline T& At(uintptr_t rva) { return *reinterpret_cast<T*>(g_base + rva); }
template <class T> inline T Fn(uintptr_t rva) { return reinterpret_cast<T>(g_base + rva); }
template <class T> inline T& Field(void* obj, uintptr_t off) { return *reinterpret_cast<T*>((uint8_t*)obj + off); }

// Compares bytes at an RVA against a hex string ("48 83 ec 58 ..."). Used to refuse unknown builds.
bool CheckBytes(uintptr_t rva, const char* hex);

// Simple INI settings stored next to the .asi (e.g. TrilogyTrainer.SA.ini).
void SettingsInit(HMODULE self);
std::wstring SettingsGet(const wchar_t* section, const wchar_t* key, const wchar_t* fallback);
void SettingsSet(const wchar_t* section, const wchar_t* key, const std::wstring& value);
int ParseKeyName(const std::wstring& name, int fallback); // "NONE" gives 0 (disabled)
std::string KeyName(int vk);

// Hotkeys polled on the game thread. Keys only count while the game window is in front.
bool GameFocused();
bool KeyDown(int vk);
bool KeyPressed(int vk); // true once per press
}
