#pragma once

#include "scan/DiscoveryScanner.h"

#include <cstddef>

#include <filesystem>
#include <map>
#include <string>
#include <string_view>
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

        std::map<std::wstring, int> extWeights;
        std::size_t fileCount = 0;

        std::vector<TreeNode> children;
    };

    TreeNode BuildTree(const std::vector<cgt::scan::DiscoveredFile>& files,
                       const fs::path& rootDir);

    TreeNode BuildSelectedTree(const std::vector<cgt::scan::DiscoveredFile>& files,
                               const std::vector<std::size_t>& selectedIds,
                               const fs::path& rootDir);

    std::vector<std::size_t> CollectFileIndices(const TreeNode& root);
    std::size_t GetMaxFileIndex(const TreeNode& root);

    std::wstring MakeRootLineText(const TreeNode& root, std::size_t maxIndexDigits);
    std::wstring MakeNodeLineText(const TreeNode& node,
                                  std::wstring_view prefix,
                                  bool isLast,
                                  std::size_t maxIndexDigits);

    std::vector<std::wstring> BuildTreeLines(const TreeNode& root);
}
