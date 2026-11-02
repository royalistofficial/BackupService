#include "scp.h"
#include "service.h"
#include "log.h"
#include <windows.h>
#include <cstdio>
#include <string>

static void PrintError(const wchar_t* what) {
    DBG(L"what=\"%ls\"", what);
    DWORD code = GetLastError();
    wchar_t* text = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                   FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, code, 0, (LPWSTR)&text, 0, NULL);
    wprintf(L"Ошибка в %ls (код %lu): %ls\n", what, code, text ? text : L"");
    if (text) LocalFree(text);
}

int InstallService(bool debug) {
    DBG(L"debug=%ls", debug ? L"true" : L"false");
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!scm) { PrintError(L"OpenSCManager"); return 1; }

    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(NULL, exe, MAX_PATH);
    std::wstring cmd = L"\"" + std::wstring(exe) + L"\"";
    if (debug) cmd += L" --debug";

    SC_HANDLE svc = CreateServiceW(
        scm,
        SERVICE_NAME,
        SERVICE_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        cmd.c_str(),
        NULL, NULL, NULL, NULL, NULL);
    if (!svc) { PrintError(L"CreateService"); CloseServiceHandle(scm); return 1; }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    wprintf(L"Служба установлена.\n");
    Log(L"Служба установлена");
    return 0;
}

int RemoveService(bool debug) {
    DBG(L"debug=%ls", debug ? L"true" : L"false");
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) { PrintError(L"OpenSCManager"); return 1; }

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!svc) { PrintError(L"OpenService"); CloseServiceHandle(scm); return 1; }

    SERVICE_STATUS status;
    if (QueryServiceStatus(svc, &status) && status.dwCurrentState != SERVICE_STOPPED) {
        ControlService(svc, SERVICE_CONTROL_STOP, &status);
        for (int i = 0; i < 20 && status.dwCurrentState != SERVICE_STOPPED; i++) {
            Sleep(500);
            QueryServiceStatus(svc, &status);
        }
    }

    if (!DeleteService(svc)) {
        PrintError(L"DeleteService");
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return 1;
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    wprintf(L"Служба удалена.\n");
    Log(L"Служба удалена");
    return 0;
}

int StartMyService(bool debug) {
    DBG(L"debug=%ls", debug ? L"true" : L"false");
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) { 
        PrintError(L"OpenSCManager"); 
        return 1; 
    }

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, SERVICE_START);
    if (!svc) { 
        PrintError(L"OpenService"); 
        CloseServiceHandle(scm); 
        return 1; 
    }

    LPCWSTR args[] = { L"--debug" };
    DWORD argCount = debug ? 1 : 0;
    if (!StartServiceW(svc, argCount, debug ? args : NULL)) {
        PrintError(L"StartService");
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return 1;
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    wprintf(L"Служба запущена.\n");
    return 0;
}

int StopMyService(bool debug) {
    DBG(L"debug=%ls", debug ? L"true" : L"false");
    SC_HANDLE scm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (!scm) { PrintError(L"OpenSCManager"); return 1; }

    SC_HANDLE svc = OpenServiceW(scm, SERVICE_NAME, SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!svc) { PrintError(L"OpenService"); CloseServiceHandle(scm); return 1; }

    SERVICE_STATUS status;
    if (!ControlService(svc, SERVICE_CONTROL_STOP, &status)) {
        PrintError(L"ControlService");
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return 1;
    }

    for (int i = 0; i < 20 && status.dwCurrentState != SERVICE_STOPPED; i++) {
        Sleep(500);
        QueryServiceStatus(svc, &status);
    }

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    if (status.dwCurrentState == SERVICE_STOPPED) wprintf(L"Служба остановлена.\n");
    else                                          wprintf(L"Служба ещё останавливается...\n");
    return 0;
}
