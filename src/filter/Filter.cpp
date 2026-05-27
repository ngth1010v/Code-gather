#include "filter/Filter.h"

#include <map>
#include <utility>

#include "filter/FilterMatcher.h"
#include "filter/FilterUtils.h"
#include "log/Logger.h"

namespace cgt::filter
{
    namespace
    {
        struct FilterState
        {
            std::map<std::wstring, std::vector<std::wstring>> filters;
            std::map<std::wstring, std::vector<std::wstring>> ignores;
        };

        FilterState& State()
        {
            static FilterState state;
            return state;
        }

        bool AddRule(std::map<std::wstring, std::vector<std::wstring>>& rules, std::wstring rule)
        {
            rule = detail::NormalizeRule(std::move(rule));
            if (rule.empty())
            {
                return false;
            }

            rules[rule] = detail::ComponentSpliter(rule);

            return true;
        }

        bool RemoveRule(std::map<std::wstring, std::vector<std::wstring>>& rules, std::wstring rule)
        {
            rule = detail::NormalizeRule(std::move(rule));
            if (rule.empty())
            {
                return false;
            }

            auto it = rules.find(rule);
            if (it == rules.end())
            {
                return false;
            }

            rules.erase(it);
            return true;
        }

        std::vector<std::wstring> GetRuleKeys(const std::map<std::wstring, std::vector<std::wstring>>& rules)
        {
            std::vector<std::wstring> out;
            out.reserve(rules.size());

            for (const auto& pair : rules)
            {
                out.push_back(pair.first);
            }

            return out;
        }
    }

    bool AddFilter(std::wstring rule)
    {
        return AddRule(State().filters, std::move(rule));
    }

    bool RemoveFilter(std::wstring rule)
    {
        return RemoveRule(State().filters, std::move(rule));
    }

    std::vector<std::wstring> GetFilters()
    {
        return GetRuleKeys(State().filters);
    }

    bool AddIgnore(std::wstring rule)
    {
        return AddRule(State().ignores, std::move(rule));
    }

    bool RemoveIgnore(std::wstring rule)
    {
        return RemoveRule(State().ignores, std::move(rule));
    }

    std::vector<std::wstring> GetIgnores()
    {
        return GetRuleKeys(State().ignores);
    }

    bool HasPass(std::wstring path)
    {
        auto& state = State();

        path = detail::NormalizePath(std::move(path));
        if (path.empty())
        {
            return false;
        }

        const auto srcComponents = detail::ComponentSpliter(path);

        for (const auto& [_, ruleComponents] : state.ignores)
        {
            if (detail::MatchRule(srcComponents, ruleComponents, true))
            {
                return false;
            }
        }        

        if (!state.filters.empty())
        {
            bool passedAnyFilter = false;
            for (const auto& [_, ruleComponents] : state.filters)
            {
                // Truyền ignore = false
                if (detail::MatchRule(srcComponents, ruleComponents, false))
                {
                    passedAnyFilter = true;
                    break;
                }
            }
            
            // Nếu không match bất kỳ filter nào -> loại
            if (!passedAnyFilter)
            {
                return false;
            }
        }       




        return true;
    }
}
