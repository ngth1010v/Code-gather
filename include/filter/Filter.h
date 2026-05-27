#pragma once

#include <string>
#include <vector>

namespace cgt::filter
{
    bool AddFilter(std::wstring rule);
    bool RemoveFilter(std::wstring rule);
    std::vector<std::wstring> GetFilters();

    bool AddIgnore(std::wstring rule);
    bool RemoveIgnore(std::wstring rule);
    std::vector<std::wstring> GetIgnores();

    bool HasPass(std::wstring path);
}
