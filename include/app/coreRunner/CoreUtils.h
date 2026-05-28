#pragma once

#include "app/CoreConstant.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <windows.h>

namespace cgt::app::coreRunner
{
    struct KeyEvent
    {
        enum class Type
        {
            Unknown,
            Character,
            Enter,
            Escape,
            Backspace,
            DeleteKey,
            Left,
            Right,
            Up,
            Down,
            Home,
            End
        };

        Type type = Type::Unknown;
        wchar_t ch = 0;
        bool ctrl = false;
        bool alt = false;
        bool shift = false;
    };

    void EnableVirtualTerminal();
    void ClearScreen();
    void MoveCursorHome();

    bool ReadKey(KeyEvent& key);

    std::wstring MakeAnsiFg(const cgt::config::RGB& color);
    std::wstring MakeAnsiBg(const cgt::config::RGB& color);
    std::wstring MakeAnsiReset();
    std::wstring PaintText(const std::wstring& text,
                           const cgt::config::RGB* fg,
                           const cgt::config::RGB* bg = nullptr);

    int GetTerminalWidth();
    std::wstring MakeDividerLine(wchar_t fill = L'-');
    std::wstring MoveCursorTo(std::size_t row, std::size_t col = 1);
    std::wstring ClearCurrentLine();
    std::wstring HideCursor();
    std::wstring ShowCursor();

    std::wstring InsertCursorMarker(const std::wstring& text, std::size_t cursor);
    void InsertText(std::wstring& text, std::size_t& cursor, std::wstring_view value);
    void EraseBeforeCursor(std::wstring& text, std::size_t& cursor);
    void EraseAtCursor(std::wstring& text, std::size_t& cursor);
    void ClampCursor(std::wstring& text, std::size_t& cursor);

    std::wstring ToWide(std::size_t value);
}
