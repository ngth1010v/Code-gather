#include "app/coreRunner/CoreSelectionUi.h"

#include "app/coreRunner/CoreSelectionInput.h"
#include "app/coreRunner/CoreTreeRender.h"
#include "scan/DiscoveryScanner.h"
#include "ui/Panel.h"
#include "ui/Terminal.h"
#include "util/PathUtil.h"

#include <algorithm>
#include <chrono>
#include <cwchar>
#include <iostream>
#include <set>
#include <string>
#include <thread>
#include <windows.h>

namespace cgt::app::coreRunner
{
    namespace
    {
        using WindowSize = cgt::ui::teminal::WindowSize;
        using CursorPos = cgt::ui::teminal::CursorPos;
        using CmdKey = cgt::ui::teminal::CmdKey;
        using MouseKey = cgt::ui::teminal::MouseKey;

        constexpr int kTargetFps = 30;
        constexpr int kMinLeftWidth = 18;
        constexpr int kMinRightWidth = 18;
        constexpr int kMinPromptHeight = 2;
        constexpr int kPromptHeaderHeight = 1;

        std::wstring CursorMarker(const std::wstring& text, std::size_t cursor)
        {
            const std::size_t pos = std::min(cursor, text.size());
            std::wstring out;
            out.reserve(text.size() + 1);
            out.append(text.substr(0, pos));
            out.push_back(L'|');
            out.append(text.substr(pos));
            return out;
        }

        void EnableVirtualTerminal()
        {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut == INVALID_HANDLE_VALUE)
            {
                return;
            }

            DWORD mode = 0;
            if (!GetConsoleMode(hOut, &mode))
            {
                return;
            }

            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, mode);
        }

        std::wstring MoveCursorTo(std::size_t row, std::size_t col = 1)
        {
            std::wstring out;
            out.reserve(24);
            out += L"\x1b[";
            out += std::to_wstring(row);
            out += L';';
            out += std::to_wstring(col);
            out += L'H';
            return out;
        }

        std::wstring ClearLine()
        {
            return L"\x1b[2K";
        }

        std::size_t WrapRows(std::size_t textLength, std::size_t width)
        {
            width = std::max<std::size_t>(1, width);
            return std::max<std::size_t>(1, (textLength + width - 1) / width);
        }

        struct Layout
        {
            int termW = 80;
            int termH = 25;
            int leftW = 48;
            int rightW = 31;
            int rightX = 49;
            int selectedTopH = 23;
            int promptH = 2;
            int promptY = 24;
        };

        Layout ComputeLayout(const WindowSize& size, const std::wstring& promptText)
        {
            Layout layout;
            layout.termW = std::max(1, size.w);
            layout.termH = std::max(1, size.h);

            int left = static_cast<int>(layout.termW * 60 / 100);
            left = std::max(kMinLeftWidth, left);
            if (left >= layout.termW - kMinRightWidth)
            {
                left = std::max(1, layout.termW - kMinRightWidth);
            }

            layout.leftW = std::max(1, left);
            layout.rightW = std::max(1, layout.termW - layout.leftW);
            if (layout.rightW < kMinRightWidth && layout.termW > kMinRightWidth)
            {
                layout.rightW = kMinRightWidth;
                layout.leftW = std::max(1, layout.termW - layout.rightW);
            }

            layout.rightX = layout.leftW + 1;

            const std::size_t promptWidth = static_cast<std::size_t>(std::max(1, layout.rightW));
            const std::size_t promptRows = WrapRows(promptText.size(), promptWidth);
            layout.promptH = static_cast<int>(std::max<std::size_t>(kMinPromptHeight, promptRows + kPromptHeaderHeight));
            layout.promptH = std::min(layout.promptH, layout.termH);
            if (layout.promptH > layout.termH - 1)
            {
                layout.promptH = std::max(1, layout.termH - 1);
            }

            layout.selectedTopH = std::max(1, layout.termH - layout.promptH);
            layout.promptY = layout.selectedTopH + 1;
            return layout;
        }

        void ApplyPanelGeometry(cgt::ui::panel::Panel& panel,
                                int x,
                                int y,
                                int w,
                                int h)
        {
            cgt::ui::panel::PanelPos pos;
            pos.absolute = true;
            pos.x = x;
            pos.y = y;
            panel.SetPos(pos);

            cgt::ui::panel::PanelSize size;
            size.absolute = true;
            size.w = w;
            size.h = h;
            panel.SetSize(size);
        }

        void RenderPromptArea(const Layout& layout,
                              const std::wstring& buffer,
                              std::size_t cursor,
                              int previousPromptH)
        {
            const std::wstring promptHeader = L"Selected files:";
            const std::wstring promptBody = CursorMarker(buffer, cursor);
            const std::size_t width = static_cast<std::size_t>(std::max(1, layout.rightW));
            const std::size_t bodyRows = WrapRows(promptBody.size(), width);
            const int promptH = std::max(2, static_cast<int>(bodyRows + 1));

            const int rowsToClear = std::max(previousPromptH, promptH);
            for (int i = 0; i < rowsToClear; ++i)
            {
                const int row = layout.promptY + i;
                std::wcout << MoveCursorTo(static_cast<std::size_t>(row), 1) << ClearLine();
            }

            std::wcout << MoveCursorTo(static_cast<std::size_t>(layout.promptY), 1)
                       << promptHeader;

            std::size_t offset = 0;
            for (std::size_t rowIndex = 0; rowIndex < bodyRows; ++rowIndex)
            {
                const std::size_t begin = offset;
                const std::size_t end = std::min(promptBody.size(), begin + width);
                const std::wstring chunk = promptBody.substr(begin, end - begin);
                offset = end;

                std::wcout << MoveCursorTo(static_cast<std::size_t>(layout.promptY + 1 + rowIndex), 1)
                           << chunk;
            }

            std::wcout.flush();
        }

        void RenderPanel(cgt::ui::panel::Panel& panel,
                         const std::vector<cgt::ui::panel::PanelLine>& lines)
        {
            panel.Clean();
            for (std::size_t i = 0; i < lines.size(); ++i)
            {
                panel.SetLine(static_cast<int>(i), lines[i]);
            }
        }

        class SessionGuard
        {
        public:
            bool initialized = false;
            int previousPromptH = 0;
            cgt::ui::panel::Panel detectedPanel;
            cgt::ui::panel::Panel selectedPanel;

            ~SessionGuard()
            {
                if (initialized)
                {
                    cgt::ui::teminal::ShowCursor(true);
                    detectedPanel.Destroy();
                    selectedPanel.Destroy();
                    cgt::ui::teminal::Destroy();
                }
            }
        };
    }

    std::vector<std::size_t> CoreSelectionUi::Run(const std::vector<cgt::scan::DiscoveredFile>& files,
                                                  const std::filesystem::path& rootDir)
    {
        if (files.empty())
        {
            return {};
        }

        EnableVirtualTerminal();

        SessionGuard session;
        if (cgt::ui::teminal::Init() != 0)
        {
            return {};
        }
        session.initialized = true;

        if (session.detectedPanel.Init() != 0 || session.selectedPanel.Init() != 0)
        {
            return {};
        }

        cgt::ui::teminal::ShowCursor(false);

        session.detectedPanel.SetScroll(true);
        session.selectedPanel.SetScroll(true);

        SelectionEditor editor;
        TreeNode detectedRoot = BuildTree(files, rootDir);
        WindowSize termSize{};
        if (cgt::ui::teminal::GetSize(termSize) != 0)
        {
            termSize.w = 80;
            termSize.h = 25;
        }

        auto renderAll = [&](const SelectionParseState& state)
        {
            const TreeNode selectedRoot = BuildSelectedTree(files, state.selectedIds, rootDir);
            const std::wstring promptBody = CursorMarker(editor.Buffer(), editor.Cursor());
            const Layout layout = ComputeLayout(termSize, promptBody);

            ApplyPanelGeometry(session.detectedPanel, 1, 1, layout.leftW, layout.termH);
            ApplyPanelGeometry(session.selectedPanel, layout.rightX, 1, layout.rightW, layout.selectedTopH);

            const auto detectedLines = BuildTreePanelLines(detectedRoot, L"Detected files", state.selectedSet, state.activeId);
            const auto selectedLines = BuildTreePanelLines(selectedRoot, L"Selected files", state.selectedSet, state.activeId);

            RenderPanel(session.detectedPanel, detectedLines);
            RenderPanel(session.selectedPanel, selectedLines);

            RenderPromptArea(layout, editor.Buffer(), editor.Cursor(), session.previousPromptH);
            session.previousPromptH = layout.promptH;
            return layout;
        };

        SelectionParseState state = editor.Parse(files.size());
        Layout layout = renderAll(state);
        cgt::ui::teminal::Flush();

        bool running = true;
        while (running)
        {
            auto frameStart = std::chrono::steady_clock::now();

            cgt::ui::teminal::Update();

            WindowSize newSize{};
            if (cgt::ui::teminal::IsResize(newSize))
            {
                termSize = newSize;
                layout = renderAll(editor.Parse(files.size()));
                cgt::ui::teminal::Flush();
            }

            CursorPos mousePos{};
            int wheelDelta = 0;
            if (cgt::ui::teminal::IsMouseScroll(mousePos, wheelDelta))
            {
                session.detectedPanel.OnMouseScroll(mousePos, wheelDelta);
                session.selectedPanel.OnMouseScroll(mousePos, wheelDelta);
            }

            bool changed = false;
            bool finish = false;

            std::vector<wchar_t> keys;
            if (cgt::ui::teminal::IsKeyDown(keys))
            {
                for (wchar_t ch : keys)
                {
                    if (ch >= L'0' && ch <= L'9')
                    {
                        editor.InsertChar(ch);
                        changed = true;
                    }
                    else if (ch == L' ')
                    {
                        editor.InsertChar(ch);
                        changed = true;
                    }
                }
            }

            std::vector<CmdKey> cmdKeys;
            if (cgt::ui::teminal::IsCmdKeyDown(cmdKeys))
            {
                for (CmdKey key : cmdKeys)
                {
                    switch (key)
                    {
                        case CmdKey::Escape:
                            return {};
                        case CmdKey::Enter:
                            finish = true;
                            break;
                        case CmdKey::Backspace:
                            editor.Backspace();
                            changed = true;
                            break;
                        case CmdKey::Delete:
                            editor.Delete();
                            changed = true;
                            break;
                        case CmdKey::Left:
                            editor.MoveLeft();
                            changed = true;
                            break;
                        case CmdKey::Right:
                            editor.MoveRight();
                            changed = true;
                            break;
                        case CmdKey::Home:
                            editor.MoveHome();
                            changed = true;
                            break;
                        case CmdKey::End:
                            editor.MoveEnd();
                            changed = true;
                            break;
                        case CmdKey::Up:
                            editor.Step(-1, files.size());
                            changed = true;
                            break;
                        case CmdKey::Down:
                            editor.Step(+1, files.size());
                            changed = true;
                            break;
                        default:
                            break;
                    }

                    if (finish)
                    {
                        break;
                    }
                }
            }

            if (finish)
            {
                return editor.Parse(files.size()).selectedIds;
            }

            if (changed)
            {
                state = editor.Parse(files.size());
                layout = renderAll(state);
                cgt::ui::teminal::Flush();
            }

            const auto frameEnd = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
            const auto frameMs = std::chrono::milliseconds(1000 / kTargetFps);
            if (elapsed < frameMs)
            {
                std::this_thread::sleep_for(frameMs - elapsed);
            }
        }

        return {};
    }
}
