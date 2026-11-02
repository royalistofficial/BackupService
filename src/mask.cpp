#include "mask.h"
#include "log.h"
#include <cwctype>

bool MatchMask(const wchar_t* name, const wchar_t* mask) {
    DBG(L"name=\"%ls\", mask=\"%ls\"", name, mask);

    if (*mask == L'\0')
        return *name == L'\0';

    if (*mask == L'*')
        return MatchMask(name, mask + 1) ||
               (*name != L'\0' && MatchMask(name + 1, mask));

    if (*name == L'\0')
        return false;

    if (*mask == L'?' || towlower(*mask) == towlower(*name))
        return MatchMask(name + 1, mask + 1);

    return false;
}

bool MatchAnyMask(const std::wstring& name, const std::vector<std::wstring>& masks) {
    if (IsDebug()) {
        std::wstring all;
        for (size_t i = 0; i < masks.size(); i++) { if (i) all += L"; "; all += masks[i]; }
        DBG(L"name=\"%ls\", masks=[%ls]", name.c_str(), all.c_str());
    }
    for (const std::wstring& m : masks)
        if (MatchMask(name.c_str(), m.c_str()))
            return true;
    return false;
}
