#pragma once
#include <windows.h>

// Appends lines to <plugin name>.log next to the .asi.
void LogInit(HMODULE self);
void Log(const char* fmt, ...);
