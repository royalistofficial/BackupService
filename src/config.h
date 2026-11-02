#pragma once
#include <string>
#include <vector>

struct Config {
    std::wstring source;
    std::wstring archive;
    std::vector<std::wstring> masks;
};

bool LoadConfig(const std::wstring& path, Config& cfg);
