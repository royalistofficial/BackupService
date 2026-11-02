#pragma once
#include <string>
#include <vector>

bool MatchMask(const wchar_t* name, const wchar_t* mask);
bool MatchAnyMask(const std::wstring& name, const std::vector<std::wstring>& masks);
