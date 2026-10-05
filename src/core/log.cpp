#include "log.h"
#include <cstdio>
#include <cstdarg>
#include <mutex>

static std::mutex g_logLock;
static wchar_t g_logPath[MAX_PATH];

void LogInit(HMODULE self)
{
    GetModuleFileNameW(self, g_logPath, MAX_PATH);
    if (wchar_t* dot = wcsrchr(g_logPath, L'.')) wcscpy_s(dot, MAX_PATH - (dot - g_logPath), L".log");
    FILE* f = nullptr;
    if (_wfopen_s(&f, g_logPath, L"w") == 0 && f) fclose(f);
}

void Log(const char* fmt, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    SYSTEMTIME t;
    GetLocalTime(&t);
    std::lock_guard<std::mutex> lock(g_logLock);
    FILE* f = nullptr;
    if (_wfopen_s(&f, g_logPath, L"a") == 0 && f)
    {
        fprintf(f, "[%02d:%02d:%02d.%03d] %s\n", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, buf);
        fclose(f);
    }
}
