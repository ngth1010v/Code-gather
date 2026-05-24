#include "config/ConfigIgnoreHelpers.h"

#include <algorithm>

namespace cgt::config::detail
{
    std::vector<std::wstring> ComponentSpliter(std::wstring path)
    {
        std::replace(path.begin(), path.end(), L'\\', L'/');

        std::vector<std::wstring> out;
        std::wstring cur;

        for (wchar_t ch : path)
        {
            if (ch == L'/')
            {
                if (!cur.empty())
                {
                    cur.push_back(L'/');
                    out.push_back(std::move(cur));
                    cur.clear();
                }
                continue;
            }

            cur.push_back(ch);
        }

        if (!cur.empty())
        {
            out.push_back(std::move(cur));
        }

        return out;
    }

    bool EndWith(const std::wstring& src, const std::wstring& sub)
    {
        if (sub.empty()) return true;
        if (src.size() < sub.size()) return false;
        return src.compare(src.size() - sub.size(), sub.size(), sub) == 0;
    }

    bool IgnoreMatcher(const std::vector<std::wstring>& srcComponents,
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