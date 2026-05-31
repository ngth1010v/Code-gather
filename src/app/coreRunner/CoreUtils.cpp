#include "app/coreRunner/CoreUtils.h"
#include "log/Logger.h"

#include "ui/Terminal.h"

#include <string>
#include <algorithm>


namespace cgt::app::coreRunner
{
    std::pair<cgt::RGB, cgt::RGB> GetTerminalDefaultColors()
    {
        cgt::RGB fg{ 255, 255, 255 };
        cgt::RGB bg{ 0, 0, 0 };
        cgt::ui::teminal::GetDefaultFontColor(fg);
        cgt::ui::teminal::GetDefaultBgColor(bg);
        return { fg, bg };
    }

    cgt::RGB GetTerminalDefaultFontColor()
    {
        return GetTerminalDefaultColors().first;
    }

    cgt::RGB GetTerminalDefaultBgColor()
    {
        return GetTerminalDefaultColors().second;
    }

    std::size_t CountDigits(std::size_t value)
    {
        std::size_t digits = 1;
        while (value >= 10)
        {
            value /= 10;
            ++digits;
        }
        return digits;
    }

    std::wstring ClipToWidth(std::wstring_view text, std::size_t width)
    {
        if (width == 0)
        {
            return {};
        }

        if (text.size() <= width)
        {
            return std::wstring(text);
        }

        return std::wstring(text.substr(0, width));
    }

    std::wstring PadToWidth(std::wstring_view text, std::size_t width)
    {
        std::wstring out = ClipToWidth(text, width);
        if (out.size() < width)
        {
            out.append(width - out.size(), L' ');
        }
        return out;
    }

    std::wstring CursorMarker(std::wstring_view text, std::size_t cursor)
    {
        const std::size_t pos = std::min(cursor, text.size());
        std::wstring out;
        out.reserve(text.size() + 1);
        out.append(text.substr(0, pos));
        out.push_back(L'|');
        out.append(text.substr(pos));
        return out;
    }

    std::vector<std::wstring> WrapToWidth(std::wstring_view text, std::size_t width)
    {
        std::vector<std::wstring> lines;
        const std::size_t wrapWidth = std::max<std::size_t>(1, width);

        if (text.empty())
        {
            lines.emplace_back();
            return lines;
        }

        for (std::size_t pos = 0; pos < text.size(); pos += wrapWidth)
        {
            const std::size_t len = std::min(wrapWidth, text.size() - pos);
            lines.emplace_back(text.substr(pos, len));
        }

        return lines;
    }

    cgt::ui::panel::PanelLine MakePanelLine(std::wstring text,
                                            const cgt::RGB& fg,
                                            const cgt::RGB& bg)
    {
        cgt::ui::panel::PanelLine line;
        line.text = std::move(text);
        line.color = fg;
        line.bgColor = bg;
        return line;
    }

    cgt::ui::panel::PanelLine MakeFilledPanelLine(std::wstring_view text,
                                                  std::size_t width,
                                                  const cgt::RGB& fg,
                                                  const cgt::RGB& bg)
    {
        return MakePanelLine(PadToWidth(text, width), fg, bg);
    }

    void ApplyPanelGeometry(cgt::ui::panel::Panel& panel,
                            int x,
                            int y,
                            int w,
                            int h)
    {
        cgt::ui::panel::PanelPos pos;
        pos.absolute = true;
        pos.x = x;
        pos.y = y;
        panel.SetPos(pos);

        cgt::ui::panel::PanelSize size;
        size.absolute = true;
        size.w = w;
        size.h = h;
        panel.SetSize(size);
    }

    void SyncPanelLines(cgt::ui::panel::Panel& panel,
                        const std::vector<cgt::ui::panel::PanelLine>& lines,
                        bool forceReprint)
    {
        std::vector<cgt::ui::panel::PanelLine> current;
        panel.GetLines(current);

        cgt::log::Logger::Info(L"CoreUtils", std::to_wstring(current.size()) + L' ' + std::to_wstring(lines.size()));

        if (current.size() > lines.size())
        {
            for (std::size_t i = lines.size(); i <= current.size(); i++)
            {
                panel.RemoveLine(static_cast<int>(i));
            }
        }

        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            panel.SetLine(static_cast<int>(i+1), lines[i]);
        }

        if (forceReprint)
        {
            panel.RePrint();
        }
    }
}
