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
        explicit DiscoveryScanner(fs::path rootDir);

        std::vector<DiscoveredFile> Scan() const;

    private:
        fs::path rootDir_;
    };
}
