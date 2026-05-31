#include "app/coreRunner/CoreSelectionUi.h"

#include "app/coreRunner/CoreConstants.h"
#include "app/coreRunner/CoreDetectedPanel.h"
#include "app/coreRunner/CoreInput.h"
#include "app/coreRunner/CoreInputPanel.h"
#include "app/coreRunner/CoreSelectedPanel.h"
#include "app/coreRunner/CoreSelectionInput.h"
#include "app/coreRunner/CoreUtils.h"
#include "app/coreRunner/TreeBuilder.h"
#include "ui/Terminal.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

namespace cgt::app::coreRunner
{
    namespace
    {
        using WindowSize = cgt::ui::teminal::WindowSize;
        using CmdKey = cgt::ui::teminal::CmdKey;

        struct Layout
        {
            int termW = 80;
            int termH = 25;

            int detectedX = 0;
            int detectedY = 0;
            int detectedW = 0;
            int detectedH = 0;

            int selectedX = 0;
            int selectedY = 0;
            int selectedW = 0;
            int selectedH = 0;

            int inputX = 0;
            int inputY = 0;
            int inputW = 0;
            int inputH = 0;
        };

        int ClampSplitWidth(int termW)
        {
            if (termW <= 1)
            {
                return 1;
            }

            int left = static_cast<int>(termW * 60 / 100);
            left = std::max(kMinLeftPanelWidth, left);

            if (left >= termW)
            {
                left = termW - 1;
            }

            if (termW - left < kMinRightPanelWidth && termW > kMinRightPanelWidth)
            {
                left = std::max(1, termW - kMinRightPanelWidth);
            }

            return std::max(1, std::min(left, termW - 1));
        }

        Layout ComputeLayout(const WindowSize& size, std::size_t inputNeededHeight)
        {
            Layout layout;

            layout.termW = std::max(1, size.w);
            layout.termH = std::max(1, size.h);

            const int leftW = ClampSplitWidth(layout.termW);
            const int rightW = std::max(1, layout.termW - leftW);

            const int availableH = layout.termH;

            const int minInputH =
                std::min(kMinInputPanelHeight, availableH);

            const int minSelectedH =
                std::min(
                    kMinSelectedPanelHeight,
                    std::max(1, availableH - minInputH));

            int inputH = static_cast<int>(
                std::min<std::size_t>(
                    inputNeededHeight,
                    static_cast<std::size_t>(
                        std::max(1, availableH - minSelectedH))));

            inputH = std::max(minInputH, inputH);

            int selectedH = availableH - inputH;

            if (selectedH < minSelectedH && availableH > 1)
            {
                selectedH = minSelectedH;
                inputH = std::max(1, availableH - selectedH);
            }

            if (selectedH < 1)
            {
                selectedH = 1;
                inputH = std::max(1, availableH - selectedH);
            }

            layout.detectedX = 0;
            layout.detectedY = 0;
            layout.detectedW = leftW;
            layout.detectedH = availableH;

            layout.selectedX = leftW + 1;
            layout.selectedY = 0;
            layout.selectedW = rightW;
            layout.selectedH = selectedH;

            layout.inputX = leftW + 1;
            layout.inputY = selectedH;
            layout.inputW = rightW;
            layout.inputH = inputH;

            return layout;
        }

        struct SessionGuard
        {
            bool terminalInitialized = false;
            bool detectedPanelInitialized = false;
            bool selectedPanelInitialized = false;
            bool inputPanelInitialized = false;

            CoreDetectedPanel detectedPanel;
            CoreSelectedPanel selectedPanel;
            CoreInputPanel inputPanel;

            ~SessionGuard()
            {
                if (inputPanelInitialized)
                {
                    inputPanel.Destroy();
                }
                if (selectedPanelInitialized)
                {
                    selectedPanel.Destroy();
                }
                if (detectedPanelInitialized)
                {
                    detectedPanel.Destroy();
                }
                if (terminalInitialized)
                {
                    cgt::ui::teminal::ShowCursor(true);
                    cgt::ui::teminal::Destroy();
                }
            }
        };

        void SetGeometry(SessionGuard& session, const Layout& layout)
        {
            cgt::ui::panel::PanelPos pos;
            cgt::ui::panel::PanelSize size;

            pos.absolute = true;
            size.absolute = true;

            // detected
            pos.x = layout.detectedX;
            pos.y = layout.detectedY;
            size.w = layout.detectedW;
            size.h = layout.detectedH;

            session.detectedPanel.SetPos(pos);
            session.detectedPanel.SetSize(size);

            // selected
            pos.x = layout.selectedX;
            pos.y = layout.selectedY;
            size.w = layout.selectedW;
            size.h = layout.selectedH;

            session.selectedPanel.SetPos(pos);
            session.selectedPanel.SetSize(size);

            // input
            pos.x = layout.inputX;
            pos.y = layout.inputY;
            size.w = layout.inputW;
            size.h = layout.inputH;

            session.inputPanel.SetPos(pos);
            session.inputPanel.SetSize(size);

            WindowSize termSize{};
            termSize.w = layout.termW;
            termSize.h = layout.termH;

            session.detectedPanel.OnResize(termSize);
            session.selectedPanel.OnResize(termSize);
            session.inputPanel.OnResize(termSize);
        }
    }

    std::vector<std::size_t> CoreSelectionUi::Run(const std::vector<cgt::scan::DiscoveredFile>& files,
                                                  const std::filesystem::path& rootDir)
    {
        if (files.empty())
        {
            return {};
        }

        SessionGuard session;
        if (cgt::ui::teminal::Init() != 0)
        {
            return {};
        }
        session.terminalInitialized = true;

        if (session.detectedPanel.Init() != 0)
        {
            return {};
        }
        session.detectedPanelInitialized = true;

        if (session.selectedPanel.Init() != 0)
        {
            return {};
        }
        session.selectedPanelInitialized = true;

        if (session.inputPanel.Init() != 0)
        {
            return {};
        }
        session.inputPanelInitialized = true;

        session.detectedPanel.SetScroll(true);
        session.selectedPanel.SetScroll(true);
        session.inputPanel.SetScroll(false);

        cgt::ui::teminal::ShowCursor(false);

        SelectionEditor editor;
        const TreeNode detectedRoot = BuildTree(files, rootDir);

        WindowSize termSize{};
        if (cgt::ui::teminal::GetSize(termSize) != 0)
        {
            termSize.w = 80;
            termSize.h = 25;
        }

        cgt::ui::teminal::Clean();

        const std::size_t initialInputHeight = session.inputPanel.RequiredHeight(
            static_cast<std::size_t>(std::max(1, termSize.w - ClampSplitWidth(std::max(1, termSize.w)))),
            editor);

        Layout layout = ComputeLayout(termSize, initialInputHeight);
        SetGeometry(session, layout);

        auto renderAll = [&](bool forceReprint)
        {
            const SelectionParseState state = editor.Parse(files.size());
            const TreeNode selectedRoot = BuildSelectedTree(files, state.selectedIds, rootDir);

            session.detectedPanel.Render(detectedRoot, state, forceReprint);
            session.selectedPanel.Render(selectedRoot, forceReprint);
            session.inputPanel.Render(editor, forceReprint);
        };

        renderAll(true);
        cgt::ui::teminal::Flush();

        while (true)
        {
            const auto frameStart = std::chrono::steady_clock::now();
            cgt::ui::teminal::Update();

            bool shouldFlush = false;

            WindowSize newSize{};
            if (cgt::ui::teminal::IsResize(newSize))
            {
                termSize = newSize;
                cgt::ui::teminal::Clean();

                const std::size_t inputNeededHeight = session.inputPanel.RequiredHeight(
                    static_cast<std::size_t>(std::max(1, termSize.w - ClampSplitWidth(std::max(1, termSize.w)))),
                    editor);

                layout = ComputeLayout(termSize, inputNeededHeight);
                SetGeometry(session, layout);
                renderAll(true);
                shouldFlush = true;
            }

            std::vector<wchar_t> keys;
            std::vector<CmdKey> cmdKeys;
            cgt::ui::teminal::IsKeyDown(keys);
            cgt::ui::teminal::IsCmdKeyDown(cmdKeys);

            bool changed = false;
            bool finish = false;
            bool cancel = false;

            if (!keys.empty() || !cmdKeys.empty())
            {
                const auto result = HandleSelectionInput(editor, keys, cmdKeys, files.size());
                changed = result.changed;
                finish = result.finish;
                cancel = result.cancel;
            }

            cgt::ui::teminal::CursorPos mousePos;
            int scrollDelta = 0;
            if (cgt::ui::teminal::IsMouseScroll(mousePos, scrollDelta))
            {
                session.detectedPanel.OnMouseScroll(mousePos, scrollDelta);
                session.selectedPanel.OnMouseScroll(mousePos, scrollDelta);
                shouldFlush = true;
            }

            if (cancel)
            {
                return {};
            }

            if (finish)
            {
                const auto state = editor.Parse(files.size());
                return state.selectedIds;
            }

            if (changed)
            {
                const std::size_t inputNeededHeight = session.inputPanel.RequiredHeight(
                    static_cast<std::size_t>(layout.selectedW),
                    editor);

                layout = ComputeLayout(termSize, inputNeededHeight);
                SetGeometry(session, layout);
                renderAll(false);
                shouldFlush = true;
            }

            if (shouldFlush)
            {
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
    }
}
