#include "config.h"
#include "log.h"
#include <windows.h>
#include <cstdio>

static std::wstring Trim(const std::wstring& s) {
    DBG(L"s=\"%ls\"", s.c_str());
    const wchar_t* spaces = L" \t\r\n";
    size_t b = s.find_first_not_of(spaces);
    if (b == std::wstring::npos) return L"";
    size_t e = s.find_last_not_of(spaces);
    return s.substr(b, e - b + 1);
}

static std::vector<std::wstring> SplitMasks(const std::wstring& s) {
    DBG(L"s=\"%ls\"", s.c_str());
    std::vector<std::wstring> result;
    size_t start = 0;
    while (start <= s.size()) {
        size_t sep = s.find(L';', start);
        if (sep == std::wstring::npos) sep = s.size();
        std::wstring m = Trim(s.substr(start, sep - start));
        if (!m.empty()) result.push_back(m);
        start = sep + 1;
    }
    return result;
}

bool LoadConfig(const std::wstring& path, Config& cfg) {
    DBG(L"path=\"%ls\"", path.c_str());
    FILE* f = _wfopen(path.c_str(), L"rb");
    if (!f) {
        Log(L"Ошибка: не найден файл настроек " + path);
        return false;
    }
    std::string bytes;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) bytes.append(buf, n);
    fclose(f);

    if (bytes.size() >= 3 && (unsigned char)bytes[0] == 0xEF &&
        (unsigned char)bytes[1] == 0xBB && (unsigned char)bytes[2] == 0xBF)
        bytes.erase(0, 3);

    std::wstring text = FromUtf8(bytes);

    size_t pos = 0;
    while (pos < text.size()) {
        size_t eol = text.find(L'\n', pos);
        if (eol == std::wstring::npos) eol = text.size();
        std::wstring line = Trim(text.substr(pos, eol - pos));
        pos = eol + 1;

        if (line.empty() || line[0] == L'#') continue;
        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;

        std::wstring key   = Trim(line.substr(0, eq));
        std::wstring value = Trim(line.substr(eq + 1));

        if      (key == L"source")  cfg.source  = value;
        else if (key == L"archive") cfg.archive = value;
        else if (key == L"masks")   cfg.masks   = SplitMasks(value);
    }

    while (!cfg.source.empty() && cfg.source.back() == L'\\') cfg.source.pop_back();

    if (cfg.source.empty())  { Log(L"Ошибка: в настройках нет source");  return false; }
    if (cfg.archive.empty()) { Log(L"Ошибка: в настройках нет archive"); return false; }
    if (cfg.masks.empty())   { Log(L"Ошибка: в настройках нет masks");   return false; }

    Log(L"Настройки прочитаны. Источник: " + cfg.source + L", архив: " + cfg.archive);
    return true;
}
