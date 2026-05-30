#pragma once

#include "scan/DiscoveryScanner.h"
#include "ui/Panel.h"

#include <filesystem>
#include <optional>
#include <set>
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
        std::vector<TreeNode> children;
    };

    TreeNode BuildTree(const std::vector<cgt::scan::DiscoveredFile>& files, const fs::path& rootDir);
    TreeNode BuildSelectedTree(const std::vector<cgt::scan::DiscoveredFile>& files,
                               const std::vector<std::size_t>& selectedIds,
                               const fs::path& rootDir);
    std::vector<std::wstring> BuildTreeLines(const TreeNode& root);
    std::vector<std::size_t> CollectFileIndices(const TreeNode& root);

    std::vector<cgt::ui::panel::PanelLine> BuildTreePanelLines(const TreeNode& root,
                                                              std::wstring_view header,
                                                              const std::set<std::size_t>& selectedIds = {},
                                                              std::optional<std::size_t> activeId = std::nullopt);

    cgt::ui::panel::PanelLine MakeHeaderLine(std::wstring_view text);
}