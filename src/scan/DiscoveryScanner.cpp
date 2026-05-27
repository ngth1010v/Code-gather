#include "scan/DiscoveryScanner.h"

#include "config/Config.h"
#include "log/Logger.h"
#include "filter/Filter.h"
#include "util/PathUtil.h"
#include "util/StringUtil.h"
#include "util/TextFilePolicy.h"

#include <algorithm>
#include <string>
#include <system_error>
#include <unordered_set>

namespace cgt::scan
{
    DiscoveryScanner::DiscoveryScanner(fs::path rootDir)
        : rootDir_(std::move(rootDir))
    {
    }

    std::vector<DiscoveredFile> DiscoveryScanner::Scan() const
    {
        std::vector<DiscoveredFile> out;
        std::error_code ec;

        if (!fs::exists(rootDir_, ec) || !fs::is_directory(rootDir_, ec))
        {
            return out;
        }

        fs::recursive_directory_iterator it(
            rootDir_,
            fs::directory_options::skip_permission_denied,
            ec);

        fs::recursive_directory_iterator end;
        for (; it != end; ++it)
        {
            const fs::path current = it->path();

            if (it->is_directory(ec))
            {
                std::wstring relDir = cgt::util::RelativeDisplayPath(rootDir_, current.lexically_normal());
                if (!relDir.empty() && relDir.back() != L'/' && relDir.back() != L'\\')
                {
                    relDir.push_back(L'/');
                }

                if (!cgt::filter::HasPass(relDir))
                {
                    it.disable_recursion_pending();
                }
                continue;
            }

            if (!it->is_regular_file(ec))
            {
                continue;
            }

            const fs::path normalized = current.lexically_normal();
            std::wstring relPathStr = cgt::util::RelativeDisplayPath(rootDir_, normalized);

            if (!cgt::filter::HasPass(relPathStr))
            {
                continue;
            }

            DiscoveredFile item;
            item.absolutePath = normalized;
            item.relativePath = std::move(relPathStr);
            out.push_back(std::move(item));
        }

        auto getPathComponents = [](const fs::path& p)
        {
            std::vector<std::wstring> comps;
            for (const auto& part : p)
            {
                if (!part.empty() && part != L"/" && part != L"\\")
                {
                    comps.push_back(part.wstring());
                }
            }
            return comps;
        };

        struct SortItem
        {
            DiscoveredFile file;
            std::vector<std::wstring> lowerComps;
        };

        std::vector<SortItem> sortableOut;
        sortableOut.reserve(out.size());

        for (auto& item : out)
        {
            SortItem si;
            si.file = std::move(item);
            auto comps = getPathComponents(si.file.relativePath);
            si.lowerComps.reserve(comps.size());
            for (const auto& c : comps)
            {
                si.lowerComps.push_back(cgt::util::ToLower(c));
            }
            sortableOut.push_back(std::move(si));
        }

        std::sort(sortableOut.begin(), sortableOut.end(), [](const SortItem& a, const SortItem& b)
        {
            const auto& compsA = a.lowerComps;
            const auto& compsB = b.lowerComps;
            size_t minSize = std::min(compsA.size(), compsB.size());

            for (size_t i = 0; i < minSize; ++i)
            {
                if (compsA[i] != compsB[i])
                {
                    bool isFileA = (i == compsA.size() - 1);
                    bool isFileB = (i == compsB.size() - 1);
                    if (isFileA != isFileB)
                    {
                        return isFileA < isFileB;
                    }
                    return compsA[i] < compsB[i];
                }
            }
            return compsA.size() < compsB.size();
        });

        out.clear();
        for (auto& si : sortableOut)
        {
            out.push_back(std::move(si.file));
        }

        out.erase(std::unique(out.begin(), out.end(), [](const DiscoveredFile& a, const DiscoveredFile& b)
        {
            return cgt::util::NormalizeForCompare(a.absolutePath) == cgt::util::NormalizeForCompare(b.absolutePath);
        }), out.end());

        return out;
    }
}
