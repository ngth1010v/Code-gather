#pragma once

#include "app/coreRunner/CoreSelectionInput.h"

#include <cstddef>
#include "ui/Terminal.h"

#include <vector>

namespace cgt::app::coreRunner
{
    struct InputActionResult
    {
        bool changed = false;
        bool finish = false;
        bool cancel = false;
    };

    InputActionResult HandleSelectionInput(SelectionEditor& editor,
                                           const std::vector<wchar_t>& keys,
                                           const std::vector<cgt::ui::teminal::CmdKey>& cmdKeys,
                                           std::size_t maxCount);
}
