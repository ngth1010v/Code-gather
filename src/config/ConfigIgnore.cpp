#include "config/ConfigState.h"
#include "config/ConfigIgnoreHelpers.h"

#include <algorithm>
#include <system_error>

namespace cgt::config
{
    namespace
    {
        std::wstring NormalizeRelPath(const fs::path& p)
        {
            std::wstring rel = p.generic_wstring();
            std::replace(rel.begin(), rel.end(), L'\\', L'/');
            rel = detail::ToLower(rel);

            while (!rel.empty() && rel.front() == L'/')
            {
                rel.erase(rel.begin());
            }

            return rel;
        }

        bool IsUnderWorkspace(const fs::path& abs, const fs::path& workspace)
        {
            std::error_code ec;
            auto rel = fs::relative(abs, workspace, ec);
            if (ec) return false;

            const auto w = NormalizeRelPath(rel);
            if (w.empty()) return false;

            if (w == L".." || w.rfind(L"../", 0) == 0)
            {
                return false;
            }

            return true;
        }
    }

    bool IsIgnored(fs::path p)
    {
        auto& state = detail::State();
        if (!state.initialized)
        {
            detail::LogWarn(L"IsIgnored() called before Init().");
            return false;
        }

        std::error_code ec;

        fs::path abs = fs::absolute(p, ec);
        if (ec)
        {
            abs = p;
        }

        if (!IsUnderWorkspace(abs, state.workspaceDir))
        {
            return false;
        }

        const bool isDir = fs::is_directory(abs, ec);

        fs::path relPath = fs::relative(abs, state.workspaceDir, ec);
        if (ec)
        {
            return false;
        }

        auto rel = NormalizeRelPath(relPath);
        if (rel.empty())
        {
            return false;
        }

        if (isDir)
        {
            rel.push_back(L'/');
        }

        const auto srcComponents = detail::ComponentSpliter(rel);

        for (const auto& ruleComponents : state.ruleComponentList)
        {
            if (detail::IgnoreMatcher(srcComponents, ruleComponents))
            {
                return true;
            }
        }

        return false;
    }
}