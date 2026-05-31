#include "app/coreRunner/CoreDetectedPanel.h"

#include "app/coreRunner/CoreConstants.h"
#include "app/coreRunner/CoreUtils.h"
#include "config/Config.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <vector>

namespace cgt::app::coreRunner
{
    namespace
    {
        cgt::RGB WeightedDirectoryColor(const TreeNode& node)
        {
            const auto [defaultFg, defaultBg] = GetTerminalDefaultColors();
            (void)defaultBg;

            std::uint64_t sumR = 0;
            std::uint64_t sumG = 0;
            std::uint64_t sumB = 0;
            std::uint64_t total = 0;

            for (const auto& [ext, count] : node.extWeights)
            {
                if (count <= 0)
                {
                    continue;
                }

                const cgt::RGB color = cgt::config::GetExtColor(ext);
                sumR += static_cast<std::uint64_t>(color.r) * static_cast<std::uint64_t>(count);
                sumG += static_cast<std::uint64_t>(color.g) * static_cast<std::uint64_t>(count);
                sumB += static_cast<std::uint64_t>(color.b) * static_cast<std::uint64_t>(count);
                total += static_cast<std::uint64_t>(count);
            }

            if (total == 0)
            {
                return defaultFg;
            }

            return cgt::RGB{
                static_cast<std::uint8_t>(sumR / total),
                static_cast<std::uint8_t>(sumG / total),
                static_cast<std::uint8_t>(sumB / total)
            };
        }

        std::size_t MaxDisplayDigits(const TreeNode& root)
        {
            return CountDigits(std::max<std::size_t>(1, GetMaxFileIndex(root)));
        }


        bool IsNodeFullySelected(const TreeNode& node, const std::set<std::size_t>& selectedIds)
        {
            if (node.isFile)
            {
                return selectedIds.contains(node.fileIndex);
            }

            bool hasAnyFile = false;
            for (const auto& child : node.children)
            {
                if (child.isFile)
                {
                    hasAnyFile = true;
                    if (!selectedIds.contains(child.fileIndex))
                    {
                        return false;
                    }
                }
                else
                {
                    if (child.fileCount > 0)
                    {
                        hasAnyFile = true;
                    }

                    if (!IsNodeFullySelected(child, selectedIds))
                    {
                        return false;
                    }
                }
            }

            return hasAnyFile;
        }

        void AppendDetectedRecursive(const TreeNode& node,
                                     const std::wstring& prefix,
                                     bool isLast,
                                     std::size_t maxIndexDigits,
                                     const std::set<std::size_t>& selectedIds,
                                     const std::optional<std::size_t>& activeId,
                                     std::vector<cgt::ui::panel::PanelLine>& out)
        {
            const auto [defaultFg, defaultBg] = GetTerminalDefaultColors();
            const std::wstring text = MakeNodeLineText(node, prefix, isLast, maxIndexDigits);

            if (node.isFile)
            {
                if (activeId.has_value() && *activeId == node.fileIndex)
                {
                    out.push_back(MakePanelLine(text, defaultFg, kDetectedActiveBackground));
                }
                else if (selectedIds.contains(node.fileIndex))
                {
                    out.push_back(MakePanelLine(text, defaultFg, kDetectedSelectedBackground));
                }
                else
                {
                    out.push_back(MakePanelLine(text,
                                                cgt::config::GetExtColor(node.absolutePath.extension().wstring()),
                                                defaultBg));
                }
            }
            else
            {
                if (node.fileIndex != 0 && IsNodeFullySelected(node, selectedIds))
                {
                    out.push_back(MakePanelLine(text, defaultFg, kDetectedSelectedBackground));
                }
                else
                {
                    out.push_back(MakePanelLine(text, WeightedDirectoryColor(node), defaultBg));
                }
            }

            std::wstring nextPrefix = prefix;
            nextPrefix += isLast ? L"    " : L"│   ";

            for (std::size_t i = 0; i < node.children.size(); ++i)
            {
                AppendDetectedRecursive(node.children[i],
                                        nextPrefix,
                                        i + 1 == node.children.size(),
                                        maxIndexDigits,
                                        selectedIds,
                                        activeId,
                                        out);
            }
        }

        std::vector<cgt::ui::panel::PanelLine> BuildDetectedLines(const TreeNode& root,
                                                                  std::size_t width,
                                                                  const SelectionParseState& selection)
        {
            const std::size_t maxIndexDigits = MaxDisplayDigits(root);
            const auto [defaultFg, defaultBg] = GetTerminalDefaultColors();

            std::vector<cgt::ui::panel::PanelLine> lines;
            lines.reserve(root.fileCount + 2);

            // lines.push_back(MakeFilledPanelLine(kDetectedHeaderLabel, width, kDetectedHeaderFg, kHeaderBackground));
            lines.push_back(MakeFilledPanelLine(MakeRootLineText(root, maxIndexDigits), width, defaultFg, defaultBg));

            for (std::size_t i = 0; i < root.children.size(); ++i)
            {
                AppendDetectedRecursive(root.children[i],
                                        L"",
                                        i + 1 == root.children.size(),
                                        maxIndexDigits,
                                        selection.selectedSet,
                                        selection.activeId,
                                        lines);
            }

            for (auto& line : lines)
            {
                line.text = PadToWidth(line.text, width);
            }

            return lines;
        }
    }

    int CoreDetectedPanel::Init()
    {
        return m_panel.Init();
    }

    int CoreDetectedPanel::Destroy()
    {
        return m_panel.Destroy();
    }

    int CoreDetectedPanel::SetScroll(bool scrollable)
    {
        return m_panel.SetScroll(scrollable);
    }

    int CoreDetectedPanel::SetSize(cgt::ui::panel::PanelSize size)
    {
        size.h--;
        return m_panel.SetSize(size);
    }

    int CoreDetectedPanel::SetPos(cgt::ui::panel::PanelPos pos)
    {
        pos.y++;
        return m_panel.SetPos(pos);
    }

    int CoreDetectedPanel::GetSize(cgt::ui::panel::PanelSize& size)
    {
        int res = m_panel.GetSize(size);
        size.h++;
        return res;
    }

    int CoreDetectedPanel::OnResize(cgt::ui::teminal::WindowSize& size)
    {
        return m_panel.OnResize(size);
    }

    int CoreDetectedPanel::OnMouseScroll(cgt::ui::teminal::CursorPos& pos, int& delta)
    {
        return m_panel.OnMouseScroll(pos, delta);
    }

    int CoreDetectedPanel::Render(const TreeNode& root,
                                  const SelectionParseState& selection,
                                  bool forceReprint)
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
            ui::teminal::SetFontColor(kDetectedHeaderFg);
            ui::teminal::Print(PadToWidth(kDetectedHeaderLabel, size.w));
        }

        const auto lines = BuildDetectedLines(root, static_cast<std::size_t>(size.w), selection);
        SyncPanelLines(m_panel, lines, forceReprint);
        return 0;
    }
}
