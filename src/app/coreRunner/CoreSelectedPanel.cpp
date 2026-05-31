#include "app/coreRunner/CoreSelectedPanel.h"

#include "app/coreRunner/CoreConstants.h"
#include "app/coreRunner/CoreUtils.h"

#include <algorithm>
#include <vector>

namespace cgt::app::coreRunner
{
    namespace
    {
        std::size_t MaxDisplayDigits(const TreeNode& root)
        {
            return CountDigits(std::max<std::size_t>(1, GetMaxFileIndex(root)));
        }

        void AppendSelectedRecursive(const TreeNode& node,
                                     const std::wstring& prefix,
                                     bool isLast,
                                     std::size_t maxIndexDigits,
                                     const cgt::RGB& fg,
                                     const cgt::RGB& bg,
                                     std::vector<cgt::ui::panel::PanelLine>& out)
        {
            const std::wstring text = MakeNodeLineText(node, prefix, isLast, maxIndexDigits);
            out.push_back(MakePanelLine(text, fg, bg));

            std::wstring nextPrefix = prefix;
            nextPrefix += isLast ? L"    " : L"│   ";

            for (std::size_t i = 0; i < node.children.size(); ++i)
            {
                AppendSelectedRecursive(node.children[i],
                                        nextPrefix,
                                        i + 1 == node.children.size(),
                                        maxIndexDigits,
                                        fg,
                                        bg,
                                        out);
            }
        }

        std::vector<cgt::ui::panel::PanelLine> BuildSelectedLines(const TreeNode& root, std::size_t width)
        {
            const std::size_t maxIndexDigits = MaxDisplayDigits(root);
            const auto [defaultFg, defaultBg] = GetTerminalDefaultColors();

            std::vector<cgt::ui::panel::PanelLine> lines;
            lines.reserve(root.fileCount + 2);

            lines.push_back(MakeFilledPanelLine(MakeRootLineText(root, maxIndexDigits), width, defaultFg, defaultBg));

            for (std::size_t i = 0; i < root.children.size(); ++i)
            {
                AppendSelectedRecursive(root.children[i],
                                        L"",
                                        i + 1 == root.children.size(),
                                        maxIndexDigits,
                                        defaultFg,
                                        defaultBg,
                                        lines);
            }

            for (auto& line : lines)
            {
                line.text = PadToWidth(line.text, width);
            }

            return lines;
        }
    }

    int CoreSelectedPanel::Init()
    {
        return m_panel.Init();
    }

    int CoreSelectedPanel::Destroy()
    {
        return m_panel.Destroy();
    }

    int CoreSelectedPanel::SetScroll(bool scrollable)
    {
        return m_panel.SetScroll(scrollable);
    }

    int CoreSelectedPanel::SetSize(cgt::ui::panel::PanelSize size)
    {
        size.h--;
        return m_panel.SetSize(size);
    }

    int CoreSelectedPanel::SetPos(cgt::ui::panel::PanelPos pos)
    {
        pos.y++;
        return m_panel.SetPos(pos);
    }

    int CoreSelectedPanel::GetSize(cgt::ui::panel::PanelSize& size)
    {
        int res = m_panel.GetSize(size);
        size.h++;
        return res;
    }

    int CoreSelectedPanel::OnResize(cgt::ui::teminal::WindowSize& size)
    {
        return m_panel.OnResize(size);
    }

    int CoreSelectedPanel::OnMouseScroll(cgt::ui::teminal::CursorPos& pos, int& delta)
    {
        return m_panel.OnMouseScroll(pos, delta);
    }

    int CoreSelectedPanel::Render(const TreeNode& root, bool forceReprint)
    {
        cgt::ui::panel::PanelSize size;
        if (m_panel.GetSize(size) != 0 || size.w <= 0 || size.h <= 0)
        {
            return 0;
        }

        if (forceReprint) {

            cgt::ui::panel::PanelPos pos;
            cgt::ui::panel::PanelSize size;
            m_panel.GetPos(pos);
            m_panel.GetSize(size);

            ui::teminal::MoveCursorTo({pos.x, pos.y-1});
            ui::teminal::SetBackgroundColor(kHeaderBackground);
            ui::teminal::SetFontColor(kSelectedHeaderFg);
            ui::teminal::Print(PadToWidth(kSelectedHeaderLabel, size.w));
        }

        const auto lines = BuildSelectedLines(root, static_cast<std::size_t>(size.w));
        SyncPanelLines(m_panel, lines, forceReprint);
        return 0;
    }
}
