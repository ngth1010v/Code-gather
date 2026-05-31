#include "ui/panel/PanelCommon.h"

#include <algorithm>

namespace cgt::ui::panel
{
    namespace
    {
        PanelLine MakeEmptyLine(PanelLine line = {})
        {
            PanelLine emptyLine = line;
            emptyLine.text.clear();
            teminal::GetDefaultBgColor(emptyLine.bgColor);
            teminal::GetDefaultFontColor(emptyLine.color);
            return emptyLine;
        }

        bool EnsureLineIndex(std::vector<PanelLine>& lines, int y)
        {
            if (y < 1)
            {
                return false;
            }

            const std::size_t index = static_cast<std::size_t>(y - 1);
            if (index >= lines.size())
            {
                lines.resize(index + 1, MakeEmptyLine());
            }
            return true;
        }
    }

    int Panel::SetLine(int y, PanelLine line)
    {
        if (!EnsureLineIndex(m_lines, y))
        {
            return detail::kError;
        }

        const std::size_t index = static_cast<std::size_t>(y - 1);
        const bool changed = !detail::SameVisibleLine(m_lines[index], line, static_cast<std::size_t>(std::max(0, m_size.w)))
                             || m_lines[index].text.size() != line.text.size();

        m_lines[index] = std::move(line);
        ClampOffset();

        const int rowIndex = y - 1 - m_offset;
        if (m_initialized && rowIndex >= 0 && rowIndex < m_size.h)
        {
            const PanelLine* oldLine = (m_visibleValid.size() > static_cast<std::size_t>(rowIndex) && m_visibleValid[static_cast<std::size_t>(rowIndex)])
                                     ? &m_visibleCache[static_cast<std::size_t>(rowIndex)]
                                     : nullptr;

            const std::size_t maxWidth = GetVisibleWidth();
            const std::size_t repaintCount = detail::CountForRepaint(m_lines[index], oldLine, maxWidth, false);

            if (repaintCount > 0)
            {
                // Ensure cache arrays are safely expanded to hold the current rowIndex structural updates
                if (m_visibleCache.size() <= static_cast<std::size_t>(rowIndex))
                {
                    m_visibleCache.resize(static_cast<std::size_t>(m_size.h));
                    m_visibleValid.resize(static_cast<std::size_t>(m_size.h), 0);
                }

                RenderRow(rowIndex, m_lines[index], oldLine, repaintCount == maxWidth);
                
                m_visibleCache[static_cast<std::size_t>(rowIndex)] = m_lines[index];
                m_visibleValid[static_cast<std::size_t>(rowIndex)] = 1;
                m_hasDrawn = true;
            }
            else if (!m_hasDrawn && changed)
            {
                RenderViewport(true);
            }
        }

        return detail::kOk;
    }

    int Panel::RemoveLine(int y)
    {
        if (y < 1)
        {
            return detail::kError;
        }

        const std::size_t index = static_cast<std::size_t>(y - 1);
        if (index >= m_lines.size())
        {
            return detail::kNoChange;
        }

        m_lines[index] = MakeEmptyLine(m_lines[index]);
        ClampOffset();

        const int rowIndex = y - 1 - m_offset;
        if (m_initialized && rowIndex >= 0 && rowIndex < m_size.h)
        {
            RenderRow(rowIndex, m_lines[index], m_visibleValid[static_cast<std::size_t>(rowIndex)] ? &m_visibleCache[static_cast<std::size_t>(rowIndex)] : nullptr, true);
            if (m_visibleCache.size() <= static_cast<std::size_t>(rowIndex))
            {
                m_visibleCache.resize(static_cast<std::size_t>(m_size.h));
                m_visibleValid.resize(static_cast<std::size_t>(m_size.h), 0);
            }
            m_visibleCache[static_cast<std::size_t>(rowIndex)] = m_lines[index];
            m_visibleValid[static_cast<std::size_t>(rowIndex)] = 1;
            m_hasDrawn = true;
        }

        return detail::kOk;
    }

    int Panel::GetLine(int y, PanelLine& line)
    {
        if (y < 1)
        {
            return detail::kError;
        }

        const std::size_t index = static_cast<std::size_t>(y);
        if (index >= m_lines.size())
        {
            return detail::kNoChange;
        }

        line = m_lines[index];
        return detail::kOk;
    }

    int Panel::GetLines(std::vector<PanelLine>& lines)
    {
        lines = m_lines;
        return detail::kOk;
    }
}
