#pragma once

#include "scan/IgnoreStore.h"

#include <filesystem>
#include <string>
#include <vector>

namespace cgt::scan
{
    namespace fs = std::filesystem;

    struct DiscoveredFile
    {
        fs::path absolutePath;
        std::wstring relativePath;
    };

    class DiscoveryScanner
    {
    public:
        DiscoveryScanner(fs::path rootDir, std::vector<std::wstring> filters, std::vector<IgnoreEntry> ignoreEntries);

        std::vector<DiscoveredFile> Scan() const;

    private:
        fs::path rootDir_;
        std::vector<std::wstring> filters_;
        std::vector<IgnoreEntry> ignoreEntries_;

        bool MatchesIgnore(const fs::path& candidate, bool directory) const;
        bool MatchesFolderIgnore(const std::wstring& candidateKey, const IgnoreEntry& entry) const;
    };
}
