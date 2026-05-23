#include "log/PrintTree.h"

#include "log/Logger.h"
#include "util/PathUtil.h"
#include "util/StringUtil.h"

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
        struct RGB
        {
            int r;
            int g;
            int b;
        };

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

        RGB HSLtoRGB(double h, double s, double l)
        {
            double c = (1.0 - std::abs(2.0 * l - 1.0)) * s;
            double x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2.0) - 1.0));
            double m = l - c / 2.0;

            double rPrime = 0;
            double gPrime = 0;
            double bPrime = 0;

            if (0 <= h && h < 60)
            {
                rPrime = c;
                gPrime = x;
            }
            else if (60 <= h && h < 120)
            {
                rPrime = x;
                gPrime = c;
            }
            else if (120 <= h && h < 180)
            {
                gPrime = c;
                bPrime = x;
            }
            else if (180 <= h && h < 240)
            {
                gPrime = x;
                bPrime = c;
            }
            else if (240 <= h && h < 300)
            {
                rPrime = x;
                bPrime = c;
            }
            else
            {
                rPrime = c;
                bPrime = x;
            }

            return {
                static_cast<int>((rPrime + m) * 255),
                static_cast<int>((gPrime + m) * 255),
                static_cast<int>((bPrime + m) * 255)
            };
        }

        void PrintColoredPath(
            const std::wstring& text,
            const std::wstring& extension)
        {
            const std::wstring key =
                cgt::util::ToLower(extension);

            std::size_t hash = 1469598103934665603ULL;

            for (wchar_t ch : key)
            {
                hash ^= static_cast<std::size_t>(ch);
                hash *= 1099511628211ULL;
            }

            double h = static_cast<double>(hash % 360);
            double s = 0.85;
            double l =
                0.72 +
                (static_cast<double>(hash % 100) / 100.0) * 0.13;

            RGB color = HSLtoRGB(h, s, l);

            std::wcout
                << L"\x1b[38;2;"
                << color.r << L";"
                << color.g << L";"
                << color.b << L"m"
                << text
                << L"\x1b[0m";
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

                // std::wcout
                //     << L"("
                //     << indexText
                //     << L") ";
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

                PrintColoredPath(
                    node.name,
                    fs::path(node.name).extension().wstring());

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