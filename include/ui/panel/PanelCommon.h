#pragma once

#include <string>

#include "ui/Panel.h"

namespace cgt::ui::panel::detail
{
    constexpr int kOk       = 0;
    constexpr int kNoChange = 1;
    constexpr int kError    = -1;

    bool IsSameRgb(const RGB& lhs, const RGB& rhs);

    std::size_t ClipLength(std::size_t value, std::size_t maxWidth);
    std::wstring ClipText(const std::wstring& text, std::size_t maxWidth);
    std::wstring PadText(const std::wstring& text, std::size_t count);

    std::size_t VisibleTextLength(const PanelLine& line, std::size_t maxWidth);
    bool SameVisibleLine(const PanelLine& lhs, const PanelLine& rhs, std::size_t maxWidth);

    std::size_t CountForRepaint(const PanelLine& current, const PanelLine* oldLine, std::size_t maxWidth, bool forceFullWidth);
}
