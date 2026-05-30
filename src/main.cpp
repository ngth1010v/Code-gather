// #include <windows.h>
// #include "app/AppRunner.h"
// #include "util/UnicodeConsole.h"

// int main()
// {
//     cgt::util::PrepareUnicodeConsole();

//     int argc = 0;
//     wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

//     if (!argv)
//     {
//         return -1;
//     }

//     cgt::app::AppRunner runner;
//     int result = runner.Run(argc, argv);

//     LocalFree(argv);

//     return result;
// }

































// #include <iostream>
// #include <string>
// #include <vector>
// #include <chrono>
// #include <thread>

// #include "ui/Terminal.h"
// #include "util/Types.h"

// // Use the slightly misspelled namespace found throughout your source files
// using namespace cgt::ui::teminal;

// int main()
// {
//     // 1. Initialize the terminal into raw/alternate mode
//     if (Init() != 0)
//     {
//         std::cerr << "Failed to initialize terminal module.\n";
//         return 1;
//     }

//     // Set an initial title and hide the cursor for a clean layout
//     SetTitle(L"cgt::ui::terminal - Test Environment");
//     ShowCursor(false);

//     // Initial setup colors
//     cgt::RGB greenColor{0, 255, 0};
//     cgt::RGB whiteColor{255, 255, 255};
//     cgt::RGB blueBg{0, 0, 100};
//     cgt::RGB redColor{255, 50, 50};

//     // Grab initial window layout
//     WindowSize size{0, 0};
//     GetSize(size);

//     std::wstring statusMessage = L"Press [Escape] to exit. Click or type anywhere!";

//     MoveTo({0, 0});
//     SetFontColor(whiteColor);
//     SetBackgroundColor(blueBg);
//     std::wstring headerLine(size.w > 0 ? size.w : 40, L' ');
//     Print(headerLine);
        
//     MoveTo({2, 0});
//     Print(L"TERMINAL TEST BENCH");

//     bool running = true;
//     while (running)
//     {
//         // 2. Poll platform-specific inputs and fill the state queues
//         Update();

//         // Check for Window Resize
//         if (IsResize(size))
//         {
//             // Clear the terminal on size changes to prevent trailing artifact blocks
//             // Note: Since a direct Clear wrapper wasn't in Terminal.h public API, 
//             // you can print spaces or rely on your internal alternate buffer clean rules.
//             statusMessage = L"Window resized to: " + std::to_wstring(size.w) + L"x" + std::to_wstring(size.h);
//         }

//         // 3. Handle Keyboard Characters
//         std::vector<wchar> characters;
//         if (IsKeyDown(characters))
//         {
//             statusMessage = L"Character Pressed: ";
//             for (auto ch : characters)
//             {
//                 statusMessage += ch;
//             }
//         }

//         // 4. Handle Command Keys (Escape, Arrows, Enter, etc.)
//         std::vector<CmdKey> commands;
//         if (IsCmdKeyDown(commands))
//         {
//             for (auto cmd : commands)
//             {
//                 if (cmd == CmdKey::Escape)
//                 {
//                     running = false;
//                 }
//                 else if (cmd == CmdKey::Enter)
//                 {
//                     statusMessage = L"Command Triggered: [Enter]";
//                 }
//                 else if (cmd == CmdKey::Up)
//                 {
//                     statusMessage = L"Command Triggered: [Arrow Up]";
//                 }
//                 else if (cmd == CmdKey::Down)
//                 {
//                     statusMessage = L"Command Triggered: [Arrow Down]";
//                 }
//                 else
//                 {
//                     statusMessage = L"Command Key Enum ID: " + std::to_wstring(static_cast<int>(cmd));
//                 }
//             }
//         }

//         // 5. Handle Mouse Clicks
//         CursorPos mousePos{0, 0};
//         std::vector<MouseKey> mouseButtons;
//         if (IsMouseDown(mousePos, mouseButtons))
//         {
//             statusMessage = L"Mouse Down at (" + std::to_wstring(mousePos.x) + L", " + std::to_wstring(mousePos.y) + L") Buttons: ";
//             for (auto btn : mouseButtons)
//             {
//                 if (btn == MouseKey::Left)   statusMessage += L"[Left] ";
//                 if (btn == MouseKey::Right)  statusMessage += L"[Right] ";
//                 if (btn == MouseKey::Middle) statusMessage += L"[Middle] ";
//             }
//         }

//         // 6. Handle Mouse Scroll Ticks
//         CursorPos scrollPos{0, 0};
//         int scrollDelta = 0;
//         if (IsMouseScroll(scrollPos, scrollDelta))
//         {
//             statusMessage = L"Mouse Scroll at (" + std::to_wstring(scrollPos.x) + L", " + std::to_wstring(scrollPos.y) + L") Delta: " + std::to_wstring(scrollDelta);
//         }

//         // 7. Render UI Frame
//         // Draw Header Border Banner

//         // Draw Live Content area
//         MoveTo({2, 3});
//         SetBackgroundColor({0, 0, 0}); // Transparent/Black background
//         SetFontColor(greenColor);
//         Print(L"Current Dimensions: " + std::to_wstring(size.w) + L" Columns x " + std::to_wstring(size.h) + L" Rows");

//         // Draw Interactive Action Status Row
//         MoveTo({2, 5});
//         SetFontColor(redColor);
//         Print(L"Last Event: ");
//         SetFontColor(whiteColor);
//         Print(statusMessage);

//         // Commit written strings to the screen buffer via raw platform write
//         Flush();

//         // Chill out the CPU loop slightly
//         std::this_thread::sleep_for(std::chrono::milliseconds(16));
//     }

//     // 8. Restore the original terminal settings safely before exit
//     ShowCursor(true);
//     Destroy();

//     return 0;
// }








































#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include "ui/Panel.h"
#include "ui/Terminal.h"
#include "util/Types.h"

// Helper function to recalculate and apply absolute center geometry (80% scale)
void RecalculatePanelGeometry(cgt::ui::panel::Panel& panel, const cgt::ui::teminal::WindowSize& termSize)
{
    cgt::ui::panel::PanelSize pSize;
    pSize.absolute = true;
    pSize.w = static_cast<int>(termSize.w * 0.8f);
    pSize.h = static_cast<int>(termSize.h * 0.8f);

    // Enforce odd-looking viewport dimensions if necessary, safely bound at >= 1
    if (pSize.w < 1) pSize.w = 1;
    if (pSize.h < 1) pSize.h = 1;

    cgt::ui::panel::PanelPos pPos;
    pPos.absolute = true;
    pPos.x = (termSize.w - pSize.w) / 2;
    pPos.y = (termSize.h - pSize.h) / 2;

    panel.SetSize(pSize);
    panel.SetPos(pPos);
}

int main()
{
    // Initialize the terminal module
    if (cgt::ui::teminal::Init() != 0)
    {
        std::wcerr << L"Failed to initialize Terminal.\n";
        return 1;
    }

    // Hide terminal cursor for a cleaner UI experience
    cgt::ui::teminal::ShowCursor(false);

    // Initialize the Panel module
    cgt::ui::panel::Panel testPanel;
    if (testPanel.Init() != 0)
    {
        std::wcerr << L"Failed to initialize Panel.\n";
        cgt::ui::teminal::Destroy();
        return 1;
    }

    // Fetch initial terminal layout and apply panel coordinates
    cgt::ui::teminal::WindowSize currentTermSize{0, 0};
    cgt::ui::teminal::GetSize(currentTermSize);
    RecalculatePanelGeometry(testPanel, currentTermSize);
    testPanel.OnResize(currentTermSize);

    // Generate test mock data: 0 to 100 lines (101 elements) using SetLine
    for (int i = 0; i <= 100; ++i)
    {
        std::wstring lineText = L"This is test line " + std::to_wstring(i);
        cgt::ui::panel::PanelLine lineData;
        lineData.text = lineText;
        lineData.color = cgt::RGB{255, 255, 255};   // White text
        lineData.bgColor = cgt::RGB{20, 20, 40};    // Dark blue background for contrast
        
        testPanel.SetLine(i, lineData);
    }

    // Initial explicit flush execution upon setup completion
    cgt::ui::teminal::Flush();

    // Event Loop Flags
    bool running = true;
    std::vector<cgt::ui::teminal::CmdKey> cmdKeys;
    cgt::ui::teminal::CursorPos mousePos;
    int scrollDelta = 0;

    while (running)
    {
        // Pull low-level pipeline events
        cgt::ui::teminal::Update();

        bool requiresFlush = false;

        // 1. Handle Window Resize Events
        cgt::ui::teminal::WindowSize newTermSize;
        if (cgt::ui::teminal::IsResize(newTermSize))
        {
            currentTermSize = newTermSize;

            cgt::ui::teminal::Clean();
            
            // Re-center and calculate absolute boundaries at 80% volume
            RecalculatePanelGeometry(testPanel, currentTermSize);
            testPanel.OnResize(currentTermSize);

            testPanel.RePrint();
            
            requiresFlush = true;
        }

        // 2. Handle Mouse Scroll Interactions
        if (cgt::ui::teminal::IsMouseScroll(mousePos, scrollDelta))
        {
            testPanel.OnMouseScroll(mousePos, scrollDelta);
            requiresFlush = true;
        }

        // 3. Handle Keyboard Action Mappings
        cmdKeys.clear();
        if (cgt::ui::teminal::IsCmdKeyDown(cmdKeys))
        {
            for (const auto& key : cmdKeys)
            {
                if (key == cgt::ui::teminal::CmdKey::Escape)
                {
                    running = false;
                    break;
                }
                else if (key == cgt::ui::teminal::CmdKey::Up)
                {
                    testPanel.MoveUp(1);
                    requiresFlush = true;
                }
                else if (key == cgt::ui::teminal::CmdKey::Down)
                {
                    testPanel.MoveDown(1);
                    requiresFlush = true;
                }
            }
        }

        // Conditional Rendering: Only perform Flush when state updates happened
        if (requiresFlush && running)
        {
            cgt::ui::teminal::Flush();
        }

        // Prevent CPU thrashing/100% core usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Standard structural cleanup routine on exit
    testPanel.Destroy();
    cgt::ui::teminal::ShowCursor(true);
    cgt::ui::teminal::Destroy();

    return 0;
}
