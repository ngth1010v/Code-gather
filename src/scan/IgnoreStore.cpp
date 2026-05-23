#include "scan/IgnoreStore.h"

#include "util/EncodingUtil.h"
#include "util/PathUtil.h"
#include "util/StringUtil.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <windows.h>

namespace cgt::scan
{
    using cgt::util::NormalizeForCompare;
    using cgt::util::ResolveFromRoot;
    using cgt::util::StripQuotes;
    using cgt::util::Trim;
    using cgt::util::ToLower;

    IgnoreStore::IgnoreStore(fs::path rootDir)
        : rootDir_(std::move(rootDir)),
          ignoreFilePath_(rootDir_ / L".cgtignore")
    {
    }

    std::wstring IgnoreStore::NormalizeKey(std::wstring key)
    {
        key = Trim(key);
        for (wchar_t& ch : key)
        {
            if (ch == L'\\')
            {
                ch = L'/';
            }
        }
        key = ToLower(key);

        while (key.size() > 1 && key.back() == L'/')
        {
            key.pop_back();
        }

        return key;
    }

    IgnoreEntry IgnoreStore::NormalizeToken(const fs::path& rootDir, const std::wstring& token)
    {
        std::wstring raw = StripQuotes(token);
        bool folder = false;

        if (!raw.empty() && (raw.back() == L'/' || raw.back() == L'\\'))
        {
            folder = true;
        }

        fs::path resolved = ResolveFromRoot(rootDir, fs::path(raw));
        const bool exists = fs::exists(resolved);
        if (exists && fs::is_directory(resolved))
        {
            folder = true;
        }

        std::wstring key;
        if (resolved.is_absolute() && cgt::util::IsPathInsideRoot(rootDir, resolved))
        {
            key = cgt::util::RelativeDisplayPath(rootDir, resolved);
        }
        else
        {
            key = NormalizeForCompare(resolved);
        }

        key = NormalizeKey(std::move(key));
        return { key, folder };
    }

    std::wstring IgnoreStore::EntryToLine(const IgnoreEntry& entry)
    {
        if (entry.folder)
        {
            return entry.key + L"/";
        }
        return entry.key;
    }

    bool IgnoreStore::Load()
    {
        entries_.clear();

        std::error_code ec;
        if (!fs::exists(ignoreFilePath_, ec))
        {
            return true;
        }

        std::ifstream file(ignoreFilePath_, std::ios::binary);
        if (!file)
        {
            return false;
        }

        const std::string bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::wstring content = cgt::util::Utf8ToWide(bytes);
        if (!content.empty() && content.front() == L'\ufeff')
        {
            content.erase(content.begin());
        }
        if (content.empty() && !bytes.empty())
        {
            // Fall back to ANSI if the file was not UTF-8 encoded.
            content = cgt::util::Utf8ToWide(std::string{});
            const int required = MultiByteToWideChar(CP_ACP, 0, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
            if (required > 0)
            {
                content.resize(static_cast<std::size_t>(required));
                MultiByteToWideChar(CP_ACP, 0, bytes.data(), static_cast<int>(bytes.size()), content.data(), required);
            }
        }

        std::wistringstream stream(content);
        std::wstring line;
        while (std::getline(stream, line))
        {
            line = Trim(line);
            if (line.empty() || line.front() == L'#')
            {
                continue;
            }

            bool folder = false;
            if (!line.empty() && (line.back() == L'/' || line.back() == L'\\'))
            {
                folder = true;
            }

            IgnoreEntry entry;
            entry.key = NormalizeKey(line);
            entry.folder = folder;
            entries_.push_back(std::move(entry));
        }

        return true;
    }

    void IgnoreStore::MergeCliEntries(const std::vector<std::wstring>& cliEntries)
    {
        if (cliEntries.empty())
        {
            return;
        }

        std::set<std::wstring> existing;
        for (const auto& entry : entries_)
        {
            existing.insert(EntryToLine(entry));
        }

        bool added = false;
        for (const auto& token : cliEntries)
        {
            IgnoreEntry entry = NormalizeToken(rootDir_, token);
            const std::wstring line = EntryToLine(entry);
            if (existing.insert(line).second)
            {
                entries_.push_back(std::move(entry));
                added = true;
            }
        }

        if (added)
        {
            dirty_ = true;
        }
    }

    bool IgnoreStore::SaveIfNeeded() const
    {
        if (!dirty_)
        {
            return true;
        }

        std::error_code ec;
        if (!ignoreFilePath_.parent_path().empty())
        {
            fs::create_directories(ignoreFilePath_.parent_path(), ec);
        }

        std::ofstream file(ignoreFilePath_, std::ios::binary | std::ios::trunc);
        if (!file)
        {
            return false;
        }

        const std::string header = cgt::util::WideToUtf8(L"# CodeGather ignore list\n");
        file.write(header.data(), static_cast<std::streamsize>(header.size()));

        for (const auto& entry : entries_)
        {
            const std::wstring line = EntryToLine(entry) + L"\n";
            const std::string bytes = cgt::util::WideToUtf8(line);
            file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        return true;
    }

    bool IgnoreStore::StartsWithFolderMatch(const std::wstring& value, const std::wstring& prefix)
    {
        if (value == prefix)
        {
            return true;
        }

        if (value.size() <= prefix.size())
        {
            return false;
        }

        if (value.compare(0, prefix.size(), prefix) != 0)
        {
            return false;
        }

        return value[prefix.size()] == L'/';
    }

    bool IgnoreStore::IsIgnored(const fs::path& absolutePath) const
    {
        std::wstring candidateKey;
        if (absolutePath.is_absolute() && cgt::util::IsPathInsideRoot(rootDir_, absolutePath))
        {
            candidateKey = cgt::util::RelativeDisplayPath(rootDir_, absolutePath);
        }
        else
        {
            candidateKey = NormalizeForCompare(absolutePath);
        }

        candidateKey = NormalizeKey(candidateKey);

        for (const auto& entry : entries_)
        {
            if (entry.folder)
            {
                if (StartsWithFolderMatch(candidateKey, entry.key))
                {
                    return true;
                }
            }
            else if (candidateKey == entry.key)
            {
                return true;
            }
        }

        return false;
    }

    std::vector<IgnoreEntry> IgnoreStore::Entries() const
    {
        return entries_;
    }
}
