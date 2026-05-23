#include "util/StringUtil.h"

#include <cwctype>

namespace cgt::util
{
    std::wstring Trim(std::wstring_view text)
    {
        std::size_t begin = 0;
        std::size_t end = text.size();

        while (begin < end && std::iswspace(text[begin]))
        {
            ++begin;
        }

        while (end > begin && std::iswspace(text[end - 1]))
        {
            --end;
        }

        return std::wstring(text.substr(begin, end - begin));
    }

    std::wstring ToLower(std::wstring_view text)
    {
        std::wstring result(text);
        for (wchar_t& ch : result)
        {
            ch = static_cast<wchar_t>(std::towlower(ch));
        }
        return result;
    }

    std::wstring StripQuotes(std::wstring_view text)
    {
        std::wstring trimmed = Trim(text);
        if (trimmed.size() >= 2)
        {
            const wchar_t first = trimmed.front();
            const wchar_t last = trimmed.back();
            if ((first == L'"' && last == L'"') || (first == L'\'' && last == L'\''))
            {
                return trimmed.substr(1, trimmed.size() - 2);
            }
        }
        return trimmed;
    }

    bool StartsWithInsensitive(std::wstring_view text, std::wstring_view prefix)
    {
        if (prefix.size() > text.size())
        {
            return false;
        }

        for (std::size_t i = 0; i < prefix.size(); ++i)
        {
            if (std::towlower(text[i]) != std::towlower(prefix[i]))
            {
                return false;
            }
        }
        return true;
    }

    bool EndsWithInsensitive(std::wstring_view text, std::wstring_view suffix)
    {
        if (suffix.size() > text.size())
        {
            return false;
        }

        const std::size_t offset = text.size() - suffix.size();
        for (std::size_t i = 0; i < suffix.size(); ++i)
        {
            if (std::towlower(text[offset + i]) != std::towlower(suffix[i]))
            {
                return false;
            }
        }
        return true;
    }

    std::vector<std::wstring> SplitTokens(std::wstring_view text, const std::wstring& separators)
    {
        std::vector<std::wstring> out;
        std::wstring current;

        for (wchar_t ch : text)
        {
            if (separators.find(ch) != std::wstring::npos)
            {
                if (!current.empty())
                {
                    out.push_back(current);
                    current.clear();
                }
                continue;
            }

            current.push_back(ch);
        }

        if (!current.empty())
        {
            out.push_back(current);
        }

        return out;
    }
}
