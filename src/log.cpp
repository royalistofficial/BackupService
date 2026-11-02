#include "log.h"
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cwchar>

static bool g_debug = false;
static bool g_debugEcho = false;
static SRWLOCK g_fileLock = SRWLOCK_INIT;
static thread_local bool t_inDebug = false;

std::wstring ExeDir() {
    DBG(L"");
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring s = path;
    size_t slash = s.find_last_of(L"\\/");
    return s.substr(0, slash);
}

std::string ToUtf8(const std::wstring& s) {
    DBG(L"s=\"%ls\"", s.c_str());
    if (s.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0, NULL, NULL);
    std::string out(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], len, NULL, NULL);
    return out;
}

std::wstring FromUtf8(const std::string& s) {
    DBG(L"s=[%d байт]", (int)s.size());
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], len);
    return out;
}

static void WriteLine(const std::wstring& text) {
    DBG(L"text=\"%ls\"", text.c_str());
    std::wstring logPath = ExeDir() + L"\\service.log";

    SYSTEMTIME t;
    GetLocalTime(&t);
    char stamp[64];
    snprintf(stamp, sizeof(stamp), "[%04d-%02d-%02d %02d:%02d:%02d] ",
             t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    std::string line = stamp + ToUtf8(text) + "\r\n";

    AcquireSRWLockExclusive(&g_fileLock);
    FILE* f = _wfopen(logPath.c_str(), L"ab");
    if (f) {
        fwrite(line.data(), 1, line.size(), f);
        fclose(f);
    }
    ReleaseSRWLockExclusive(&g_fileLock);
}

void Log(const std::wstring& text) {
    DBG(L"text=\"%ls\"", text.c_str());
    WriteLine(text);
}

void SetDebug(bool on) {
    g_debug = on;
    DBG(L"on=%ls", on ? L"true" : L"false");
}

bool IsDebug() {
    return g_debug;
}

void SetDebugEcho(bool on) {
    g_debugEcho = on;
    DBG(L"on=%ls", on ? L"true" : L"false");
}

std::wstring Fmt(const wchar_t* fmt, ...) {
    wchar_t buf[2048];
    va_list args;
    va_start(args, fmt);
    vswprintf(buf, sizeof(buf) / sizeof(buf[0]), fmt, args);
    va_end(args);
    return buf;
}

void DebugWrite(const char* file, const char* func, const std::wstring& args) {
    if (t_inDebug) return;
    t_inDebug = true;
    DWORD savedError = GetLastError();

    const char* base = file;
    for (const char* p = file; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;
    std::wstring fileName(base, base + strlen(base));
    std::wstring funcName(func, func + strlen(func));
    std::wstring line = Fmt(L"[DBG t=%lu] %ls %ls(%ls)",
                            (unsigned long)GetCurrentThreadId(),
                            fileName.c_str(), funcName.c_str(), args.c_str());
    WriteLine(line);
    if (g_debugEcho) wprintf(L"%ls\n", line.c_str());

    SetLastError(savedError);
    t_inDebug = false;
}
