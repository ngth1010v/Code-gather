#pragma once

#include "app/coreRunner/TreeBuilder.h"
#include "scan/DiscoveryScanner.h"

#include <filesystem>
#include <vector>

namespace cgt::app::coreRunner::printResult
{
    namespace fs = std::filesystem;

    int PrintSuccess(const std::vector<cgt::scan::DiscoveredFile>& discovered,
                     const std::vector<std::size_t>& selectedIds,
                     const fs::path& rootDir,
                     const fs::path& outputPath);

    int PrintNothing();
}