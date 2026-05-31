#include <windows.h>
#include "app/AppRunner.h"
#include "util/UnicodeConsole.h"

int main()
{
    cgt::util::PrepareUnicodeConsole();

    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if (!argv)
    {
        return -1;
    }

    cgt::app::AppRunner runner;
    int result = runner.Run(argc, argv);

    LocalFree(argv);

    return result;
}



























































// #include <iostream>
// #include <string>
// #include <vector>
// #include <chrono>
// #include <thread>

// #include "ui/Panel.h"
// #include "ui/Terminal.h"
// #include "util/Types.h"

// // Helper function to recalculate and apply absolute center geometry (80% scale)
// void RecalculatePanelGeometry(cgt::ui::panel::Panel& panel, const cgt::ui::teminal::WindowSize& termSize)
// {
//     cgt::ui::panel::PanelSize pSize;
//     pSize.absolute = true;
//     pSize.w = static_cast<int>(termSize.w * 0.8f);
//     pSize.h = static_cast<int>(termSize.h * 0.8f);

//     // Enforce odd-looking viewport dimensions if necessary, safely bound at >= 1
//     if (pSize.w < 1) pSize.w = 1;
//     if (pSize.h < 1) pSize.h = 1;

//     cgt::ui::panel::PanelPos pPos;
//     pPos.absolute = true;
//     pPos.x = (termSize.w - pSize.w) / 2;
//     pPos.y = (termSize.h - pSize.h) / 2;

//     panel.SetSize(pSize);
//     panel.SetPos(pPos);
// }

// int main()
// {
//     // Initialize the terminal module
//     if (cgt::ui::teminal::Init() != 0)
//     {
//         std::wcerr << L"Failed to initialize Terminal.\n";
//         return 1;
//     }

//     // Hide terminal cursor for a cleaner UI experience
//     cgt::ui::teminal::ShowCursor(false);

//     // Initialize the Panel module
//     cgt::ui::panel::Panel testPanel;
//     if (testPanel.Init() != 0)
//     {
//         std::wcerr << L"Failed to initialize Panel.\n";
//         cgt::ui::teminal::Destroy();
//         return 1;
//     }

//     // Fetch initial terminal layout and apply panel coordinates
//     cgt::ui::teminal::WindowSize currentTermSize{0, 0};
//     cgt::ui::teminal::GetSize(currentTermSize);
//     RecalculatePanelGeometry(testPanel, currentTermSize);
//     testPanel.OnResize(currentTermSize);

//     // Generate test mock data: 0 to 100 lines (101 elements) using SetLine
//     for (int i = 1; i <= 10; ++i)
//     {
//         std::wstring lineText = L"This is test line " + std::to_wstring(i);
//         cgt::ui::panel::PanelLine lineData;
//         lineData.text = lineText;
//         lineData.color = cgt::RGB{255, 255, 255};   // White text
//         lineData.bgColor = cgt::RGB{20, 20, 40};    // Dark blue background for contrast
//         cgt::ui::teminal::GetDefaultBgColor(lineData.bgColor);  
        
//         testPanel.SetLine(i, lineData);
//     }

//     // Initial explicit flush execution upon setup completion
//     cgt::ui::teminal::Flush();

//     // Event Loop Flags
//     bool running = true;
//     std::vector<cgt::ui::teminal::CmdKey> cmdKeys;
//     cgt::ui::teminal::CursorPos mousePos;
//     int scrollDelta = 0;
//     int removeCount = 1;

//     while (running)
//     {
//         // Pull low-level pipeline events
//         cgt::ui::teminal::Update();

//         bool requiresFlush = false;

//         // 1. Handle Window Resize Events
//         cgt::ui::teminal::WindowSize newTermSize;
//         if (cgt::ui::teminal::IsResize(newTermSize))
//         {
//             currentTermSize = newTermSize;

//             cgt::ui::teminal::Clean();
            
//             // Re-center and calculate absolute boundaries at 80% volume
//             RecalculatePanelGeometry(testPanel, currentTermSize);
//             testPanel.OnResize(currentTermSize);

//             testPanel.RePrint();
            
//             requiresFlush = true;
//         }

//         // 2. Handle Mouse Scroll Interactions
//         if (cgt::ui::teminal::IsMouseScroll(mousePos, scrollDelta))
//         {
//             testPanel.OnMouseScroll(mousePos, scrollDelta);
//             requiresFlush = true;
//         }

//         // 3. Handle Keyboard Action Mappings
//         cmdKeys.clear();
//         if (cgt::ui::teminal::IsCmdKeyDown(cmdKeys))
//         {
//             for (const auto& key : cmdKeys)
//             {
//                 if (key == cgt::ui::teminal::CmdKey::Escape)
//                 {
//                     running = false;
//                     break;
//                 }
//                 else if (key == cgt::ui::teminal::CmdKey::Up)
//                 {
//                     testPanel.MoveUp(1);
//                     requiresFlush = true;
//                 }
//                 else if (key == cgt::ui::teminal::CmdKey::Down)
//                 {
//                     testPanel.MoveDown(1);
//                     requiresFlush = true;
//                 }
//                 else if (key == cgt::ui::teminal::CmdKey::Tab)
//                 {
//                     testPanel.MoveDown(1);
//                     testPanel.RemoveLine(removeCount); 
//                     removeCount++;
//                     requiresFlush = true;
//                 }
//             }
//         }

//         // Conditional Rendering: Only perform Flush when state updates happened
//         if (requiresFlush && running)
//         {
//             cgt::ui::teminal::Flush();
//         }

//         // Prevent CPU thrashing/100% core usage
//         std::this_thread::sleep_for(std::chrono::milliseconds(16));
//     }

//     // Standard structural cleanup routine on exit
//     testPanel.Destroy();
//     cgt::ui::teminal::ShowCursor(true);
//     cgt::ui::teminal::Destroy();

//     return 0;
// }
