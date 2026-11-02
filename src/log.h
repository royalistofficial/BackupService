#pragma once
#include <string>

std::wstring ExeDir();
void Log(const std::wstring& text);
std::string  ToUtf8(const std::wstring& s);
std::wstring FromUtf8(const std::string& s);

void SetDebug(bool on);
bool IsDebug();
void SetDebugEcho(bool on);
void DebugWrite(const char* file, const char* func, const std::wstring& args);
std::wstring Fmt(const wchar_t* fmt, ...);

#define DBG(...) do { if (IsDebug()) DebugWrite(__FILE__, __func__, Fmt(__VA_ARGS__)); } while (0)
