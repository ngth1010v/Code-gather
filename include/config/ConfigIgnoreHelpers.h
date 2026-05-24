#pragma once

#include <string>
#include <vector>

namespace cgt::config::detail
{
    std::vector<std::wstring> ComponentSpliter(std::wstring path);
    bool EndWith(const std::wstring& src, const std::wstring& sub);
    bool IgnoreMatcher(const std::vector<std::wstring>& srcComponents,
                       const std::vector<std::wstring>& ruleComponents);
}