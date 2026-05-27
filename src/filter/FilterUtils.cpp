#include "filter/FilterUtils.h"

#include <algorithm>
#include <cctype>
#include <clocale>
#include <cwctype>

namespace cgt::filter::detail
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

    std::wstring Trim(const std::wstring& s)
    {
        const auto start = s.find_first_not_of(L" \t\r\n");
        if (start == std::wstring::npos) return L"";
        const auto end = s.find_last_not_of(L" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    std::wstring ToLower(std::wstring s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        return s;
    }

    std::wstring NormalizeLine(std::wstring line)
    {
        line = Trim(line);
        while (!line.empty() && (line.back() == L'\r' || line.back() == L'\n'))
        {
            line.pop_back();
        }
        return line;
    }

    std::wstring NormalizeRule(std::wstring rule)
    {
        return ToLower(Trim(std::move(rule)));
    }

    std::wstring NormalizePath(std::wstring path)
    {
        std::replace(path.begin(), path.end(), L'\\', L'/');
        path = ToLower(Trim(std::move(path)));

        while (!path.empty() && path.front() == L'/')
        {
            path.erase(path.begin());
        }

        return path;
    }
}
