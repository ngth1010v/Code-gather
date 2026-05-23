#include "scan/DiscoveryScanner.h"

#include "util/PathUtil.h"
#include "util/TextFilePolicy.h"
#include "util/StringUtil.h"
#include "log/Logger.h"


#include <algorithm>
#include <system_error>
#include <unordered_set>
#include <string>
#include <iostream>

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
        

        // ĐÃ SỬA: Giữ nguyên tên file nguyên vẹn giúp cấu trúc cây thư mục chuẩn hóa hơn
        auto getPathComponents = [](const fs::path& p) {
            std::vector<std::wstring> comps;
            for (const auto& part : p) {
                if (!part.empty() && part != L"/" && part != L"\\") {
                    comps.push_back(part.wstring());
                }
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

        // 2. Sort dựa trên các component chuẩn
        std::sort(sortableOut.begin(), sortableOut.end(), [](const SortItem& a, const SortItem& b) {
            const auto& compsA = a.lowerComps;
            const auto& compsB = b.lowerComps;

            size_t minSize = std::min(compsA.size(), compsB.size());
            
            for (size_t i = 0; i < minSize; ++i) {
                if (compsA[i] != compsB[i]) {
                    // Xác định phần tử hiện tại có phải là file không (nếu là component cuối cùng của mảng)
                    bool isFileA = (i == compsA.size() - 1);
                    bool isFileB = (i == compsB.size() - 1);

                    // Ưu tiên hiển thị thư mục lên trước file nếu cùng cấp
                    if (isFileA != isFileB) {
                        return isFileA < isFileB; 
                    }

                    return compsA[i] < compsB[i];
                }
            }

            return compsA.size() < compsB.size();
        });

        // 3. Undecorate: Trả dữ liệu về mảng ban đầu
        out.clear();
        for (auto& si : sortableOut) {
            out.push_back(std::move(si.file));
        }

        // 4. Lọc trùng chính xác tuyệt đối nhờ mảng đã được sort chuẩn thứ tự alphabet tự nhiên
        out.erase(std::unique(out.begin(), out.end(), [](const DiscoveredFile& a, const DiscoveredFile& b)
        {
            return cgt::util::NormalizeForCompare(a.absolutePath) == cgt::util::NormalizeForCompare(b.absolutePath);
        }), out.end());

        return out;
    }
}