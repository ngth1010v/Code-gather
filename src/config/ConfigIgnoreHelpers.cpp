#include "config/ConfigIgnoreHelpers.h"

#include "config/ConfigState.h"

namespace ctg::config::detail
{
    std::vector<std::wstring> SplitSegments(const std::wstring& rel)
    {
        std::vector<std::wstring> out;
        std::wstring cur;
        for (wchar_t ch : rel)
        {
            if (ch == L'/')
            {
                if (!cur.empty()) out.push_back(cur);
                cur.clear();
            }
            else
            {
                cur.push_back(ch);
            }
        }
        if (!cur.empty()) out.push_back(cur);
        return out;
    }

    bool MatchDirRule(const std::vector<std::wstring>& segs, const std::wstring& rule)
    {
        auto r = Trim(ToLower(rule));
        if (r.empty()) return false;

        bool recursive = false;
        bool rootOnly = false;

        if (!r.empty() && r.front() == L'/')
        {
            rootOnly = true;
            r.erase(r.begin());
        }
        if (r.rfind(L"**/", 0) == 0)
        {
            recursive = true;
            r = r.substr(3);
        }
        if (!r.empty() && r.back() == L'/') r.pop_back();
        if (r.empty()) return false;

        if (recursive)
        {
            for (const auto& seg : segs)
            {
                if (seg == r) return true;
            }
            return false;
        }

        return rootOnly ? (!segs.empty() && segs.size() == 1 && segs.front() == r)
                        : (!segs.empty() && segs.size() == 1 && segs.front() == r);
    }

    bool MatchFileRule(const std::vector<std::wstring>& segs, const std::wstring& rule)
    {
        auto r = Trim(ToLower(rule));
        if (r.empty()) return false;

        bool rootOnly = false;
        bool recursive = false;

        if (!r.empty() && r.front() == L'/')
        {
            rootOnly = true;
            r.erase(r.begin());
        }
        if (r.rfind(L"**/", 0) == 0)
        {
            recursive = true;
            r = r.substr(3);
        }

        const auto& name = segs.empty() ? std::wstring{} : segs.back();
        const bool atRoot = segs.size() == 1;
        auto extPos = name.find_last_of(L'.');
        auto ext = extPos == std::wstring::npos ? std::wstring{} : name.substr(extPos + 1);

        if (r.size() >= 2 && r[0] == L'*' && r[1] == L'.')
        {
            auto wantExt = r.substr(2);
            return (recursive || atRoot || !rootOnly) && ext == wantExt && !name.empty();
        }

        if (r.rfind(L"**.", 0) == 0)
        {
            auto wantExt = r.substr(3);
            return ext == wantExt && !name.empty();
        }

        if (rootOnly) return atRoot && name == r;
        if (recursive) return name == r;
        return name == r;
    }

    bool MatchRule(const std::wstring& rel, bool isDir, const std::wstring& rule)
    {
        auto segs = SplitSegments(rel);
        if (segs.empty()) return false;

        if (isDir || (!rule.empty() && rule.back() == L'/'))
        {
            return MatchDirRule(segs, rule);
        }

        return MatchFileRule(segs, rule);
    }
}
