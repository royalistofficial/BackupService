#pragma once
#include <string>
#include <vector>

struct FileInfo {
    std::wstring fullPath;
    std::wstring nameInZip;
    unsigned long long writeTime;
    unsigned long long size;
};

std::vector<FileInfo> ScanFolder(const std::wstring& root, const std::vector<std::wstring>& masks,
                                 const std::wstring& exclude);
bool SameFiles(const std::vector<FileInfo>& a, const std::vector<FileInfo>& b);
