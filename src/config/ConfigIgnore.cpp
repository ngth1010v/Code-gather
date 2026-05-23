#include "config/ConfigState.h"
#include "config/ConfigIgnoreHelpers.h"

#include <vector>

namespace ctg::config
{
    bool IsIgnored(fs::path p)
    {
        auto& state = detail::State();
        if (!state.initialized)
        {
            detail::LogWarn(L"IsIgnored() called before Init().");
            return false;
        }

        std::error_code ec;
        if (!fs::exists(p, ec))
        {
            return false;
        }

        const bool isDir = fs::is_directory(p, ec);
        fs::path abs = fs::absolute(p, ec);
        if (ec) abs = p;

        fs::path rel = fs::relative(abs, state.workspaceDir, ec);
        if (ec) rel = abs.filename();

        std::vector<std::wstring> segs;
        for (const auto& part : rel)
        {
            auto s = detail::ToLower(part.generic_wstring());
            if (!s.empty()) segs.push_back(s);
        }
        if (segs.empty()) return false;

        for (size_t i = 1; i <= segs.size(); ++i)
        {
            std::wstring cur;
            for (size_t j = 0; j < i; ++j)
            {
                if (j) cur.push_back(L'/');
                cur += segs[j];
            }

            const bool currentIsDir = (i < segs.size()) || isDir;
            for (const auto& rule : state.ignoreRules)
            {
                if (detail::MatchRule(cur, currentIsDir, rule))
                {
                    return true;
                }
            }
        }

        return false;
    }
}
