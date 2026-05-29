#pragma once

#include "app/CoreConstant.h"
#include "util/Types.h"

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

    std::wstring MakeAnsiFg(const cgt::RGB& color);
    std::wstring MakeAnsiBg(const cgt::RGB& color);
    std::wstring MakeAnsiReset();
    std::wstring PaintText(const std::wstring& text,
                           const cgt::RGB* fg,
                           const cgt::RGB* bg = nullptr);

    int GetTerminalWidth();
    std::wstring MakeDividerLine(wchar_t fill = L'-');
    std::wstring MoveCursorTo(std::size_t row, std::size_t col = 1);
    std::wstring ClearCurrentLine();
    std::wstring HideCursor();
    std::wstring ShowCursor();

    bool EnsureConsoleBufferHeight(std::size_t requiredRows);

    // Positions cursor at absolute buffer row `row` (1-based) using Win32
    // SetConsoleCursorPosition, clears the line, then writes `text`.
    // Use this instead of MoveCursorTo+wcout for all tree/prompt rows so
    // rendering works even when the tree is taller than the visible viewport.
    void WriteLineAbsolute(std::size_t row, const std::wstring& text);

    void ClearRenderedRegion(std::size_t firstRow, std::size_t lastRow);

    std::wstring InsertCursorMarker(const std::wstring& text, std::size_t cursor);
    void InsertText(std::wstring& text, std::size_t& cursor, std::wstring_view value);
    void EraseBeforeCursor(std::wstring& text, std::size_t& cursor);
    void EraseAtCursor(std::wstring& text, std::size_t& cursor);
    void ClampCursor(std::wstring& text, std::size_t& cursor);

    std::wstring ToWide(std::size_t value);
}
