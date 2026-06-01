#pragma once

#include <string>
#include <vector>
#include "filter/FilterMatcher.h"

namespace cgt::filter::detail
{
    std::vector<std::wstring> ComponentSpliter(std::wstring path);
    bool EndWith(const std::wstring& src, const std::wstring& sub);
    bool StartWith(const std::wstring& src, const std::wstring& sub);
    bool Contains(const std::wstring& src, const std::wstring& sub);

    std::wstring Trim(const std::wstring& s);
    std::wstring ToLower(std::wstring s);
    std::wstring NormalizeLine(std::wstring line);

    std::wstring NormalizeRule(std::wstring rule);
    std::wstring NormalizePath(std::wstring path);

    std::wstring StripQuotes(std::wstring s);
    std::vector<std::wstring> SplitOutsideQuotes(const std::wstring& text,
                                                 wchar_t delimiter);
    std::wstring StripTrailingSlash(std::wstring s);

    bool PrintRuleEntry(RuleEntry& rules);
}
