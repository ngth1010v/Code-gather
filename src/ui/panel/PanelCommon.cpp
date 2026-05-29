#include "ui/panel/PanelCommon.h"

#include <algorithm>

namespace cgt::ui::panel::detail
{
    bool IsSameRgb(const RGB& lhs, const RGB& rhs)
    {
        return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b;
    }

    std::size_t ClipLength(std::size_t value, std::size_t maxWidth)
    {
        return std::min(value, maxWidth);
    }

    std::wstring ClipText(const std::wstring& text, std::size_t maxWidth)
    {
        if (maxWidth == 0)
        {
            return {};
        }

        return text.substr(0, ClipLength(text.size(), maxWidth));
    }

    std::wstring PadText(const std::wstring& text, std::size_t count)
    {
        if (count == 0)
        {
            return {};
        }

        std::wstring out = ClipText(text, count);
        if (out.size() < count)
        {
            out.append(count - out.size(), L' ');
        }
        return out;
    }

    std::size_t VisibleTextLength(const PanelLine& line, std::size_t maxWidth)
    {
        return ClipLength(line.text.size(), maxWidth);
    }

    bool SameVisibleLine(const PanelLine& lhs, const PanelLine& rhs, std::size_t maxWidth)
    {
        return IsSameRgb(lhs.color, rhs.color)
            && IsSameRgb(lhs.bgColor, rhs.bgColor)
            && ClipText(lhs.text, maxWidth) == ClipText(rhs.text, maxWidth);
    }

    std::size_t CountForRepaint(const PanelLine& current, const PanelLine* oldLine, std::size_t maxWidth, bool forceFullWidth)
    {
        if (maxWidth == 0)
        {
            return 0;
        }

        if (forceFullWidth || oldLine == nullptr)
        {
            return maxWidth;
        }

        const std::size_t oldLen = VisibleTextLength(*oldLine, maxWidth);
        const std::size_t newLen = VisibleTextLength(current, maxWidth);

        if (!IsSameRgb(oldLine->bgColor, current.bgColor))
        {
            return maxWidth;
        }

        if (!IsSameRgb(oldLine->color, current.color) || ClipText(oldLine->text, maxWidth) != ClipText(current.text, maxWidth))
        {
            return std::max(oldLen, newLen);
        }

        return 0;
    }
}
