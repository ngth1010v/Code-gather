#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace cgt::util
{
    std::wstring Trim(std::wstring_view text);
    std::wstring ToLower(std::wstring_view text);
    std::wstring StripQuotes(std::wstring_view text);
    bool StartsWithInsensitive(std::wstring_view text, std::wstring_view prefix);
    bool EndsWithInsensitive(std::wstring_view text, std::wstring_view suffix);
    std::vector<std::wstring> SplitTokens(std::wstring_view text, const std::wstring& separators = L", \t\r\n");
}
