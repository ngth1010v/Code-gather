#pragma once

#include "app/coreRunner/CoreBuildTree.h"

#include <filesystem>
#include <vector>

namespace cgt::scan
{
    struct DiscoveredFile;
}

namespace cgt::app::coreRunner
{
    class CoreSelectionUi
    {
    public:
        std::vector<std::size_t> Run(const std::vector<cgt::scan::DiscoveredFile>& files,
                                     const std::filesystem::path& rootDir);
    };
}
