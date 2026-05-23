#pragma once

#include "scan/DiscoveryScanner.h"

#include <filesystem>
#include <vector>

namespace cgt::log
{
    namespace fs = std::filesystem;

    class PrintTree
    {
    public:
        static void PrintFoundFilesTree(
            const std::vector<scan::DiscoveredFile>& files,
            const fs::path& rootDir);
    };
}