#pragma once

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
        DiscoveryScanner(fs::path rootDir, std::vector<std::wstring> extFilters, std::vector<std::wstring> dirFilters);

        std::vector<DiscoveredFile> Scan() const;

    private:
        fs::path rootDir_;
        std::vector<std::wstring> extFilters_;
        std::vector<std::wstring> dirFilters_;
    };
}