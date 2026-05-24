#include "scan/DiscoveryScanner.h"

#include "util/PathUtil.h"
#include "util/TextFilePolicy.h"
#include "util/StringUtil.h"
#include "log/Logger.h"
#include "config/Config.h"

#include <algorithm>
#include <system_error>
#include <unordered_set>
#include <string>

namespace cgt::scan
{
    using cgt::util::MatchesFilters;
    using cgt::util::NormalizeForCompare;
    using cgt::util::RelativeDisplayPath;
    using cgt::util::IsPathInsideRoot; // Đảm bảo hàm này có sẵn nếu cần check logic nâng cao

    DiscoveryScanner::DiscoveryScanner(fs::path rootDir, std::vector<std::wstring> extFilters, std::vector<std::wstring> dirFilters)
        : rootDir_(std::move(rootDir)),
          extFilters_(std::move(extFilters)),
          dirFilters_(std::move(dirFilters))
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
                if (cgt::config::IsIgnored(current))
                {
                    it.disable_recursion_pending();
                }
                continue;
            }

            if (!it->is_regular_file(ec) || cgt::config::IsIgnored(current))
            {
                continue;
            }

            // 1. Lọc theo extFilters (nếu có danh sách lọc)
            if (!extFilters_.empty() && !MatchesFilters(current, extFilters_))
            {
                continue;
            }

            // Lấy relative path chuẩn của file để thực hiện kiểm tra dirFilters hoặc đóng gói kết quả
            std::wstring relPathStr = RelativeDisplayPath(rootDir_, current.lexically_normal());

            // 2. Lọc theo dirFilters (Nếu dirFilters.size() > 0 thì chỉ giữ lại file nằm trong các thư mục đó)
            if (!dirFilters_.empty())
            {
                bool insideTargetDir = false;
                std::wstring lowerRelPath = cgt::util::ToLower(relPathStr);

                for (const auto& dirFilter : dirFilters_)
                {
                    std::wstring lowerFilter = cgt::util::ToLower(dirFilter);
                    
                    // Chuẩn hóa ký tự phân tách của bộ lọc thư mục để tránh miss match
                    std::replace(lowerFilter.begin(), lowerFilter.end(), L'\\', L'/');
                    std::wstring normalizedRelPath = lowerRelPath;
                    std::replace(normalizedRelPath.begin(), normalizedRelPath.end(), L'\\', L'/');

                    if (!lowerFilter.empty() && lowerFilter.back() != L'/')
                    {
                        lowerFilter += L'/';
                    }

                    if (normalizedRelPath.rfind(lowerFilter, 0) == 0)
                    {
                        insideTargetDir = true;
                        break;
                    }
                }

                if (!insideTargetDir)
                {
                    continue;
                }
            }

            DiscoveredFile item;
            item.absolutePath = current.lexically_normal();
            item.relativePath = std::move(relPathStr);
            out.push_back(std::move(item));
        }

        auto getPathComponents = [](const fs::path& p) {
            std::vector<std::wstring> comps;
            for (const auto& part : p) {
                if (!part.empty() && part != L"/" && part != L"\\") {
                    comps.push_back(part.wstring());
                }
            }
            return comps;
        };

        struct SortItem {
            DiscoveredFile file;
            std::vector<std::wstring> lowerComps;
        };

        std::vector<SortItem> sortableOut;
        sortableOut.reserve(out.size());
        
        for (auto& item : out) {
            SortItem si;
            si.file = std::move(item);
            auto comps = getPathComponents(si.file.relativePath);
            si.lowerComps.reserve(comps.size());
            for (const auto& c : comps) {
                si.lowerComps.push_back(cgt::util::ToLower(c));
            }
            sortableOut.push_back(std::move(si));
        }

        std::sort(sortableOut.begin(), sortableOut.end(), [](const SortItem& a, const SortItem& b) {
            const auto& compsA = a.lowerComps;
            const auto& compsB = b.lowerComps;
            size_t minSize = std::min(compsA.size(), compsB.size());
            
            for (size_t i = 0; i < minSize; ++i) {
                if (compsA[i] != compsB[i]) {
                    bool isFileA = (i == compsA.size() - 1);
                    bool isFileB = (i == compsB.size() - 1);
                    if (isFileA != isFileB) {
                        return isFileA < isFileB; 
                    }
                    return compsA[i] < compsB[i];
                }
            }
            return compsA.size() < compsB.size();
        });

        out.clear();
        for (auto& si : sortableOut) {
            out.push_back(std::move(si.file));
        }

        out.erase(std::unique(out.begin(), out.end(), [](const DiscoveredFile& a, const DiscoveredFile& b)
        {
            return cgt::util::NormalizeForCompare(a.absolutePath) == cgt::util::NormalizeForCompare(b.absolutePath);
        }), out.end());

        return out;
    }
}