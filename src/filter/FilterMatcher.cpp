#include "filter/FilterMatcher.h"

#include "filter/FilterUtils.h"

namespace cgt::filter::detail
{
    bool MatchRule(const std::vector<std::wstring>& srcComponents,
                   const std::vector<std::wstring>& ruleComponents)
    {
        if (ruleComponents.empty()) return false;
        if (srcComponents.empty()) return false;

        size_t sci = 0;
        size_t rci = 0;

        while (sci < srcComponents.size() && rci < ruleComponents.size())
        {
            const std::wstring& rule = ruleComponents[rci];

            // **xxx -> match any component that ends with xxx
            if (rule.size() >= 2 && rule[0] == L'*' && rule[1] == L'*')
            {
                std::wstring ruleName = rule.substr(2);

                while (sci < srcComponents.size() && !EndWith(srcComponents[sci], ruleName))
                {
                    ++sci;
                }

                if (sci >= srcComponents.size())
                {
                    return false;
                }

                ++sci;
                ++rci;
                continue;
            }

            // *xxx -> match current component that ends with xxx
            if (!rule.empty() && rule[0] == L'*')
            {
                std::wstring ruleName = rule.substr(1);

                if (!EndWith(srcComponents[sci], ruleName))
                {
                    return false;
                }

                ++sci;
                ++rci;
                continue;
            }

            // exact match
            if (srcComponents[sci] != rule)
            {
                return false;
            }

            ++sci;
            ++rci;
        }

        return rci == ruleComponents.size();
    }
}
