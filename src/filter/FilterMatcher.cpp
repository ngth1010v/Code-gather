#include "filter/FilterMatcher.h"

#include <algorithm>

#include "filter/FilterUtils.h"

namespace cgt::filter::detail
{
    namespace
    {
        enum class MatchKind
        {
            Exact,
            EndsWith,
            StartsWith,
            Contains,
        };

        std::wstring ComponentText(std::wstring s)
        {
            s = StripTrailingSlash(std::move(s));
            return s;
        }

        MatchKind GetMatchKind(const Component& c)
        {
            const bool recursive = c.prefixCount >= 2 || c.suffixCount >= 2;

            if (!recursive)
            {
                if (c.prefixCount == 0 && c.suffixCount == 0) return MatchKind::Exact;
                if (c.prefixCount > 0 && c.suffixCount == 0) return MatchKind::EndsWith;
                if (c.prefixCount == 0 && c.suffixCount > 0) return MatchKind::StartsWith;
                return MatchKind::Contains;
            }

            if (c.prefixCount >= 2 && c.suffixCount == 0) return MatchKind::EndsWith;
            if (c.prefixCount == 0 && c.suffixCount >= 2) return MatchKind::StartsWith;
            return MatchKind::Contains;
        }

        bool MatchOneComponent(const std::wstring& src, const Component& rule)
        {
            const auto srcText = ComponentText(src);
            const auto ruleText = ComponentText(rule.name);

            switch (GetMatchKind(rule))
            {
            case MatchKind::Exact:
                return srcText == ruleText;
            case MatchKind::EndsWith:
                return EndWith(srcText, ruleText);
            case MatchKind::StartsWith:
                return StartWith(srcText, ruleText);
            case MatchKind::Contains:
                return Contains(srcText, ruleText);
            }

            return false;
        }

        bool IsRecursive(const Component& c)
        {
            return c.prefixCount >= 2 || c.suffixCount >= 2;
        }

        bool MatchSequence(const std::vector<std::wstring>& src,
                           size_t si,
                           const Condition& condition,
                           size_t ci,
                           bool isDir)
        {

            if (ci >= condition.components.size())
            {
                return true;
            }

            if (si >= src.size())
            {
                return isDir;
            }

            const auto& comp = condition.components[ci];

            if (IsRecursive(comp))
            {
                for (size_t i = si; i < src.size(); ++i)
                {
                    // if (!MatchOneComponent(src[i], comp))
                    if (MatchOneComponent(src[i], comp) == comp.negation)
                    {
                        continue;
                    }

                    if (MatchSequence(src, i + 1, condition, ci + 1, isDir))
                    {
                        return true;
                    }
                }

                return isDir;
            }

            // if (!MatchOneComponent(src[si], comp))
            if (MatchOneComponent(src[si], comp) == comp.negation)
            {
                return false;
            }

            return MatchSequence(src, si + 1, condition, ci + 1, isDir);
        }

        bool MatchCondition(const std::vector<std::wstring>& srcComponents,
                            const Condition& condition)
        {
            if (condition.components.empty())
            {
                return false;
            }

            const bool isDir = !srcComponents.empty() && !srcComponents.back().empty() && srcComponents.back().back() == L'/';
            return MatchSequence(srcComponents, 0, condition, 0, isDir);
        }
    }

    bool MatchRule(const std::vector<std::wstring>& srcComponents,
                   const RuleEntry& rule)
    {
        if (rule.clauses.empty())
        {
            return false;
        }

        for (const auto& andGroup : rule.clauses)
        {
            bool clausePass = true;
            for (const auto& condition : andGroup)
            {
                const bool matched = MatchCondition(srcComponents, condition);
                if (!matched)
                {
                    clausePass = false;
                    break;
                }
            }

            if (clausePass)
            {
                return true;
            }
        }

        return false;
    }
}
