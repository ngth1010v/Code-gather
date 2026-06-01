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
            std::map<std::wstring, detail::RuleEntry> ignores;
            std::map<std::wstring, detail::RuleEntry> filters;
        };

        FilterState& State()
        {
            static FilterState state;
            return state;
        }

        bool AddRule(std::map<std::wstring, detail::RuleEntry>& rules, std::wstring rule)
        {
            detail::RuleEntry entry;
            if (!detail::ParseRule(std::move(rule), entry))
            {
                return false;
            }

            detail::PrintRuleEntry(entry);

            const auto key = detail::SerializeRule(entry);
            entry.original = key;
            rules[key] = std::move(entry);
            return true;
        }

        bool RemoveRule(std::map<std::wstring, detail::RuleEntry>& rules, std::wstring rule)
        {
            detail::RuleEntry entry;
            if (!detail::ParseRule(std::move(rule), entry))
            {
                return false;
            }

            const auto key = detail::SerializeRule(entry);
            auto it = rules.find(key);
            if (it == rules.end())
            {
                return false;
            }

            rules.erase(it);
            return true;
        }

        std::vector<std::wstring> GetRuleKeys(const std::map<std::wstring, detail::RuleEntry>& rules)
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

    bool HasPass(std::wstring path)
    {
        auto& state = State();

        path = detail::NormalizePath(std::move(path));
        if (path.empty())
        {
            return false;
        }

        const auto srcComponents = detail::ComponentSpliter(path);
        if (srcComponents.empty())
        {
            return false;
        }

        // Ignore first
        for (const auto& [_, rule] : state.ignores)
        {
            if (detail::MatchRule(srcComponents, rule))
            {
                return false;
            }
        }

        // No filter => everything passes (except ignored)
        if (state.filters.empty())
        {
            return true;
        }

        // Must match at least one filter
        for (const auto& [_, rule] : state.filters)
        {
            if (detail::MatchRule(srcComponents, rule))
            {
                return true;
            }
        }

        return false;
    }
}
