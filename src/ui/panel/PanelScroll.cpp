#include "ui/panel/PanelCommon.h"

#include <algorithm>

namespace cgt::ui::panel
{
    bool Panel::IsInside(const CursorPos& pos) const
    {
        if (!HasValidGeometry())
        {
            return false;
        }

        const long long left = static_cast<long long>(m_pos.x);
        const long long top = static_cast<long long>(m_pos.y);
        const long long right = left + static_cast<long long>(m_size.w);
        const long long bottom = top + static_cast<long long>(m_size.h);

        return static_cast<long long>(pos.x) >= left
            && static_cast<long long>(pos.x) < right
            && static_cast<long long>(pos.y) >= top
            && static_cast<long long>(pos.y) < bottom;
    }

    int Panel::MoveUp(int step)
    {
        if (step <= 0)
        {
            return detail::kNoChange;
        }

        if (!m_scrollable)
        {
            return detail::kNoChange;
        }

        const int oldOffset = m_offset;
        m_offset = std::max(0, m_offset - step);
        ClampOffset();

        const bool reachedBoundary = (m_offset == 0) && (oldOffset == 0 || step > oldOffset);

        if (m_initialized && m_offset != oldOffset)
        {
            RenderViewport(false);
        }

        return reachedBoundary ? detail::kNoChange : detail::kOk;
    }

    int Panel::MoveDown(int step)
    {
        if (step <= 0)
        {
            return detail::kNoChange;
        }

        if (!m_scrollable)
        {
            return detail::kNoChange;
        }

        const int oldOffset = m_offset;
        const int maxOffset = std::max(0, static_cast<int>(m_lines.size()) - std::max(0, m_size.h));
        m_offset = std::min(maxOffset, m_offset + step);
        ClampOffset();

        const bool reachedBoundary = (m_offset == maxOffset);

        if (m_initialized && m_offset != oldOffset)
        {
            RenderViewport(false);
        }

        return reachedBoundary ? detail::kNoChange : detail::kOk;
    }

    int Panel::OnMouseScroll(CursorPos& pos, int& delta)
    {
        if (!IsInside(pos))
        {
            return detail::kNoChange;
        }

        if (delta == 0)
        {
            return detail::kNoChange;
        }

        if (delta > 0)
        {
            MoveUp(delta * 3);
        }
        else
        {
            MoveDown(-delta * 3);
        }

        return detail::kOk;
    }
}
