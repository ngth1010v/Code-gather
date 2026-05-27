#pragma once

#include <string>
#include <vector>

namespace cgt::filter::detail
{
    bool MatchRule(const std::vector<std::wstring>& srcComponents,
                   const std::vector<std::wstring>& ruleComponents);
}
