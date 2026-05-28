#pragma once

#include "app/coreRunner/CoreBuildTree.h"

#include <filesystem>
#include <string>
#include <vector>

namespace cgt::scan
{
    struct DiscoveredFile;
}

namespace cgt::app::coreRunner
{
    struct CoreConfirmResult
    {
        std::wstring outputToken;
        bool replace = false;
        bool wrapped = false;
        bool cancelled = false;
    };

    class CoreConfirmer
    {
    public:
        CoreConfirmResult Run(const std::vector<cgt::scan::DiscoveredFile>& files,
                              const std::vector<std::size_t>& selectedIds,
                              const std::filesystem::path& rootDir,
                              std::wstring outputToken,
                              bool replaceDefault,
                              bool wrappedDefault);
    };
}
