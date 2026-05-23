#include "scan/DiscoveryScanner.h"

#include "util/PathUtil.h"
#include "util/TextFilePolicy.h"
#include "util/StringUtil.h"
#include "log/Logger.h"


#include <algorithm>
#include <system_error>
#include <unordered_set>
#include <string>

namespace cgt::scan
{
    using cgt::util::ExtensionKey;
    using cgt::util::IsPathInsideRoot;
    using cgt::util::MatchesFilters;
    using cgt::util::NormalizeForCompare;
    using cgt::util::RelativeDisplayPath;

    DiscoveryScanner::DiscoveryScanner(fs::path rootDir, std::vector<std::wstring> filters, std::vector<IgnoreEntry> ignoreEntries)
        : rootDir_(std::move(rootDir)),
          filters_(std::move(filters)),
          ignoreEntries_(std::move(ignoreEntries))
    {
    }

    bool DiscoveryScanner::MatchesFolderIgnore(const std::wstring& candidateKey, const IgnoreEntry& entry) const
    {
        if (candidateKey == entry.key)
        {
            return true;
        }

        if (candidateKey.size() <= entry.key.size())
        {
            return false;
        }

        if (candidateKey.compare(0, entry.key.size(), entry.key) != 0)
        {
            return false;
        }

        return candidateKey[entry.key.size()] == L'/';
    }

    bool DiscoveryScanner::MatchesIgnore(const fs::path& candidate, bool directory) const
    {
        std::wstring candidateKey;
        if (candidate.is_absolute() && IsPathInsideRoot(rootDir_, candidate))
        {
            candidateKey = RelativeDisplayPath(rootDir_, candidate);
        }
        else
        {
            candidateKey = NormalizeForCompare(candidate);
        }

        for (const auto& entry : ignoreEntries_)
        {
            if (entry.folder)
            {
                if (MatchesFolderIgnore(candidateKey, entry))
                {
                    return true;
                }
            }
            else
            {
                const std::wstring fileKey = candidateKey;
                if (fileKey == entry.key)
                {
                    return true;
                }
            }
        }

        return false;
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
                if (MatchesIgnore(current, true))
                {
                    it.disable_recursion_pending();
                    continue;
                }
                continue;
            }

            if (!it->is_regular_file(ec))
            {
                continue;
            }

            if (MatchesIgnore(current, false))
            {
                continue;
            }

            if (!cgt::util::IsTextCandidate(current))
            {
                continue;
            }

            if (!MatchesFilters(current, filters_))
            {
                continue;
            }

            DiscoveredFile item;
            item.absolutePath = current.lexically_normal();
            item.relativePath = RelativeDisplayPath(rootDir_, item.absolutePath);
            out.push_back(std::move(item));
        }

        auto getPathComponents = [](const fs::path& p) {
            std::vector<std::wstring> comps;
            if (p.empty()) return comps;

            auto it = p.begin();
            auto end = p.end();
            if (it != end) {
                auto next_it = std::next(it);
                while (next_it != end) {
                    if (!it->empty() && *it != L"/" && *it != L"\\") {
                        comps.push_back(it->wstring());
                    }
                    it = next_it;
                    next_it = std::next(next_it);
                }
            }

            if (it != end && !it->empty() && *it != L"/" && *it != L"\\") {
                std::wstring stem = it->stem().wstring();
                std::wstring ext = it->extension().wstring();
                
                if (!stem.empty()) comps.push_back(stem);
                if (!ext.empty()) comps.push_back(ext);
            }
            return comps;
        };

        // 1. Decorate: Tạo cấu trúc tạm để Cache kết quả path đã được parse & lower case
        struct SortItem {
            DiscoveredFile file;
            std::vector<std::wstring> lowerComps;
        };

        std::vector<SortItem> sortableOut;
        sortableOut.reserve(out.size());
        
        // Chỉ tốn O(N) lần parse và ToLower thay vì O(N log N)
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

        // 2. Sort trên mảng đã cache (Cực kỳ nhanh vì chỉ so sánh chuỗi đã có sẵn)
        std::sort(sortableOut.begin(), sortableOut.end(), [](const SortItem& a, const SortItem& b) {
            const auto& compsA = a.lowerComps;
            const auto& compsB = b.lowerComps;

            size_t minSize = std::min(compsA.size(), compsB.size());
            
            for (size_t i = 0; i < minSize; ++i) {
                if (compsA[i] != compsB[i]) {
                    bool isFileA = (compsA.size() - i) <= 2;
                    bool isFileB = (compsB.size() - i) <= 2;

                    if (isFileA != isFileB) {
                        return isFileA < isFileB;
                    }

                    return compsA[i] < compsB[i];
                }
            }

            return compsA.size() < compsB.size();
        });

        // 3. Undecorate: Đẩy ngược dữ liệu đã sắp xếp về mảng out ban đầu
        out.clear();
        for (auto& si : sortableOut) {
            out.push_back(std::move(si.file));
        }

        // 4. Lọc unique (Giữ nguyên logic của bạn)
        out.erase(std::unique(out.begin(), out.end(), [](const DiscoveredFile& a, const DiscoveredFile& b)
        {
            return cgt::util::NormalizeForCompare(a.absolutePath) == cgt::util::NormalizeForCompare(b.absolutePath);
        }), out.end());

        return out;
    }
}