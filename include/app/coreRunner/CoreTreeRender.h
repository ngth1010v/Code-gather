#pragma once

#include "app/coreRunner/CoreBuildTree.h"
#include "ui/Panel.h"

#include <optional>
#include <set>
#include <string_view>
#include <vector>

namespace cgt::app::coreRunner
{
    std::vector<cgt::ui::panel::PanelLine> BuildTreePanelLines(const TreeNode& root,
                                                               std::wstring_view header,
                                                               const std::set<std::size_t>& selectedIds = {},
                                                               std::optional<std::size_t> activeId = std::nullopt);

    cgt::ui::panel::PanelLine MakeHeaderLine(std::wstring_view text);
}
