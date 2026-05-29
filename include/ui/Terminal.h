#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "util/Types.h"

namespace cgt::ui::teminal
{
    using wchar = wchar_t;

    struct WindowSize
    {
        int w{0};
        int h{0};
    };

    struct CursorPos
    {
        int x{0};
        int y{0};
    };

    enum class CmdKey : std::uint8_t
    {
        Unknown = 0,
        Enter,
        Escape,
        Backspace,
        Tab,
        Up,
        Down,
        Left,
        Right,
        Home,
        End,
        PageUp,
        PageDown,
        Insert,
        Delete,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
    };

    int Init();
    int Destroy();
    int Update();
    int Clean();

    int GetSize(WindowSize& size);
    int GetCursorPos(CursorPos& pos);

    int ShowCursor(bool visible);
    int SetTitle(std::wstring_view title);

    int MoveTo(const CursorPos& pos);
    int SetFontColor(const RGB& rgb);
    int SetBackgroundColor(const RGB& rgb);

    int Print(std::wstring_view text);
    int Flush();

    bool IsResize(WindowSize& size);
    bool IsMouseDown(CursorPos& pos);
    bool IsKeyDown(std::vector<wchar>& key);
    bool IsCmdKeyDown(std::vector<CmdKey>& key);
}
