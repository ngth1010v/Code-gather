#include "ui/panel/PanelCommon.h"

#include <algorithm>

namespace cgt::ui::panel
{
    namespace
    {
        PanelLine MakeEmptyLine()
        {
            return PanelLine{};
        }
    }

    int Panel::Init()
    {
        if (m_initialized)
        {
            return detail::kNoChange;
        }

        WindowSize current{};
        if (CaptureTerminalSize(current) == detail::kOk)
        {
            m_terminalSize = current;
        }

        m_initialized = true;

        m_visibleCache.assign(static_cast<std::size_t>(std::max(0, m_size.h)), MakeEmptyLine());
        m_visibleValid.assign(static_cast<std::size_t>(std::max(0, m_size.h)), 0);

        ClampOffset();
        if (HasValidGeometry())
        {
            RenderViewport(true);
            m_lastDrawPos = m_pos;
            m_lastDrawSize = m_size;
        }

        return detail::kOk;
    }

    int Panel::Destroy()
    {
        Clean();
        m_initialized = false;
        m_visibleCache.clear();
        m_visibleValid.clear();
        m_hasDrawn = false;
        return detail::kOk;
    }

    int Panel::Clean()
    {
        if (m_hasDrawn)
        {
            ClearRegion(m_lastDrawPos, m_lastDrawSize);
        }

        m_lines.clear();
        m_offset = 0;
        m_visibleCache.assign(static_cast<std::size_t>(std::max(0, m_size.h)), MakeEmptyLine());
        m_visibleValid.assign(static_cast<std::size_t>(std::max(0, m_size.h)), 0);
        m_hasDrawn = false;
        return detail::kOk;
    }

    int Panel::SetScroll(bool scrollable)
    {
        m_scrollable = scrollable;
        return detail::kOk;
    }

    int Panel::ClearRegion(const PanelPos& pos, const PanelSize& size) const
    {
        if (!m_initialized)
        {
            return detail::kOk;
        }

        if (size.w <= 0 || size.h <= 0)
        {
            return detail::kOk;
        }

        if (pos.x >= m_terminalSize.w || pos.y >= m_terminalSize.h)
        {
            return detail::kOk;
        }

        const int visibleWidth = std::max(0, std::min(size.w, m_terminalSize.w - pos.x));
        const int visibleHeight = std::max(0, std::min(size.h, m_terminalSize.h - pos.y));

        if (visibleWidth <= 0 || visibleHeight <= 0)
        {
            return detail::kOk;
        }

        const PanelLine clearLine{};
        const std::wstring spaces(static_cast<std::size_t>(visibleWidth), L' ');

        for (int row = 0; row < visibleHeight; ++row)
        {
            cgt::ui::teminal::MoveCursorTo({pos.x, pos.y + row});
            cgt::ui::teminal::SetFontColor(clearLine.color);
            cgt::ui::teminal::SetBackgroundColor(clearLine.bgColor);
            cgt::ui::teminal::Print(spaces);
        }

        return detail::kOk;
    }

    int Panel::RenderRow(int rowIndex, const PanelLine& newLine, const PanelLine* oldLine, bool forceFullWidth)
    {
        if (!m_initialized)
        {
            return detail::kOk;
        }

        if (rowIndex < 0 || rowIndex >= m_size.h)
        {
            return detail::kError;
        }

        if (m_pos.y + rowIndex < 0 || m_pos.y + rowIndex >= m_terminalSize.h)
        {
            return detail::kOk;
        }

        const std::size_t maxWidth = GetVisibleWidth();
        const std::size_t repaintCount = detail::CountForRepaint(newLine, oldLine, maxWidth, forceFullWidth);

        if (repaintCount == 0)
        {
            return detail::kNoChange;
        }

        const std::wstring output = detail::PadText(newLine.text, repaintCount);
        if (output.empty())
        {
            return detail::kNoChange;
        }

        if (m_pos.x >= m_terminalSize.w)
        {
            return detail::kNoChange;
        }

        cgt::ui::teminal::MoveCursorTo({m_pos.x, m_pos.y + rowIndex});
        cgt::ui::teminal::SetFontColor(newLine.color);
        cgt::ui::teminal::SetBackgroundColor(newLine.bgColor);
        cgt::ui::teminal::Print(output);

        return detail::kOk;
    }

    int Panel::RenderViewport(bool forceAll)
    {
        if (!m_initialized)
        {
            return detail::kOk;
        }

        if (m_size.h <= 0 || m_size.w <= 0)
        {
            return detail::kOk;
        }

        if (m_visibleCache.size() != static_cast<std::size_t>(m_size.h))
        {
            m_visibleCache.assign(static_cast<std::size_t>(m_size.h), MakeEmptyLine());
            m_visibleValid.assign(static_cast<std::size_t>(m_size.h), 0);
        }

        for (int row = 0; row < m_size.h; ++row)
        {
            const int lineIndex = m_offset + row;
            PanelLine current = (lineIndex >= 0 && lineIndex < static_cast<int>(m_lines.size()))
                ? m_lines[static_cast<std::size_t>(lineIndex)]
                : PanelLine{};

            const PanelLine* oldLine = nullptr;
            if (row >= 0 && row < static_cast<int>(m_visibleValid.size()) && m_visibleValid[static_cast<std::size_t>(row)] != 0)
            {
                oldLine = &m_visibleCache[static_cast<std::size_t>(row)];
            }

            const bool mustForce = forceAll || oldLine == nullptr;
            RenderRow(row, current, oldLine, mustForce);

            m_visibleCache[static_cast<std::size_t>(row)] = current;
            m_visibleValid[static_cast<std::size_t>(row)] = 1;
        }

        m_hasDrawn = true;
        m_lastDrawPos = m_pos;
        m_lastDrawSize = m_size;
        return detail::kOk;
    }
}
