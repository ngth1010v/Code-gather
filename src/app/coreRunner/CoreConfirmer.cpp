#include "app/coreRunner/CoreConfirmer.h"

#include "app/CoreConstant.h"
#include "app/coreRunner/CoreBuildTree.h"
#include "app/coreRunner/CoreUtils.h"
#include "util/StringUtil.h"

#include <algorithm>
#include <cwctype>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace cgt::app::coreRunner
{
    namespace
    {
        enum class Stage
        {
            Action,
            EditOutput,
            EditReplace,
            EditWrapped
        };

        struct ConfirmState
        {
            Stage stage = Stage::Action;
            std::wstring promptBuffer = L"1";
            std::size_t promptCursor = 1;
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

        std::wstring BoolText(bool value)
        {
            return value ? L"Yes" : L"No";
        }

        bool IsYesWord(const std::wstring& text)
        {
            const std::wstring lower = cgt::util::ToLower(cgt::util::Trim(text));
            return lower == L"y" || lower == L"yes" || lower == L"1";
        }

        bool IsNoWord(const std::wstring& text)
        {
            const std::wstring lower = cgt::util::ToLower(cgt::util::Trim(text));
            return lower == L"n" || lower == L"no" || lower == L"0";
        }

        bool IsCancelWord(const std::wstring& text)
        {
            const std::wstring lower = cgt::util::ToLower(cgt::util::Trim(text));
            return lower == L"3" || lower == L"c" || lower == L"cancel";
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

        std::wstring MakePlainRootLine(const TreeNode& root, std::size_t maxIndexDigits)
        {
            std::wstring line;
            line.append(maxIndexDigits, L' ');
            line += L" ";
            line += root.name;
            line += L"/";
            return line;
        }

        std::wstring MakePlainNodeLine(const TreeNode& node,
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
                const cgt::RGB extColor = cgt::config::GetExtColor(node.absolutePath.extension().wstring());
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
            return line;
        }

        void WriteLine(std::size_t row, const std::wstring& text)
        {
            std::wcout << MoveCursorTo(row, 1) << ClearCurrentLine() << text;
            std::wcout.flush();
        }

        std::wstring RenderPrompt(const ConfirmState& state)
        {
            switch (state.stage)
            {
                case Stage::Action:
                    return L"Choose action (1/2/3): " + InsertCursorMarker(state.promptBuffer, state.promptCursor);
                case Stage::EditOutput:
                    return L"Output : " + InsertCursorMarker(state.promptBuffer, state.promptCursor);
                case Stage::EditReplace:
                    return L"Replace (y/N): " + InsertCursorMarker(state.promptBuffer, state.promptCursor);
                case Stage::EditWrapped:
                    return L"Wrapped (Y/n): " + InsertCursorMarker(state.promptBuffer, state.promptCursor);
                default:
                    return {};
            }
        }

        std::wstring MakeActionMenu()
        {
            return L"(1) Start merge\n(2) Edit config\n(3) Cancel";
        }

        void PreparePrompt(ConfirmState& state, Stage stage, const std::wstring& outputToken, bool replace, bool wrapped)
        {
            state.stage = stage;
            state.promptBuffer.clear();
            state.promptCursor = 0;

            switch (stage)
            {
                case Stage::Action:
                    state.promptBuffer = L"1";
                    state.promptCursor = state.promptBuffer.size();
                    break;
                case Stage::EditOutput:
                    state.promptBuffer = outputToken;
                    state.promptCursor = state.promptBuffer.size();
                    break;
                case Stage::EditReplace:
                    state.promptBuffer = replace ? L"y" : L"n";
                    state.promptCursor = state.promptBuffer.size();
                    break;
                case Stage::EditWrapped:
                    state.promptBuffer = wrapped ? L"y" : L"n";
                    state.promptCursor = state.promptBuffer.size();
                    break;
            }
        }

        void RenderTreeNode(const TreeNode& node,
                            const std::wstring& prefix,
                            bool isLast,
                            std::size_t maxIndexDigits)
        {
            std::wcout << MakePlainNodeLine(node, prefix, isLast, maxIndexDigits) << L"\n";

            const std::wstring nextPrefix = prefix + (isLast ? L"    " : L"│   ");
            for (std::size_t i = 0; i < node.children.size(); ++i)
            {
                RenderTreeNode(node.children[i], nextPrefix, i + 1 == node.children.size(), maxIndexDigits);
            }
        }

        void RenderScreen(const TreeNode& selectedTree,
                          const CoreConfirmResult& current,
                          const ConfirmState& state)
        {
            ClearScreen();

            const std::size_t maxIndexDigits = std::to_wstring(std::max<std::size_t>(1, CollectFileIndices(selectedTree).size())).size();

            std::wcout << MakeDividerLine() << L"\n";
            std::wcout << L"Selected files\n";
            std::wcout << MakePlainRootLine(selectedTree, maxIndexDigits) << L"\n";
            for (std::size_t i = 0; i < selectedTree.children.size(); ++i)
            {
                RenderTreeNode(selectedTree.children[i], L"", i + 1 == selectedTree.children.size(), maxIndexDigits);
            }
            std::wcout << MakeDividerLine() << L"\n";
            std::wcout << L"Output : " << current.outputToken << L"\n";
            std::wcout << L"Replace: " << BoolText(current.replace) << L"\n";
            std::wcout << L"Wrapped: " << BoolText(current.wrapped) << L"\n";
            std::wcout << MakeDividerLine() << L"\n";
            std::wcout << MakeActionMenu() << L"\n";
            std::wcout << RenderPrompt(state) << L"\n";
            std::wcout.flush();
        }
    }

    CoreConfirmResult CoreConfirmer::Run(const std::vector<cgt::scan::DiscoveredFile>& files,
                                         const std::vector<std::size_t>& selectedIds,
                                         const fs::path& rootDir,
                                         std::wstring outputToken,
                                         bool replaceDefault,
                                         bool wrappedDefault)
    {
        EnableVirtualTerminal();

        CursorGuard cursorGuard;
        std::wcout << HideCursor();

        CoreConfirmResult result;
        result.outputToken = std::move(outputToken);
        result.replace = replaceDefault;
        result.wrapped = wrappedDefault;

        const TreeNode selectedTree = BuildSelectedTree(files, selectedIds, rootDir);

        ConfirmState state;
        PreparePrompt(state, Stage::Action, result.outputToken, result.replace, result.wrapped);

        for (;;)
        {
            RenderScreen(selectedTree, result, state);

            KeyEvent key;
            if (!ReadKey(key))
            {
                continue;
            }

            if (key.type == KeyEvent::Type::Escape)
            {
                ClearScreen();
                result.cancelled = true;
                return result;
            }

            switch (state.stage)
            {
                case Stage::Action:
                    if (key.type == KeyEvent::Type::Enter)
                    {
                        const std::wstring text = cgt::util::Trim(state.promptBuffer);
                        if (text.empty() || IsYesWord(text))
                        {
                            ClearScreen();
                            result.cancelled = false;
                            return result;
                        }
                        if (text == L"2" || IsNoWord(text))
                        {
                            PreparePrompt(state, Stage::EditOutput, result.outputToken, result.replace, result.wrapped);
                            break;
                        }
                        if (IsCancelWord(text))
                        {
                            ClearScreen();
                            result.cancelled = true;
                            return result;
                        }
                    }
                    else if (key.type == KeyEvent::Type::Character)
                    {
                        if (key.ch == L'1' || key.ch == L'2' || key.ch == L'3' || key.ch == L'y' || key.ch == L'Y' || key.ch == L'n' || key.ch == L'N' || key.ch == L'c' || key.ch == L'C')
                        {
                            state.promptBuffer.assign(1, static_cast<wchar_t>(std::towlower(key.ch)));
                            state.promptCursor = state.promptBuffer.size();
                        }
                    }
                    else if (key.type == KeyEvent::Type::Backspace)
                    {
                        EraseBeforeCursor(state.promptBuffer, state.promptCursor);
                    }
                    else if (key.type == KeyEvent::Type::DeleteKey)
                    {
                        EraseAtCursor(state.promptBuffer, state.promptCursor);
                    }
                    else if (key.type == KeyEvent::Type::Left)
                    {
                        if (state.promptCursor > 0)
                        {
                            --state.promptCursor;
                        }
                    }
                    else if (key.type == KeyEvent::Type::Right)
                    {
                        if (state.promptCursor < state.promptBuffer.size())
                        {
                            ++state.promptCursor;
                        }
                    }
                    else if (key.type == KeyEvent::Type::Home)
                    {
                        state.promptCursor = 0;
                    }
                    else if (key.type == KeyEvent::Type::End)
                    {
                        state.promptCursor = state.promptBuffer.size();
                    }
                    break;

                case Stage::EditOutput:
                    if (key.type == KeyEvent::Type::Enter)
                    {
                        const std::wstring trimmed = cgt::util::Trim(state.promptBuffer);
                        if (!trimmed.empty())
                        {
                            result.outputToken = trimmed;
                        }
                        PreparePrompt(state, Stage::EditReplace, result.outputToken, result.replace, result.wrapped);
                        break;
                    }
                    if (key.type == KeyEvent::Type::Character)
                    {
                        if (key.ch >= 32)
                        {
                            InsertText(state.promptBuffer, state.promptCursor, std::wstring(1, key.ch));
                        }
                    }
                    else if (key.type == KeyEvent::Type::Backspace)
                    {
                        EraseBeforeCursor(state.promptBuffer, state.promptCursor);
                    }
                    else if (key.type == KeyEvent::Type::DeleteKey)
                    {
                        EraseAtCursor(state.promptBuffer, state.promptCursor);
                    }
                    else if (key.type == KeyEvent::Type::Left)
                    {
                        if (state.promptCursor > 0)
                        {
                            --state.promptCursor;
                        }
                    }
                    else if (key.type == KeyEvent::Type::Right)
                    {
                        if (state.promptCursor < state.promptBuffer.size())
                        {
                            ++state.promptCursor;
                        }
                    }
                    else if (key.type == KeyEvent::Type::Home)
                    {
                        state.promptCursor = 0;
                    }
                    else if (key.type == KeyEvent::Type::End)
                    {
                        state.promptCursor = state.promptBuffer.size();
                    }
                    break;

                case Stage::EditReplace:
                    if (key.type == KeyEvent::Type::Enter)
                    {
                        const std::wstring text = cgt::util::Trim(state.promptBuffer);
                        if (IsYesWord(text))
                        {
                            result.replace = true;
                        }
                        else if (IsNoWord(text))
                        {
                            result.replace = false;
                        }
                        PreparePrompt(state, Stage::EditWrapped, result.outputToken, result.replace, result.wrapped);
                        break;
                    }
                    if (key.type == KeyEvent::Type::Character)
                    {
                        if (key.ch == L'y' || key.ch == L'Y' || key.ch == L'n' || key.ch == L'N')
                        {
                            state.promptBuffer.assign(1, static_cast<wchar_t>(std::towlower(key.ch)));
                            state.promptCursor = state.promptBuffer.size();
                        }
                    }
                    break;

                case Stage::EditWrapped:
                    if (key.type == KeyEvent::Type::Enter)
                    {
                        const std::wstring text = cgt::util::Trim(state.promptBuffer);
                        if (IsYesWord(text))
                        {
                            result.wrapped = true;
                        }
                        else if (IsNoWord(text))
                        {
                            result.wrapped = false;
                        }
                        PreparePrompt(state, Stage::Action, result.outputToken, result.replace, result.wrapped);
                        break;
                    }
                    if (key.type == KeyEvent::Type::Character)
                    {
                        if (key.ch == L'y' || key.ch == L'Y' || key.ch == L'n' || key.ch == L'N')
                        {
                            state.promptBuffer.assign(1, static_cast<wchar_t>(std::towlower(key.ch)));
                            state.promptCursor = state.promptBuffer.size();
                        }
                    }
                    break;
            }

            if (state.stage == Stage::Action)
            {
                state.promptCursor = std::min(state.promptCursor, state.promptBuffer.size());
            }
        }
    }
}
