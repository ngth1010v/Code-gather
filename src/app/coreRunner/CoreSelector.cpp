#include "app/coreRunner/CoreSelector.h"

#include "app/AppUtils.h"
#include "app/CoreConstant.h"
#include "app/coreRunner/CoreBuildTree.h"
#include "app/coreRunner/CoreUtils.h"
#include "config/Config.h"
#include "util/StringUtil.h"

#include <algorithm>
#include <cwctype>
#include <iostream>
#include <optional>
#include <set>
#include <vector>

namespace cgt::app::coreRunner
{
    namespace
    {
        struct TokenInfo
        {
            std::size_t start = 0;
            std::size_t end = 0; // exclusive
            std::size_t value = 0;
            bool valid = false;
        };

        struct ParseState
        {
            std::vector<std::size_t> selectedIds;
            std::set<std::size_t> selectedSet;
            std::optional<std::size_t> activeId;
        };

        struct RenderLine
        {
            const TreeNode* node = nullptr;
            std::wstring prefix;
            bool isLast = false;
            std::size_t row = 0;
            bool rootLine = false;
        };

        class CursorGuard
        {
        public:
            CursorGuard() = default;
            CursorGuard(const CursorGuard&) = delete;
            CursorGuard& operator=(const CursorGuard&) = delete;
            ~CursorGuard()
            {
                std::wcout << ShowCursor();
                std::wcout.flush();
            }
        };

        bool IsSpace(wchar_t ch)
        {
            return std::iswspace(ch) != 0;
        }

        std::vector<TokenInfo> ParseTokens(const std::wstring& buffer, std::size_t maxCount)
        {
            std::vector<TokenInfo> tokens;

            std::size_t i = 0;
            while (i < buffer.size())
            {
                while (i < buffer.size() && IsSpace(buffer[i]))
                {
                    ++i;
                }

                const std::size_t begin = i;
                while (i < buffer.size() && !IsSpace(buffer[i]))
                {
                    ++i;
                }
                const std::size_t end = i;

                if (begin >= end)
                {
                    continue;
                }

                TokenInfo token;
                token.start = begin;
                token.end = end;

                bool numeric = true;
                for (std::size_t j = begin; j < end; ++j)
                {
                    if (!std::iswdigit(buffer[j]))
                    {
                        numeric = false;
                        break;
                    }
                }

                if (numeric)
                {
                    try
                    {
                        token.value = static_cast<std::size_t>(std::stoull(buffer.substr(begin, end - begin)));
                        token.valid = token.value >= 1 && token.value <= maxCount;
                    }
                    catch (...)
                    {
                        token.valid = false;
                    }
                }

                tokens.push_back(token);
            }

            return tokens;
        }

        ParseState ParseBuffer(const std::wstring& buffer,
                               std::size_t cursor,
                               std::size_t maxCount)
        {
            ParseState state;
            const auto tokens = ParseTokens(buffer, maxCount);
            cursor = std::min(cursor, buffer.size());

            for (const auto& token : tokens)
            {
                if (cursor >= token.start && cursor <= token.end && token.valid)
                {
                    state.activeId = token.value;
                    break;
                }
            }

            std::set<std::size_t> seen;
            for (const auto& token : tokens)
            {
                if (!token.valid)
                {
                    continue;
                }

                if (seen.insert(token.value).second)
                {
                    state.selectedIds.push_back(token.value);
                    state.selectedSet.insert(token.value);
                }
            }

            return state;
        }

        std::wstring LastValidTokenText(const std::wstring& buffer, std::size_t maxCount)
        {
            const auto tokens = ParseTokens(buffer, maxCount);
            for (auto it = tokens.rbegin(); it != tokens.rend(); ++it)
            {
                if (it->valid)
                {
                    return buffer.substr(it->start, it->end - it->start);
                }
            }
            return {};
        }

        void ReplaceActiveToken(std::wstring& buffer,
                                std::size_t& cursor,
                                std::size_t maxCount,
                                std::size_t newValue)
        {
            const auto tokens = ParseTokens(buffer, maxCount);
            for (const auto& token : tokens)
            {
                if (cursor >= token.start && cursor <= token.end && token.valid)
                {
                    const std::wstring text = std::to_wstring(newValue);
                    buffer.replace(token.start, token.end - token.start, text);
                    cursor = token.start + text.size();
                    return;
                }
            }

            InsertText(buffer, cursor, std::to_wstring(newValue));
        }

        void CollectRenderLines(const TreeNode& node,
                                std::wstring prefix,
                                bool isLast,
                                std::size_t& row,
                                std::vector<RenderLine>& lines)
        {
            lines.push_back(RenderLine{ &node, std::move(prefix), isLast, row++, false });
            const std::wstring nextPrefix = lines.back().prefix + (isLast ? L"    " : L"│   ");
            for (std::size_t i = 0; i < node.children.size(); ++i)
            {
                CollectRenderLines(node.children[i], nextPrefix, i + 1 == node.children.size(), row, lines);
            }
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

        std::wstring MakeRootLine(const TreeNode& root,
                                  std::size_t maxIndexDigits,
                                  const std::set<std::size_t>& selected)
        {
            std::wstring line;
            const bool bgSelected = IsDirectoryFullySelected(root, selected);
            if (bgSelected)
            {
                line += MakeAnsiBg(kSelectedBackground);
            }

            line += std::wstring(maxIndexDigits, L' ');
            line += L" ";
            line += root.name;
            line += L"/";

            if (bgSelected)
            {
                line += MakeAnsiReset();
            }
            return line;
        }

        std::wstring MakeNodeLine(const TreeNode& node,
                                 const std::wstring& prefix,
                                 bool isLast,
                                 std::size_t maxIndexDigits,
                                 const std::set<std::size_t>& selected,
                                 const std::optional<std::size_t>& activeId)
        {
            const std::wstring connector = isLast ? L"└── " : L"├── ";
            const bool selectedHere = node.isFile ? selected.contains(node.fileIndex) : IsDirectoryFullySelected(node, selected);
            const bool activeHere = node.isFile && activeId.has_value() && *activeId == node.fileIndex;
            const cgt::config::RGB* bg = nullptr;
            if (activeHere)
            {
                bg = &kSelectingBackground;
            }
            else if (selectedHere)
            {
                bg = &kSelectedBackground;
            }

            std::wstring line;
            if (bg != nullptr)
            {
                line += MakeAnsiBg(*bg);
            }

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
                const cgt::config::RGB extColor = cgt::config::GetExtColor(node.absolutePath.extension().wstring());
                line += MakeAnsiFg(extColor);
                line += node.name;
                line += MakeAnsiReset();
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

            if (bg != nullptr)
            {
                line += MakeAnsiReset();
            }
            return line;
        }

        void WriteLine(std::size_t row, const std::wstring& text)
        {
            std::wcout << MoveCursorTo(row, 1) << ClearCurrentLine() << text;
            std::wcout.flush();
        }

        void RenderPrompt(std::size_t row, const std::wstring& buffer, std::size_t cursor)
        {
            WriteLine(row, L"Selected files: " + InsertCursorMarker(buffer, cursor));
        }

        void RenderTree(const TreeNode& root,
                        const std::vector<RenderLine>& lines,
                        const ParseState& state,
                        std::size_t maxIndexDigits)
        {
            WriteLine(3, MakeRootLine(root, maxIndexDigits, state.selectedSet));

            for (const auto& line : lines)
            {
                WriteLine(line.row, MakeNodeLine(*line.node, line.prefix, line.isLast, maxIndexDigits, state.selectedSet, state.activeId));
            }
        }

        std::size_t CountRenderedRows(const std::vector<RenderLine>& lines)
        {
            std::size_t row = 4;
            if (!lines.empty())
            {
                row = lines.back().row + 1;
            }
            return row;
        }

        void RenderScreen(const TreeNode& root,
                          const std::vector<RenderLine>& lines,
                          const ParseState& state,
                          const std::wstring& buffer,
                          std::size_t cursor,
                          std::size_t maxIndexDigits,
                          std::size_t promptRow)
        {
            ClearScreen();
            WriteLine(1, MakeDividerLine());
            WriteLine(2, L"List of detected files");
            WriteLine(3, MakeRootLine(root, maxIndexDigits, state.selectedSet));
            for (const auto& line : lines)
            {
                WriteLine(line.row, MakeNodeLine(*line.node, line.prefix, line.isLast, maxIndexDigits, state.selectedSet, state.activeId));
            }
            WriteLine(promptRow - 1, MakeDividerLine());
            RenderPrompt(promptRow, buffer, cursor);
            std::wcout.flush();
        }

        void UpdateChangedLines(const TreeNode& root,
                                const std::vector<RenderLine>& lines,
                                const ParseState& prev,
                                const ParseState& next,
                                std::size_t maxIndexDigits)
        {
            if (prev.selectedSet != next.selectedSet)
            {
                WriteLine(3, MakeRootLine(root, maxIndexDigits, next.selectedSet));
                for (const auto& line : lines)
                {
                    const bool prevSelected = line.node->isFile ? prev.selectedSet.contains(line.node->fileIndex) : IsDirectoryFullySelected(*line.node, prev.selectedSet);
                    const bool nextSelected = line.node->isFile ? next.selectedSet.contains(line.node->fileIndex) : IsDirectoryFullySelected(*line.node, next.selectedSet);
                    const bool prevActive = line.node->isFile && prev.activeId.has_value() && *prev.activeId == line.node->fileIndex;
                    const bool nextActive = line.node->isFile && next.activeId.has_value() && *next.activeId == line.node->fileIndex;
                    if (prevSelected != nextSelected || prevActive != nextActive)
                    {
                        WriteLine(line.row, MakeNodeLine(*line.node, line.prefix, line.isLast, maxIndexDigits, next.selectedSet, next.activeId));
                    }
                }
                return;
            }

            if (prev.activeId != next.activeId)
            {
                for (const auto& line : lines)
                {
                    const bool prevActive = line.node->isFile && prev.activeId.has_value() && *prev.activeId == line.node->fileIndex;
                    const bool nextActive = line.node->isFile && next.activeId.has_value() && *next.activeId == line.node->fileIndex;
                    if (prevActive != nextActive)
                    {
                        WriteLine(line.row, MakeNodeLine(*line.node, line.prefix, line.isLast, maxIndexDigits, next.selectedSet, next.activeId));
                    }
                }
            }
        }
    }

    std::vector<std::size_t> CoreSelector::Run(const std::vector<cgt::scan::DiscoveredFile>& files,
                                               const fs::path& rootDir)
    {
        EnableVirtualTerminal();

        if (files.empty())
        {
            return {};
        }

        CursorGuard cursorGuard;
        std::wcout << HideCursor();

        const TreeNode root = BuildTree(files, rootDir);
        const std::size_t maxIndexDigits = std::to_wstring(std::max<std::size_t>(1, CollectFileIndices(root).size())).size();

        std::vector<RenderLine> lines;
        std::size_t row = 4;
        for (std::size_t i = 0; i < root.children.size(); ++i)
        {
            CollectRenderLines(root.children[i], L"", i + 1 == root.children.size(), row, lines);
        }
        const std::size_t promptRow = lines.empty() ? 5 : lines.back().row + 2;

        std::wstring buffer;
        std::size_t cursor = 0;
        ParseState state = ParseBuffer(buffer, cursor, files.size());
        RenderScreen(root, lines, state, buffer, cursor, maxIndexDigits, promptRow);

        for (;;)
        {
            KeyEvent key;
            if (!ReadKey(key))
            {
                continue;
            }

            const ParseState prevState = state;
            const std::wstring prevBuffer = buffer;
            const std::size_t prevCursor = cursor;

            switch (key.type)
            {
                case KeyEvent::Type::Escape:
                    ClearScreen();
                    return {};

                case KeyEvent::Type::Enter:
                {
                    const ParseState finalState = ParseBuffer(buffer, cursor, files.size());
                    ClearScreen();
                    return finalState.selectedIds;
                }

                case KeyEvent::Type::Left:
                    if (cursor > 0)
                    {
                        --cursor;
                    }
                    break;

                case KeyEvent::Type::Right:
                    if (cursor < buffer.size())
                    {
                        ++cursor;
                    }
                    break;

                case KeyEvent::Type::Home:
                    cursor = 0;
                    break;

                case KeyEvent::Type::End:
                    cursor = buffer.size();
                    break;

                case KeyEvent::Type::Backspace:
                    EraseBeforeCursor(buffer, cursor);
                    break;

                case KeyEvent::Type::DeleteKey:
                    EraseAtCursor(buffer, cursor);
                    break;

                case KeyEvent::Type::Up:
                case KeyEvent::Type::Down:
                {
                    std::size_t target = 1;

                    if (state.activeId.has_value())
                    {
                        target = *state.activeId;
                    }
                    else
                    {
                        const std::wstring token = LastValidTokenText(buffer, files.size());
                        if (!token.empty())
                        {
                            try
                            {
                                target = static_cast<std::size_t>(std::stoull(token));
                                if (target == 0 || target > files.size())
                                {
                                    target = 1;
                                }
                            }
                            catch (...)
                            {
                                target = 1;
                            }
                        }
                    }

                    if (key.type == KeyEvent::Type::Up)
                    {
                        target = target > 1 ? target - 1 : 1;
                    }
                    else
                    {
                        target = target < files.size() ? target + 1 : files.size();
                    }

                    if (state.activeId.has_value())
                    {
                        ReplaceActiveToken(buffer, cursor, files.size(), target);
                    }
                    else
                    {
                        const auto tokens = ParseTokens(buffer, files.size());
                        bool replaced = false;
                        for (auto it = tokens.rbegin(); it != tokens.rend(); ++it)
                        {
                            if (it->valid)
                            {
                                buffer.replace(it->start, it->end - it->start, std::to_wstring(target));
                                cursor = it->start + std::to_wstring(target).size();
                                replaced = true;
                                break;
                            }
                        }

                        if (!replaced)
                        {
                            InsertText(buffer, cursor, std::to_wstring(target));
                        }
                    }
                    break;
                }

                case KeyEvent::Type::Character:
                    if (key.ch >= L'0' && key.ch <= L'9')
                    {
                        InsertText(buffer, cursor, std::wstring(1, key.ch));
                    }
                    else if (key.ch == L' ')
                    {
                        InsertText(buffer, cursor, L" ");
                    }
                    break;

                default:
                    break;
            }

            state = ParseBuffer(buffer, cursor, files.size());

            if (buffer != prevBuffer || cursor != prevCursor || prevState.selectedSet != state.selectedSet || prevState.activeId != state.activeId)
            {
                UpdateChangedLines(root, lines, prevState, state, maxIndexDigits);
                RenderPrompt(promptRow, buffer, cursor);
            }
        }
    }
}
