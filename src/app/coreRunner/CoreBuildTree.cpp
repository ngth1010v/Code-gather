#include "app/coreRunner/CoreBuildTree.h"

#include "util/PathUtil.h"

#include <algorithm>
#include <set>

namespace cgt::app::coreRunner
{
    namespace
    {
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
}
