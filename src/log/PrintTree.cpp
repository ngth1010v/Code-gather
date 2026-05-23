#include "log/PrintTree.h"

#include "log/Logger.h"
#include "util/PathUtil.h"
#include "util/StringUtil.h"
#include "config/Config.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

namespace cgt::log
{
    namespace fs = std::filesystem;

    namespace
    {

        struct TreeNode
        {
            std::wstring name;
            bool isFile = false;
            std::size_t fileIndex = 0;

            // QUAN TRỌNG:
            // dùng vector thay vì map để giữ đúng thứ tự gốc
            std::vector<TreeNode> children;
        };

        void EnableVirtualTerminalProcessing()
        {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

            if (hOut == INVALID_HANDLE_VALUE)
            {
                return;
            }

            DWORD dwMode = 0;

            if (GetConsoleMode(hOut, &dwMode))
            {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }
        }

        TreeNode* FindChild(
            TreeNode& parent,
            const std::wstring& name)
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

        void BuildTree(
            TreeNode& root,
            const std::vector<scan::DiscoveredFile>& files)
        {
            for (std::size_t i = 0; i < files.size(); ++i)
            {
                const auto& item = files[i];

                fs::path path(item.relativePath);

                TreeNode* current = &root;

                for (auto it = path.begin(); it != path.end(); ++it)
                {
                    const std::wstring part =
                        it->wstring();

                    TreeNode* found =
                        FindChild(*current, part);

                    if (found == nullptr)
                    {
                        TreeNode node;
                        node.name = part;

                        current->children.push_back(
                            std::move(node));

                        found =
                            &current->children.back();
                    }

                    current = found;
                }

                current->isFile = true;
                current->fileIndex = i + 1;
            }
        }

        void PrintNode(
            const TreeNode& node,
            const std::wstring& prefix,
            bool isLast,
            std::size_t maxIndexDigits)
        {
            const std::wstring connector =
                isLast
                    ? L"└── "
                    : L"├── ";

            if (node.isFile)
            {
                const std::wstring indexText =
                    std::to_wstring(node.fileIndex);

                std::wcout << indexText << L" ";

                for (std::size_t i = 0;
                     i < maxIndexDigits - indexText.size();
                     ++i)
                {
                    std::wcout << L" ";
                }

                std::wcout
                    << prefix
                    << connector;

                const auto color = cgt::config::GetExtColor(fs::path(node.name).extension().wstring());

                std::wcout
                    << L"\x1b[38;2;"
                    << color.r << L";"
                    << color.g << L";"
                    << color.b << L"m"
                    << node.name
                    << L"\x1b[0m";

                std::wcout << std::endl;
            }
            else
            {
                // std::wcout << L"(-)";
                std::wcout << L" ";

                for (std::size_t i = 0;
                     i < maxIndexDigits;
                     ++i)
                {
                    std::wcout << L" ";
                }

                std::wcout
                    << prefix
                    << connector
                    << node.name
                    << L"/"
                    << std::endl;
            }

            std::wstring nextPrefix = prefix;

            nextPrefix +=
                isLast
                    ? L"    "
                    : L"│   ";

            for (std::size_t i = 0;
                 i < node.children.size();
                 ++i)
            {
                PrintNode(
                    node.children[i],
                    nextPrefix,
                    i + 1 == node.children.size(),
                    maxIndexDigits);
            }
        }
    }

    void PrintTree::PrintFoundFilesTree(
        const std::vector<scan::DiscoveredFile>& files,
        const fs::path& rootDir)
    {
        if (files.empty())
        {
            return;
        }

        EnableVirtualTerminalProcessing();

        Logger::Plain(L"Found files:");

        TreeNode root;
        root.name = rootDir.filename().wstring();

        BuildTree(root, files);

        const std::size_t maxIndexDigits =
            std::to_wstring(files.size()).size();

        std::wcout << L" ";

        for (std::size_t i = 0;
             i < maxIndexDigits;
             ++i)
        {
            std::wcout << L" ";
        }

        std::wcout
            << root.name
            << L"/"
            << std::endl;

        for (std::size_t i = 0;
             i < root.children.size();
             ++i)
        {
            PrintNode(
                root.children[i],
                L"",
                i + 1 == root.children.size(),
                maxIndexDigits);
        }
    }
}