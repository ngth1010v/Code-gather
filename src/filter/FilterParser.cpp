#include "filter/FilterMatcher.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "filter/FilterUtils.h"

namespace cgt::filter::detail
{
    namespace
    {
        int StarCountPrefix(const std::wstring& s)
        {
            int count = 0;
            while (count < static_cast<int>(s.size()) && s[static_cast<size_t>(count)] == L'*')
            {
                ++count;
            }
            return count;
        }

        int StarCountSuffix(const std::wstring& s)
        {
            int count = 0;
            for (int i = static_cast<int>(s.size()) - 1; i >= 0 && s[static_cast<size_t>(i)] == L'*'; --i)
            {
                ++count;
            }
            return count;
        }

        bool ParseComponentToken(const std::wstring& rawToken, Component& out)
        {
            std::wstring token = Trim(rawToken);
            if (token.empty())
            {
                return false;
            }

            out = Component{};
            out.isDir = !token.empty() && token.back() == L'/';
            token = StripTrailingSlash(std::move(token));
            token = StripQuotes(std::move(token));
            token = Trim(std::move(token));
            if (token.empty())
            {
                return false;
            }
            
            
            int nega = token[0] == L'!' ? 1 : 0;
            int prefix = 0;
            while (prefix + nega < static_cast<int>(token.size()) && token[static_cast<size_t>(prefix + nega)] == L'*')
            {
                ++prefix;
            }

            int suffix = 0;
            for (int i = static_cast<int>(token.size()) - 1; i >= prefix && token[static_cast<size_t>(i)] == L'*'; --i)
            {
                ++suffix;
            }

            if (prefix + suffix > static_cast<int>(token.size()))
            {
                return false;
            }

            out.prefixCount = std::min(prefix, 2);
            out.suffixCount = std::min(suffix, 2);
            out.negation = nega;
            out.name = token.substr(static_cast<size_t>(prefix+nega), token.size() - static_cast<size_t>(prefix + suffix + nega));
            out.name = Trim(std::move(out.name));

            if (out.name.empty())
            {
                return false;
            }

            return true;
        }

        bool ParseCondition(std::wstring token, Condition& out)
        {
            token = Trim(std::move(token));
            if (token.empty())
            {
                return false;
            }

            out = Condition{};

            if (token.empty())
            {
                return false;
            }

            const auto parts = ComponentSpliter(std::move(token));
            if (parts.empty())
            {
                return false;
            }

            out.components.reserve(parts.size());
            for (const auto& part : parts)
            {
                Component component;
                if (!ParseComponentToken(part, component))
                {
                    return false;
                }
                out.components.push_back(std::move(component));
            }

            return !out.components.empty();
        }

        std::wstring QuoteIfNeeded(const std::wstring& s)
        {
            if (s.empty())
            {
                return L"\"\"";
            }

            const bool needsQuotes = s.find_first_of(L" \t\r\n/,\"") != std::wstring::npos;
            if (!needsQuotes)
            {
                return s;
            }

            return L"\"" + s + L"\"";
        }

        std::wstring SerializeComponent(const Component& c)
        {
            std::wstring out;
            out.reserve(c.name.size() + 8);

            if (c.prefixCount >= 2)
            {
                out.append(L"**");
            }
            else if (c.prefixCount == 1)
            {
                out.push_back(L'*');
            }

            out.append(QuoteIfNeeded(c.name));

            if (c.suffixCount >= 2)
            {
                out.append(L"**");
            }
            else if (c.suffixCount == 1)
            {
                out.push_back(L'*');
            }

            if (c.isDir)
            {
                out.push_back(L'/');
            }

            return out;
        }

        std::wstring SerializeCondition(const Condition& condition)
        {
            std::wstring out;
            if (condition.negated)
            {
                out.push_back(L'!');
            }

            for (size_t i = 0; i < condition.components.size(); ++i)
            {
                if (i != 0)
                {
                    out.push_back(L'/');
                }
                out += SerializeComponent(condition.components[i]);
            }

            return out;
        }
    }

    bool ParseRule(std::wstring rule, RuleEntry& out)
    {
        rule = NormalizeRule(std::move(rule));
        if (rule.empty())
        {
            return false;
        }

        const auto clausesText = SplitOutsideQuotes(rule, L' ');
        if (clausesText.empty())
        {
            return false;
        }

        RuleEntry parsed;
        parsed.original = rule;
        parsed.clauses.reserve(clausesText.size());

        for (const auto& clauseText : clausesText)
        {
            const auto termText = SplitOutsideQuotes(clauseText, L',');
            if (termText.empty())
            {
                return false;
            }

            AndGroup andGroup;
            andGroup.reserve(termText.size());

            for (const auto& term : termText)
            {
                Condition condition;
                if (!ParseCondition(term, condition))
                {
                    return false;
                }
                andGroup.push_back(std::move(condition));
            }

            parsed.clauses.push_back(std::move(andGroup));
        }

        out = std::move(parsed);
        return true;
    }

    std::wstring SerializeRule(const RuleEntry& rule)
    {
        std::wstring out;
        for (size_t i = 0; i < rule.clauses.size(); ++i)
        {
            if (i != 0)
            {
                out.push_back(L' ');
            }

            const auto& andGroup = rule.clauses[i];
            for (size_t j = 0; j < andGroup.size(); ++j)
            {
                if (j != 0)
                {
                    out.push_back(L',');
                }
                out += SerializeCondition(andGroup[j]);
            }
        }
        return out;
    }
}
