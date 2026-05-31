#pragma once

#include "app/coreRunner/CoreConstants.h"
#include "ui/Panel.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cgt::app::coreRunner
{
    std::pair<cgt::RGB, cgt::RGB> GetTerminalDefaultColors();

    cgt::RGB GetTerminalDefaultFontColor();
    cgt::RGB GetTerminalDefaultBgColor();

    std::size_t CountDigits(std::size_t value);

    std::wstring ClipToWidth(std::wstring_view text, std::size_t width);
    std::wstring PadToWidth(std::wstring_view text, std::size_t width);
    std::wstring CursorMarker(std::wstring_view text, std::size_t cursor);

    std::vector<std::wstring> WrapToWidth(std::wstring_view text, std::size_t width);

    cgt::ui::panel::PanelLine MakePanelLine(std::wstring text,
                                            const cgt::RGB& fg,
                                            const cgt::RGB& bg);

    cgt::ui::panel::PanelLine MakeFilledPanelLine(std::wstring_view text,
                                                  std::size_t width,
                                                  const cgt::RGB& fg,
                                                  const cgt::RGB& bg);

    void ApplyPanelGeometry(cgt::ui::panel::Panel& panel,
                            int x,
                            int y,
                            int w,
                            int h);

    void SyncPanelLines(cgt::ui::panel::Panel& panel,
                        const std::vector<cgt::ui::panel::PanelLine>& lines,
                        bool forceReprint);
}
