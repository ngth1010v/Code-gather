#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace cgt::scan
{
    namespace fs = std::filesystem;

    struct IgnoreEntry
    {
        std::wstring key;
        bool folder = false;
    };

    class IgnoreStore
    {
    public:
        explicit IgnoreStore(fs::path rootDir);

        bool Load();
        void MergeCliEntries(const std::vector<std::wstring>& cliEntries);
        bool SaveIfNeeded() const;

        bool IsIgnored(const fs::path& absolutePath) const;
        std::vector<IgnoreEntry> Entries() const;

        static IgnoreEntry NormalizeToken(const fs::path& rootDir, const std::wstring& token);

    private:
        fs::path rootDir_;
        fs::path ignoreFilePath_;
        std::vector<IgnoreEntry> entries_;
        bool dirty_ = false;

        static std::wstring EntryToLine(const IgnoreEntry& entry);
        static std::wstring NormalizeKey(std::wstring key);
        static bool StartsWithFolderMatch(const std::wstring& value, const std::wstring& prefix);
    };
}
