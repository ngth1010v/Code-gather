#include "filter/FilterUtils.h"
#include "log/Logger.h"

#include <algorithm>
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

    bool StartWith(const std::wstring& src, const std::wstring& sub)
    {
        if (sub.empty()) return true;
        if (src.size() < sub.size()) return false;
        return src.compare(0, sub.size(), sub) == 0;
    }

    bool Contains(const std::wstring& src, const std::wstring& sub)
    {
        if (sub.empty()) return true;
        return src.find(sub) != std::wstring::npos;
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

    std::wstring StripQuotes(std::wstring s)
    {
        s = Trim(std::move(s));
        if (s.size() >= 2 && s.front() == L'"' && s.back() == L'"')
        {
            return s.substr(1, s.size() - 2);
        }
        return s;
    }

    std::wstring StripTrailingSlash(std::wstring s)
    {
        while (!s.empty() && s.back() == L'/')
        {
            s.pop_back();
        }
        return s;
    }

    

    std::vector<std::wstring> SplitOutsideQuotes(const std::wstring& text,
                                                 wchar_t delimiter)
    {
        std::vector<std::wstring> parts;
        std::wstring cur;
        bool inQuotes = false;

        for (wchar_t ch : text)
        {
            if (ch == L'"')
            {
                inQuotes = !inQuotes;
                cur.push_back(ch);
                continue;
            }

            if (!inQuotes && ch == delimiter)
            {
                const auto token = Trim(cur);
                if (!token.empty())
                {
                    parts.push_back(token);
                }
                cur.clear();
                continue;
            }

            cur.push_back(ch);
        }

        const auto token = Trim(cur);
        if (!token.empty())
        {
            parts.push_back(token);
        }

        return parts;
    }

    bool PrintRuleEntry(RuleEntry& rule)
    {
            log::Logger::Info(L"Filter", L"===");
            log::Logger::Info(L"Filter", rule.original);
            
            for (const auto& andG : rule.clauses) { // Thêm & để tối ưu hiệu năng
                log::Logger::Info(L"Filter", L" = Start And ===");
                
                for (const auto& con : andG) { // Thêm &
                    log::Logger::Info(L"Filter", L"  = Start Condition ===");
                    
                    for (const auto& com : con.components) { // Thêm &
                        log::Logger::Info(L"Filter", L"  = Start Component ===");
                        log::Logger::Info(L"Filter", L"  name= " + com.name);
                        log::Logger::Info(L"Filter", L"  nega= " + std::wstring(com.negation ? L"yes" : L"no"));
                        log::Logger::Info(L"Filter", L"  isDir= " + std::wstring(com.isDir ? L"yes" : L"no"));
                        log::Logger::Info(L"Filter", L"  prefix= " + std::to_wstring(com.prefixCount));
                        log::Logger::Info(L"Filter", L"  suffix= " + std::to_wstring(com.suffixCount));
                        log::Logger::Info(L"Filter", L"  = End Component ===");
                    }
                    log::Logger::Info(L"Filter", L" = End Condition ===");
                }
                log::Logger::Info(L"Filter", L"= End And ===");
            }
        return true; // 💡 SỬA: Thêm return cho hàm bool
    }
}
