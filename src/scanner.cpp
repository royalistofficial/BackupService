#include "scanner.h"
#include "mask.h"
#include "log.h"
#include <windows.h>
#include <algorithm>

static void ScanDir(const std::wstring& dir, const std::wstring& prefix,
                    const std::vector<std::wstring>& masks, const std::wstring& exclude,
                    std::vector<FileInfo>& out) {
    DBG(L"dir=\"%ls\", prefix=\"%ls\", masks=[%d], out=[%d]",
        dir.c_str(), prefix.c_str(), (int)masks.size(), (int)out.size());
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;

    do {
        std::wstring name = fd.cFileName;
        if (name == L"." || name == L"..") continue;

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
            ScanDir(dir + L"\\" + name, prefix + name + L"/", masks, exclude, out);
        } else {
            if (name.rfind(L"~$", 0) == 0) continue;
            if (!MatchAnyMask(name, masks)) continue;
            if (_wcsicmp((dir + L"\\" + name).c_str(), exclude.c_str()) == 0) continue; 
            
            FileInfo fi;
            fi.fullPath  = dir + L"\\" + name;
            fi.nameInZip = prefix + name;
            fi.writeTime = ((unsigned long long)fd.ftLastWriteTime.dwHighDateTime << 32) |
                           fd.ftLastWriteTime.dwLowDateTime;
            fi.size      = ((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
            out.push_back(fi);
        }
    } while (FindNextFileW(h, &fd));

    FindClose(h);
}

static bool CompareByName(const FileInfo& a, const FileInfo& b) {
    DBG(L"a=\"%ls\", b=\"%ls\"", a.nameInZip.c_str(), b.nameInZip.c_str());
    return a.nameInZip < b.nameInZip;
}

std::vector<FileInfo> ScanFolder(const std::wstring& root, const std::vector<std::wstring>& masks,
                                 const std::wstring& exclude) {
    DBG(L"root=\"%ls\", masks=[%d], exclude=\"%ls\"", root.c_str(), (int)masks.size(), exclude.c_str());
    std::vector<FileInfo> files;
    ScanDir(root, L"", masks, exclude, files);
    std::sort(files.begin(), files.end(), CompareByName);
    return files;
}

bool SameFiles(const std::vector<FileInfo>& a, const std::vector<FileInfo>& b) {
    DBG(L"a=[%d], b=[%d]", (int)a.size(), (int)b.size());
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i].nameInZip != b[i].nameInZip) return false;
        if (a[i].writeTime != b[i].writeTime) return false;
        if (a[i].size      != b[i].size)      return false;
    }
    return true;
}
