#pragma once
#include <string>
#include <vector>
#include "scanner.h"

struct ZipStats {
    unsigned int added     = 0;
    unsigned int replaced  = 0; 
    unsigned int unchanged = 0; 
    unsigned int total     = 0;
    bool         written   = false;
};

bool UpdateZip(const std::vector<FileInfo>& files, const std::wstring& zipPath, ZipStats& st);
