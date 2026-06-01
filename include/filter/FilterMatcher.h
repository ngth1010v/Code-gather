#pragma once

#include <string>
#include <vector>

namespace cgt::filter::detail
{
    struct Component
    {
        std::wstring name;
        bool negation = false;
        int prefixCount = 0;
        int suffixCount = 0;
        bool isDir = false;
    };

    struct Condition
    {
        bool negated = false;
        std::vector<Component> components;
    };

    using AndGroup = std::vector<Condition>;
    using OrGroup  = std::vector<AndGroup>;

    struct RuleEntry
    {
        std::wstring original;
        OrGroup clauses;
    };

    bool ParseRule(std::wstring rule, RuleEntry& out);
    std::wstring SerializeRule(const RuleEntry& rule);

    bool MatchRule(const std::vector<std::wstring>& srcComponents,
                   const RuleEntry& rule);
}
