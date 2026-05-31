#include "app/coreRunner/CoreInputPanel.h"

#include "app/coreRunner/CoreConstants.h"
#include "app/coreRunner/CoreUtils.h"

#include <algorithm>
#include <vector>

namespace cgt::app::coreRunner
{
    namespace
    {
        std::vector<cgt::ui::panel::PanelLine> BuildInputLines(const SelectionEditor& editor, std::size_t width)
        {
            const auto [defaultFg, defaultBg] = GetTerminalDefaultColors();
            const std::wstring text = CursorMarker(editor.Buffer(), editor.Cursor());

            std::vector<cgt::ui::panel::PanelLine> lines;
            lines.reserve(4);

            lines.push_back(MakeFilledPanelLine(kInputHeaderLabel, width, kInputHeaderFg, kHeaderBackground));

            const auto wrapped = WrapToWidth(text, std::max<std::size_t>(1, width));
            if (wrapped.empty())
            {
                lines.push_back(MakeFilledPanelLine(L"", width, defaultFg, defaultBg));
            }
            else
            {
                for (const auto& row : wrapped)
                {
                    lines.push_back(MakeFilledPanelLine(row, width, defaultFg, defaultBg));
                }
            }

            for (auto& line : lines)
            {
                line.text = PadToWidth(line.text, width);
            }

            return lines;
        }
    }

    int CoreInputPanel::Init()
    {
        return m_panel.Init();
    }

    int CoreInputPanel::Destroy()
    {
        return m_panel.Destroy();
    }

    int CoreInputPanel::SetScroll(bool scrollable)
    {
        return m_panel.SetScroll(scrollable);
    }

    int CoreInputPanel::SetSize(cgt::ui::panel::PanelSize size)
    {
        return m_panel.SetSize(size);
    }

    int CoreInputPanel::SetPos(cgt::ui::panel::PanelPos pos)
    {
        return m_panel.SetPos(pos);
    }

    int CoreInputPanel::GetSize(cgt::ui::panel::PanelSize& size)
    {
        return m_panel.GetSize(size);
    }

    int CoreInputPanel::OnResize(cgt::ui::teminal::WindowSize& size)
    {
        return m_panel.OnResize(size);
    }

    int CoreInputPanel::OnMouseScroll(cgt::ui::teminal::CursorPos& pos, int& delta)
    {
        return m_panel.OnMouseScroll(pos, delta);
    }

    std::size_t CoreInputPanel::RequiredHeight(std::size_t width, const SelectionEditor& editor) const
    {
        const std::wstring text = CursorMarker(editor.Buffer(), editor.Cursor());
        const std::size_t wrapWidth = std::max<std::size_t>(1, width);
        const std::size_t wrappedLines = std::max<std::size_t>(1, WrapToWidth(text, wrapWidth).size());
        return 1 + wrappedLines;
    }

    int CoreInputPanel::Render(const SelectionEditor& editor, bool forceReprint)
    {
        cgt::ui::panel::PanelSize size;
        if (m_panel.GetSize(size) != 0 || size.w <= 0 || size.h <= 0)
        {
            return 0;
        }

        const auto lines = BuildInputLines(editor, static_cast<std::size_t>(size.w));
        SyncPanelLines(m_panel, lines, forceReprint);
        return 0;
    }
}
