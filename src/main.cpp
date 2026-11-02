#include "scp.h"
#include "service.h"
#include "log.h"
#include <windows.h>
#include <cstdio>
#include <cwchar>
#include <fcntl.h>
#include <io.h>

static const wchar_t* ParseArgs(int argc, wchar_t* argv[]) {
    if (IsDebug()) {
        std::wstring all;
        for (int i = 0; i < argc; i++) { if (i) all += L" "; all += argv[i]; }
        DBG(L"argc=%d, argv=\"%ls\"", argc, all.c_str());
    }
    for (int i = 1; i < argc; i++)
        if (_wcsicmp(argv[i], L"--debug") != 0) return argv[i];
    return L"";
}

int wmain(int argc, wchar_t* argv[]) {
    _setmode(_fileno(stdout), _O_U16TEXT);

    bool debug = false;
    for (int i = 1; i < argc; i++)
        if (_wcsicmp(argv[i], L"--debug") == 0) debug = true;
    SetDebug(debug);
    if (IsDebug()) {
        std::wstring all;
        for (int i = 0; i < argc; i++) { if (i) all += L" "; all += argv[i]; }
        DBG(L"argc=%d, argv=\"%ls\"", argc, all.c_str());
    }

    const wchar_t* cmd = ParseArgs(argc, argv);

    if (cmd[0] == L'\0') {
        RunAsService();
        return 0;
    }

    SetDebugEcho(true);
    int rc = 1;
    if      (_wcsicmp(cmd, L"install") == 0) rc = InstallService(debug);
    else if (_wcsicmp(cmd, L"remove")  == 0) rc = RemoveService(debug);
    else if (_wcsicmp(cmd, L"start")   == 0) rc = StartMyService(debug);
    else if (_wcsicmp(cmd, L"stop")    == 0) rc = StopMyService(debug);
    return rc;
}
