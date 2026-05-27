#pragma once

#include <string>
#include <vector>

namespace cgt::filter::detail
{
    std::vector<std::wstring> ComponentSpliter(std::wstring path);
    bool EndWith(const std::wstring& src, const std::wstring& sub);

    std::wstring Trim(const std::wstring& s);
    std::wstring ToLower(std::wstring s);
    std::wstring NormalizeLine(std::wstring line);

    std::wstring NormalizeRule(std::wstring rule);
    std::wstring NormalizePath(std::wstring path);
}
