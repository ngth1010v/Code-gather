#include "ui/panel/PanelCommon.h"

#include <algorithm>
#include <cmath>

namespace cgt::ui::panel
{
    namespace
    {
        PanelLine MakeEmptyLine()
        {
            PanelLine line{};
            teminal::GetDefaultBgColor(line.bgColor);
            teminal::GetDefaultFontColor(line.color);
            return line;
        }

        int RoundToInt(float value)
        {
            return static_cast<int>(std::lround(value));
        }

        int ClampNonNegative(int value)
        {
            return value < 0 ? 0 : value;
        }
    }

    int Panel::CaptureTerminalSize(WindowSize& size) const
    {
        return cgt::ui::teminal::GetSize(size) ? detail::kOk : detail::kError;
    }

    int Panel::RefreshRelativeFromAbsolute()
    {
        if (m_terminalSize.w > 0)
        {
            m_pos.rx = static_cast<float>(m_pos.x) / static_cast<float>(m_terminalSize.w);
            m_size.rw = static_cast<float>(m_size.w) / static_cast<float>(m_terminalSize.w);
        }
        else
        {
            m_pos.rx = 0.0f;
            m_size.rw = 0.0f;
        }

        if (m_terminalSize.h > 0)
        {
            m_pos.ry = static_cast<float>(m_pos.y) / static_cast<float>(m_terminalSize.h);
            m_size.rh = static_cast<float>(m_size.h) / static_cast<float>(m_terminalSize.h);
        }
        else
        {
            m_pos.ry = 0.0f;
            m_size.rh = 0.0f;
        }

        m_pos.absolute = true;
        m_size.absolute = true;
        return detail::kOk;
    }

    int Panel::RefreshAbsoluteFromRelative()
    {
        if (m_terminalSize.w > 0)
        {
            m_pos.x = RoundToInt(m_pos.rx * static_cast<float>(m_terminalSize.w));
            m_size.w = RoundToInt(m_size.rw * static_cast<float>(m_terminalSize.w));
        }
        else
        {
            m_pos.x = 0;
            m_size.w = 0;
        }

        if (m_terminalSize.h > 0)
        {
            m_pos.y = RoundToInt(m_pos.ry * static_cast<float>(m_terminalSize.h));
            m_size.h = RoundToInt(m_size.rh * static_cast<float>(m_terminalSize.h));
        }
        else
        {
            m_pos.y = 0;
            m_size.h = 0;
        }

        m_pos.x = ClampNonNegative(m_pos.x);
        m_pos.y = ClampNonNegative(m_pos.y);
        m_size.w = ClampNonNegative(m_size.w);
        m_size.h = ClampNonNegative(m_size.h);

        m_pos.absolute = false;
        m_size.absolute = false;
        return detail::kOk;
    }

    std::size_t Panel::GetVisibleWidth() const
    {
        if (m_size.w <= 0 || m_pos.x >= m_terminalSize.w)
        {
            return 0;
        }

        const int remaining = m_terminalSize.w - m_pos.x;
        return static_cast<std::size_t>(std::max(0, std::min(m_size.w, remaining)));
    }

    std::size_t Panel::GetVisibleHeight() const
    {
        if (m_size.h <= 0 || m_pos.y >= m_terminalSize.h)
        {
            return 0;
        }

        const int remaining = m_terminalSize.h - m_pos.y;
        return static_cast<std::size_t>(std::max(0, std::min(m_size.h, remaining)));
    }

    bool Panel::HasValidGeometry() const
    {
        return m_size.w > 0 && m_size.h > 0 && m_pos.x >= 0 && m_pos.y >= 0;
    }

    int Panel::ClampOffset()
    {
        const int lineCount = static_cast<int>(m_lines.size());
        const int maxOffset = std::max(0, lineCount - std::max(0, m_size.h));

        if (m_offset < 0)
        {
            m_offset = 0;
        }
        else if (m_offset > maxOffset)
        {
            m_offset = maxOffset;
        }

        return detail::kOk;
    }

    int Panel::SetSize(PanelSize size)
    {
        const PanelPos oldPos = m_pos;
        const PanelSize oldSize = m_size;
        const bool hadDrawn = m_hasDrawn;

        if (!size.absolute)
        {
            m_size.absolute = false;
            m_size.rw = size.rw;
            m_size.rh = size.rh;
            if (m_terminalSize.w == 0 && m_terminalSize.h == 0)
            {
                WindowSize current{};
                if (CaptureTerminalSize(current) == detail::kOk)
                {
                    m_terminalSize = current;
                }
            }
            RefreshAbsoluteFromRelative();
        }
        else
        {
            m_size.absolute = true;
            m_size.w = ClampNonNegative(size.w);
            m_size.h = ClampNonNegative(size.h);
            RefreshRelativeFromAbsolute();
        }

        const bool geometryChanged = oldPos.x != m_pos.x || oldPos.y != m_pos.y || oldSize.w != m_size.w || oldSize.h != m_size.h;

        if (geometryChanged && hadDrawn)
        {
            ClearRegion(oldPos, oldSize);
            m_hasDrawn = false;
        }

        if (m_initialized)
        {
            m_visibleCache.assign(static_cast<std::size_t>(std::max(0, m_size.h)), MakeEmptyLine());
            m_visibleValid.assign(static_cast<std::size_t>(std::max(0, m_size.h)), 0);

            ClampOffset();
            RenderViewport(true);
            m_lastDrawPos = m_pos;
            m_lastDrawSize = m_size;
        }

        return detail::kOk;
    }

    int Panel::SetPos(PanelPos pos)
    {
        const PanelPos oldPos = m_pos;
        const PanelSize oldSize = m_size;
        const bool hadDrawn = m_hasDrawn;

        if (!pos.absolute)
        {
            m_pos.absolute = false;
            m_pos.rx = pos.rx;
            m_pos.ry = pos.ry;
            if (m_terminalSize.w == 0 && m_terminalSize.h == 0)
            {
                WindowSize current{};
                if (CaptureTerminalSize(current) == detail::kOk)
                {
                    m_terminalSize = current;
                }
            }
            RefreshAbsoluteFromRelative();
        }
        else
        {
            m_pos.absolute = true;
            m_pos.x = ClampNonNegative(pos.x);
            m_pos.y = ClampNonNegative(pos.y);
            RefreshRelativeFromAbsolute();
        }

        const bool geometryChanged = oldPos.x != m_pos.x || oldPos.y != m_pos.y || oldSize.w != m_size.w || oldSize.h != m_size.h;

        if (geometryChanged && hadDrawn)
        {
            ClearRegion(oldPos, oldSize);
            m_hasDrawn = false;
        }

        if (m_initialized)
        {
            m_visibleCache.assign(static_cast<std::size_t>(std::max(0, m_size.h)), PanelLine{});
            m_visibleValid.assign(static_cast<std::size_t>(std::max(0, m_size.h)), 0);

            ClampOffset();
            RenderViewport(true);
            m_lastDrawPos = m_pos;
            m_lastDrawSize = m_size;
        }

        return detail::kOk;
    }

    int Panel::GetSize(PanelSize& size)
    {
        size = m_size;
        return detail::kOk;
    }

    int Panel::GetPos(PanelPos& pos)
    {
        pos = m_pos;
        return detail::kOk;
    }

    int Panel::GetOffset(int& offset)
    {
        offset = m_offset;
        return detail::kOk;
    }

    int Panel::OnResize(WindowSize& size)
    {
        m_terminalSize = size;

        if (m_size.absolute)
        {
            RefreshRelativeFromAbsolute();
        }
        else
        {
            RefreshAbsoluteFromRelative();
        }

        ClampOffset();

        if (m_initialized)
        {
            m_visibleCache.assign(static_cast<std::size_t>(std::max(0, m_size.h)), PanelLine{});
            m_visibleValid.assign(static_cast<std::size_t>(std::max(0, m_size.h)), 0);
            RenderViewport(true);
            m_lastDrawPos = m_pos;
            m_lastDrawSize = m_size;
        }

        return detail::kOk;
    }
}
