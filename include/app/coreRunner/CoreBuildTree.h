#pragma once

#include "scan/DiscoveryScanner.h"

#include <filesystem>
#include <string>
#include <vector>

namespace cgt::app::coreRunner
{
    namespace fs = std::filesystem;

    struct TreeNode
    {
        std::wstring name;
        fs::path absolutePath;
        std::wstring relativePath;
        bool isFile = false;
        std::size_t fileIndex = 0;
        std::vector<TreeNode> children;
    };

    TreeNode BuildTree(const std::vector<cgt::scan::DiscoveredFile>& files, const fs::path& rootDir);
    TreeNode BuildSelectedTree(const std::vector<cgt::scan::DiscoveredFile>& files,
                               const std::vector<std::size_t>& selectedIds,
                               const fs::path& rootDir);
    std::vector<std::wstring> BuildTreeLines(const TreeNode& root);
    std::vector<std::size_t> CollectFileIndices(const TreeNode& root);
}
