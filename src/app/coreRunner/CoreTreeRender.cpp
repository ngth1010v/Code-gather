#include "app/coreRunner/CoreTreeRender.h"
#include "util/PathUtil.h"

#include "config/Config.h"

#include <algorithm>
#include <optional>
#include <set>
#include <string>

namespace cgt::app::coreRunner
{
    namespace
    {
        constexpr cgt::RGB kHeaderFg{ 235, 235, 235 };
        constexpr cgt::RGB kHeaderBg{ 24, 24, 24 };
        constexpr cgt::RGB kSelectedBg{ 44, 88, 60 };
        constexpr cgt::RGB kActiveBg{ 60, 84, 140 };
        constexpr cgt::RGB kDirectoryFg{ 235, 235, 235 };

        TreeNode* FindChild(TreeNode& parent, const std::wstring& name)
        {
            for (auto& child : parent.children)
            {
                if (child.name == name)
                {
                    return &child;
                }
            }
            return nullptr;
        }

        std::wstring RootName(const fs::path& rootDir)
        {
            std::wstring name = rootDir.filename().wstring();
            if (name.empty())
            {
                name = rootDir.root_name().wstring();
            }
            if (name.empty())
            {
                name = rootDir.wstring();
            }
            if (name.empty())
            {
                name = L".";
            }
            return name;
        }

        void InsertPath(TreeNode& root,
                        const fs::path& rootDir,
                        const cgt::scan::DiscoveredFile& item,
                        std::size_t fileIndex)
        {
            fs::path rel(item.relativePath);
            TreeNode* current = &root;
            fs::path currentPath = rootDir;

            for (auto it = rel.begin(); it != rel.end(); ++it)
            {
                const std::wstring part = it->wstring();
                currentPath /= part;

                TreeNode* found = FindChild(*current, part);
                if (found == nullptr)
                {
                    TreeNode node;
                    node.name = part;
                    node.absolutePath = currentPath;
                    node.relativePath = cgt::util::RelativeDisplayPath(rootDir, currentPath);
                    current->children.push_back(std::move(node));
                    found = &current->children.back();
                }

                current = found;
            }

            current->isFile = true;
            current->fileIndex = fileIndex;
            current->absolutePath = item.absolutePath;
            current->relativePath = item.relativePath;
        }

        void BuildTreeRecursive(TreeNode& root,
                                const std::vector<cgt::scan::DiscoveredFile>& files,
                                const fs::path& rootDir)
        {
            for (std::size_t i = 0; i < files.size(); ++i)
            {
                InsertPath(root, rootDir, files[i], i + 1);
            }
        }

        void BuildSelectedTreeRecursive(TreeNode& root,
                                        const std::vector<cgt::scan::DiscoveredFile>& files,
                                        const std::set<std::size_t>& selected,
                                        const fs::path& rootDir)
        {
            for (std::size_t i = 0; i < files.size(); ++i)
            {
                if (!selected.contains(i + 1))
                {
                    continue;
                }
                InsertPath(root, rootDir, files[i], i + 1);
            }
        }

        void BuildLinesRecursive(const TreeNode& node,
                                 const std::wstring& prefix,
                                 bool isLast,
                                 std::vector<std::wstring>& lines,
                                 std::size_t maxIndexDigits)
        {
            const std::wstring connector = isLast ? L"└── " : L"├── ";
            std::wstring line;

            if (node.isFile)
            {
                const std::wstring indexText = std::to_wstring(node.fileIndex);
                line += indexText;
                line += L" ";
                if (maxIndexDigits > indexText.size())
                {
                    line.append(maxIndexDigits - indexText.size(), L' ');
                }
                line += prefix;
                line += connector;
                line += node.name;
            }
            else
            {
                line.append(maxIndexDigits, L' ');
                line += L" ";
                line += prefix;
                line += connector;
                line += node.name;
                line += L"/";
            }

            lines.push_back(std::move(line));

            std::wstring nextPrefix = prefix;
            nextPrefix += isLast ? L"    " : L"│   ";

            for (std::size_t i = 0; i < node.children.size(); ++i)
            {
                BuildLinesRecursive(node.children[i], nextPrefix, i + 1 == node.children.size(), lines, maxIndexDigits);
            }
        }

        void CollectIndicesRecursive(const TreeNode& node, std::vector<std::size_t>& out)
        {
            if (node.isFile && node.fileIndex != 0)
            {
                out.push_back(node.fileIndex);
            }

            for (const auto& child : node.children)
            {
                CollectIndicesRecursive(child, out);
            }
        }

        std::wstring RootName(const std::wstring& text)
        {
            if (text.empty())
            {
                return L"(empty)";
            }
            return text;
        }

        bool HasSelectedFile(const TreeNode& node, const std::set<std::size_t>& selected)
        {
            if (node.isFile)
            {
                return selected.contains(node.fileIndex);
            }

            for (const auto& child : node.children)
            {
                if (HasSelectedFile(child, selected))
                {
                    return true;
                }
            }
            return false;
        }

        bool IsDirectoryFullySelected(const TreeNode& node, const std::set<std::size_t>& selected)
        {
            if (node.isFile)
            {
                return selected.contains(node.fileIndex);
            }

            bool hasAnyFile = false;
            for (const auto& child : node.children)
            {
                if (child.isFile)
                {
                    hasAnyFile = true;
                    if (!selected.contains(child.fileIndex))
                    {
                        return false;
                    }
                }
                else
                {
                    if (HasSelectedFile(child, selected))
                    {
                        hasAnyFile = true;
                    }
                    if (!IsDirectoryFullySelected(child, selected))
                    {
                        return false;
                    }
                }
            }
            return hasAnyFile;
        }

        std::wstring MakeRootLineText(const TreeNode& root, std::size_t maxIndexDigits)
        {
            std::wstring line;
            line.append(maxIndexDigits, L' ');
            line += L" ";
            line += RootName(root.name);
            line += L"/";
            return line;
        }

        std::wstring MakeNodeLineText(const TreeNode& node,
                                      const std::wstring& prefix,
                                      bool isLast,
                                      std::size_t maxIndexDigits)
        {
            const std::wstring connector = isLast ? L"└── " : L"├── ";
            std::wstring line;

            if (node.isFile)
            {
                const std::wstring indexText = std::to_wstring(node.fileIndex);
                line += indexText;
                line += L" ";
                if (maxIndexDigits > indexText.size())
                {
                    line.append(maxIndexDigits - indexText.size(), L' ');
                }
                line += prefix;
                line += connector;
                line += node.name;
            }
            else
            {
                line.append(maxIndexDigits, L' ');
                line += L" ";
                line += prefix;
                line += connector;
                line += node.name;
                line += L"/";
            }

            return line;
        }

        cgt::RGB MakeFileFg(const TreeNode& node)
        {
            return cgt::config::GetExtColor(node.absolutePath.extension().wstring());
        }

        cgt::ui::panel::PanelLine MakeLine(const std::wstring& text, const cgt::RGB& fg, const cgt::RGB& bg)
        {
            cgt::ui::panel::PanelLine line;
            line.text = text;
            line.color = fg;
            line.bgColor = bg;
            return line;
        }

        cgt::ui::panel::PanelLine MakeNodeLine(const TreeNode& node,
                                               const std::wstring& prefix,
                                               bool isLast,
                                               std::size_t maxIndexDigits,
                                               const std::set<std::size_t>& selectedIds,
                                               const std::optional<std::size_t>& activeId)
        {
            const std::wstring text = MakeNodeLineText(node, prefix, isLast, maxIndexDigits);
            const bool selectedHere = node.isFile
                ? selectedIds.contains(node.fileIndex)
                : IsDirectoryFullySelected(node, selectedIds);
            const bool activeHere = node.isFile && activeId.has_value() && *activeId == node.fileIndex;

            if (activeHere)
            {
                return MakeLine(text, cgt::RGB{ 255, 255, 255 }, kActiveBg);
            }

            if (selectedHere)
            {
                return MakeLine(text, cgt::RGB{ 255, 255, 255 }, kSelectedBg);
            }

            if (node.isFile)
            {
                return MakeLine(text, MakeFileFg(node), cgt::RGB{ 0, 0, 0 });
            }

            return MakeLine(text, kDirectoryFg, cgt::RGB{ 0, 0, 0 });
        }

        void AppendRecursive(const TreeNode& node,
                             const std::wstring& prefix,
                             bool isLast,
                             std::size_t maxIndexDigits,
                             const std::set<std::size_t>& selectedIds,
                             const std::optional<std::size_t>& activeId,
                             std::vector<cgt::ui::panel::PanelLine>& out)
        {
            out.push_back(MakeNodeLine(node, prefix, isLast, maxIndexDigits, selectedIds, activeId));

            std::wstring nextPrefix = prefix;
            nextPrefix += isLast ? L"    " : L"│   ";

            for (std::size_t i = 0; i < node.children.size(); ++i)
            {
                AppendRecursive(node.children[i],
                                nextPrefix,
                                i + 1 == node.children.size(),
                                maxIndexDigits,
                                selectedIds,
                                activeId,
                                out);
            }
        }
    }

    TreeNode BuildTree(const std::vector<cgt::scan::DiscoveredFile>& files, const fs::path& rootDir)
    {
        TreeNode root;
        root.name = RootName(rootDir);
        root.absolutePath = rootDir;
        root.relativePath = L".";
        BuildTreeRecursive(root, files, rootDir);
        return root;
    }

    TreeNode BuildSelectedTree(const std::vector<cgt::scan::DiscoveredFile>& files,
                               const std::vector<std::size_t>& selectedIds,
                               const fs::path& rootDir)
    {
        std::set<std::size_t> selected(selectedIds.begin(), selectedIds.end());

        TreeNode root;
        root.name = RootName(rootDir);
        root.absolutePath = rootDir;
        root.relativePath = L".";
        BuildSelectedTreeRecursive(root, files, selected, rootDir);
        return root;
    }

    std::vector<std::wstring> BuildTreeLines(const TreeNode& root)
    {
        std::vector<std::size_t> indices = CollectFileIndices(root);
        const std::size_t maxIndexDigits = std::to_wstring(std::max<std::size_t>(1, indices.size())).size();

        std::vector<std::wstring> lines;
        lines.push_back(root.name + L"/");

        for (std::size_t i = 0; i < root.children.size(); ++i)
        {
            BuildLinesRecursive(root.children[i], L"", i + 1 == root.children.size(), lines, maxIndexDigits);
        }
        return lines;
    }

    std::vector<std::size_t> CollectFileIndices(const TreeNode& root)
    {
        std::vector<std::size_t> out;
        CollectIndicesRecursive(root, out);
        return out;
    }

    cgt::ui::panel::PanelLine MakeHeaderLine(std::wstring_view text)
    {
        return MakeLine(std::wstring(text), kHeaderFg, kHeaderBg);
    }

    std::vector<cgt::ui::panel::PanelLine> BuildTreePanelLines(const TreeNode& root,
                                                               std::wstring_view header,
                                                               const std::set<std::size_t>& selectedIds,
                                                               std::optional<std::size_t> activeId)
    {
        const std::vector<std::size_t> indices = CollectFileIndices(root);
        const std::size_t maxIndexDigits = std::to_wstring(std::max<std::size_t>(1, indices.size())).size();

        std::vector<cgt::ui::panel::PanelLine> lines;
        lines.reserve(root.children.size() + 2);
        lines.push_back(MakeHeaderLine(header));
        lines.push_back(MakeLine(MakeRootLineText(root, maxIndexDigits), kDirectoryFg, cgt::RGB{ 0, 0, 0 }));

        for (std::size_t i = 0; i < root.children.size(); ++i)
        {
            AppendRecursive(root.children[i],
                            L"",
                            i + 1 == root.children.size(),
                            maxIndexDigits,
                            selectedIds,
                            activeId,
                            lines);
        }

        return lines;
    }
}
