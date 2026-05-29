#pragma once

#include <string_view>

#include "ui/Terminal.h"
#include "util/Types.h"

namespace cgt::ui::teminal
{
    void AppendAltEnter();
    void AppendAltLeave();

    void AppendHideCursor();
    void AppendShowCursor();

    void AppendClear();
    void AppendReset();

    void AppendMoveTo(const CursorPos& pos);
    void AppendFontColor(const RGB& rgb);
    void AppendBackgroundColor(const RGB& rgb);
    void AppendTitle(std::wstring_view title);

    void AppendText(std::wstring_view text);
}
