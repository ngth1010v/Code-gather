#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace cgt::cli
{
    struct ParsedArgs
    {
        std::vector<std::wstring> sourceArgs;
        std::vector<std::wstring> unknownTokens;
        std::set<std::wstring> flags;
        std::map<std::wstring, std::vector<std::wstring>> configValues;

        bool HasFlag(const std::wstring& flag) const;
        std::wstring GetFirstConfigValue(const std::wstring& key, const std::wstring& defaultValue = L"") const;
        std::vector<std::wstring> GetConfigValues(const std::wstring& key) const;
    };
}
