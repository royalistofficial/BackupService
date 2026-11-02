#include "service.h"
#include "config.h"
#include "scanner.h"
#include "zip.h"
#include "log.h"
#include <windows.h>

static SERVICE_STATUS        g_status;
static SERVICE_STATUS_HANDLE g_statusHandle;
static HANDLE                g_stopEvent;

static const DWORD CHECK_EVERY_MS = 3000;

static const wchar_t* StateName(DWORD state) {
    DBG(L"state=%lu", (unsigned long)state);
    switch (state) {
    case SERVICE_START_PENDING: return L"SERVICE_START_PENDING";
    case SERVICE_RUNNING:       return L"SERVICE_RUNNING";
    case SERVICE_STOP_PENDING:  return L"SERVICE_STOP_PENDING";
    case SERVICE_STOPPED:       return L"SERVICE_STOPPED";
    default:                    return L"?";
    }
}

static void SetState(DWORD state, DWORD exitCode = NO_ERROR) {
    DBG(L"state=%ls, exitCode=%lu", StateName(state), (unsigned long)exitCode);
    g_status.dwCurrentState = state;
    g_status.dwWin32ExitCode = exitCode;
    g_status.dwControlsAccepted = (state == SERVICE_RUNNING)
                                  ? (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN) : 0;
    SetServiceStatus(g_statusHandle, &g_status);
}

static void WINAPI ControlHandler(DWORD control) {
    DBG(L"control=%lu", (unsigned long)control);
    switch (control) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        Log(control == SERVICE_CONTROL_STOP ? L"Получена команда: остановка"
                                            : L"Получена команда: выключение компьютера");
        SetState(SERVICE_STOP_PENDING);
        SetEvent(g_stopEvent);
        break;
    default:
        SetServiceStatus(g_statusHandle, &g_status);
        break;
    }
}

static void WINAPI ServiceMain(DWORD argc, LPWSTR* argv) {
    for (DWORD i = 0; i < argc; i++)
        if (_wcsicmp(argv[i], L"--debug") == 0) SetDebug(true);
    if (IsDebug()) {
        std::wstring all;
        for (DWORD i = 0; i < argc; i++) { if (i) all += L" "; all += argv[i]; }
        DBG(L"argc=%lu, argv=\"%ls\"", (unsigned long)argc, all.c_str());
    }

    g_statusHandle = RegisterServiceCtrlHandlerW(SERVICE_NAME, ControlHandler);
    if (!g_statusHandle) return;

    g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    SetState(SERVICE_START_PENDING);

    g_stopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!g_stopEvent) {
        Log(L"Служба не запущена: CreateEvent");
        SetState(SERVICE_STOPPED, GetLastError());
        return;
    }

    Config cfg;
    if (!LoadConfig(ExeDir() + L"\\config.ini", cfg)) {
        Log(L"Служба не запущена: ошибка в настройках");
        CloseHandle(g_stopEvent);
        SetState(SERVICE_STOPPED, ERROR_BAD_CONFIGURATION);
        return;
    }

    SetState(SERVICE_RUNNING);
    Log(L"Служба запущена");

    std::vector<FileInfo> lastFiles;
    bool archiveDone = false;

    do {
        std::vector<FileInfo> files = ScanFolder(cfg.source, cfg.masks, cfg.archive);
        bool archiveMissing = GetFileAttributesW(cfg.archive.c_str()) == INVALID_FILE_ATTRIBUTES;

        if (!archiveDone || archiveMissing || !SameFiles(files, lastFiles)) {
            ZipStats st;
            if (UpdateZip(files, cfg.archive, st)) {
                if (st.written)
                    Log(L"Архив обновлён: добавлено " + std::to_wstring(st.added) +
                        L", заменено " + std::to_wstring(st.replaced) +
                        L", без изменений " + std::to_wstring(st.unchanged) +
                        L", всего в архиве " + std::to_wstring(st.total));
                else
                    Log(L"Архив актуален, записей: " + std::to_wstring(st.total));
                lastFiles = files;
                archiveDone = true;
            }
        }
    } while (WaitForSingleObject(g_stopEvent, CHECK_EVERY_MS) == WAIT_TIMEOUT);

    CloseHandle(g_stopEvent);
    Log(L"Служба остановлена");
    SetState(SERVICE_STOPPED);
}

void RunAsService() {
    DBG(L"RunAsService");
    SERVICE_TABLE_ENTRYW table[] = {
        { (LPWSTR)SERVICE_NAME, ServiceMain },
        { NULL, NULL }
    };
    StartServiceCtrlDispatcherW(table);
}
