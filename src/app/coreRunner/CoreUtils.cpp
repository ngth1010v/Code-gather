#include "app/coreRunner/CoreUtils.h"

#include <algorithm>
#include <cwctype>
#include <iostream>
#include <sstream>
#include <string>

namespace cgt::app::coreRunner
{
    namespace
    {
        std::wstring MakeAnsiColorPrefix(const char* kind, const cgt::RGB& color)
        {
            std::wstring out;
            out.reserve(32);
            out += L"\x1b[";
            out += (kind[0] == 'b' ? L'4' : L'3');
            out += L'8';
            out += L';';
            out += L'2';
            out += L';';
            out += std::to_wstring(color.r);
            out += L';';
            out += std::to_wstring(color.g);
            out += L';';
            out += std::to_wstring(color.b);
            out += L'm';
            return out;
        }

        std::wstring RepeatChar(wchar_t ch, std::size_t count)
        {
            return std::wstring(count, ch);
        }
    }

    void EnableVirtualTerminal()
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;

        DWORD mode = 0;
        if (!GetConsoleMode(hOut, &mode)) return;

        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, mode);
    }

    void ClearScreen()
    {
        std::wcout << L"\x1b[2J\x1b[H";
        std::wcout.flush();
    }

    void MoveCursorHome()
    {
        std::wcout << L"\x1b[H";
        std::wcout.flush();
    }

    bool ReadKey(KeyEvent& key)
    {
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        if (hIn == INVALID_HANDLE_VALUE) return false;

        INPUT_RECORD record{};
        DWORD read = 0;
        for (;;)
        {
            if (!ReadConsoleInputW(hIn, &record, 1, &read) || read == 0)
                return false;

            if (record.EventType != KEY_EVENT) continue;

            const KEY_EVENT_RECORD& k = record.Event.KeyEvent;
            if (!k.bKeyDown) continue;

            key = {};
            key.ctrl  = (k.dwControlKeyState & (LEFT_CTRL_PRESSED  | RIGHT_CTRL_PRESSED))  != 0;
            key.alt   = (k.dwControlKeyState & (LEFT_ALT_PRESSED   | RIGHT_ALT_PRESSED))   != 0;
            key.shift = (k.dwControlKeyState & SHIFT_PRESSED) != 0;
            key.ch    = k.uChar.UnicodeChar;

            switch (k.wVirtualKeyCode)
            {
                case VK_RETURN:  key.type = KeyEvent::Type::Enter;     break;
                case VK_ESCAPE:  key.type = KeyEvent::Type::Escape;    break;
                case VK_BACK:    key.type = KeyEvent::Type::Backspace;  break;
                case VK_DELETE:  key.type = KeyEvent::Type::DeleteKey;  break;
                case VK_LEFT:    key.type = KeyEvent::Type::Left;       break;
                case VK_RIGHT:   key.type = KeyEvent::Type::Right;      break;
                case VK_UP:      key.type = KeyEvent::Type::Up;         break;
                case VK_DOWN:    key.type = KeyEvent::Type::Down;       break;
                case VK_HOME:    key.type = KeyEvent::Type::Home;       break;
                case VK_END:     key.type = KeyEvent::Type::End;        break;
                default:
                    if (key.ch != 0) key.type = KeyEvent::Type::Character;
                    break;
            }
            return true;
        }
    }

    std::wstring MakeAnsiFg(const cgt::RGB& color) { return MakeAnsiColorPrefix("fg", color); }
    std::wstring MakeAnsiBg(const cgt::RGB& color) { return MakeAnsiColorPrefix("bg", color); }
    std::wstring MakeAnsiReset()                           { return L"\x1b[0m"; }

    std::wstring PaintText(const std::wstring& text,
                           const cgt::RGB* fg,
                           const cgt::RGB* bg)
    {
        std::wstring out;
        if (fg) out += MakeAnsiFg(*fg);
        if (bg) out += MakeAnsiBg(*bg);
        out += text;
        out += MakeAnsiReset();
        return out;
    }

    int GetTerminalWidth()
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return 80;

        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (!GetConsoleScreenBufferInfo(hOut, &info)) return 80;

        const int width = static_cast<int>(info.srWindow.Right - info.srWindow.Left + 1);
        return width > 0 ? width : 80;
    }

    std::wstring MakeDividerLine(wchar_t fill)
    {
        const int width = GetTerminalWidth();
        return RepeatChar(fill, static_cast<std::size_t>(width > 0 ? width : 80));
    }

    std::wstring MoveCursorTo(std::size_t row, std::size_t col)
    {
        // NOTE: This VT escape is viewport-relative (1-based).
        // Use WriteLineAbsolute() instead of this when painting tree rows,
        // because the tree can be taller than the visible viewport.
        std::wstring out;
        out.reserve(24);
        out += L"\x1b[";
        out += std::to_wstring(row);
        out += L';';
        out += std::to_wstring(col);
        out += L'H';
        return out;
    }

    std::wstring ClearCurrentLine() { return L"\x1b[2K"; }
    std::wstring HideCursor()       { return L"\x1b[?25l"; }
    std::wstring ShowCursor()       { return L"\x1b[?25h"; }

    // -------------------------------------------------------------------------
    // EnsureConsoleBufferHeight
    //
    // THE BUG (both previous attempts):
    //
    //   Attempt 1 – Only called SetConsoleScreenBufferSize.
    //     The buffer grew but the visible WINDOW (viewport) stayed at its
    //     original height (e.g. 30 rows).  VT escape \x1b[row;colH is
    //     viewport-relative, so addressing row 60 with a 30-row window either
    //     clips to row 30 or causes erratic scroll — every line from row 30
    //     downward overwrote the same bottom line.
    //
    //   Attempt 2 – Added newline-scrolling to push the viewport down.
    //     Emitting N newlines does scroll the viewport, but it moves
    //     srWindow.Top to N.  After that \x1b[H ("cursor home") lands at
    //     buffer row N, not row 0.  Every subsequent MoveCursorTo(r,1) then
    //     renders at buffer row N+r instead of row r — the whole display is
    //     shifted down by N rows, making it even worse.
    //
    // THE CORRECT FIX:
    //
    //   The root cause is that VT cursor positioning is viewport-relative, so
    //   the viewport must be pinned at the top of the buffer (srWindow.Top = 0)
    //   and must be tall enough to contain all required rows.
    //
    //   Steps:
    //     1. Expand the screen BUFFER to at least requiredRows tall.
    //     2. Anchor the visible WINDOW at Top=0 and grow it to requiredRows,
    //        clamped to GetLargestConsoleWindowSize() so we never exceed the
    //        physical display.
    //     3. Do NOT emit newlines — that would shift the viewport anchor.
    //     4. Move cursor to (1,1) so rendering starts at the top.
    //
    //   After this call, VT row 1 == buffer row 0, row 2 == buffer row 1, etc.
    //   MoveCursorTo(r,1) reliably addresses buffer row r-1 for all r up to
    //   the window height.
    //
    //   For trees taller than the physical display the window is clamped to
    //   the max display height.  WriteLineAbsolute (below) handles that case
    //   by using Win32 SetConsoleCursorPosition, which addresses by absolute
    //   buffer coordinates and is not limited by the viewport height.
    // -------------------------------------------------------------------------
    bool EnsureConsoleBufferHeight(std::size_t requiredRows)
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return false;

        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (!GetConsoleScreenBufferInfo(hOut, &info)) return false;

        // ── Step 1: grow buffer to at least requiredRows ──────────────────────
        const SHORT needBuf = static_cast<SHORT>(requiredRows);
        if (info.dwSize.Y < needBuf)
        {
            COORD newSize = info.dwSize;
            newSize.Y = needBuf;
            if (!SetConsoleScreenBufferSize(hOut, newSize)) return false;
            if (!GetConsoleScreenBufferInfo(hOut, &info)) return false;
        }

        // ── Step 2: anchor window at Top=0, expand to requiredRows ───────────
        // Clamp to what the physical display can actually show.
        const COORD maxWin    = GetLargestConsoleWindowSize(hOut);
        const SHORT maxWinH   = (maxWin.Y > 0) ? maxWin.Y : info.srWindow.Bottom - info.srWindow.Top + 1;
        const SHORT targetWinH = static_cast<SHORT>(
            std::min<std::size_t>(requiredRows, static_cast<std::size_t>(maxWinH)));

        SMALL_RECT win;
        win.Left   = info.srWindow.Left;
        win.Right  = info.srWindow.Right;
        win.Top    = 0;                       // ← pin to buffer top — CRITICAL
        win.Bottom = targetWinH - 1;

        // SetConsoleWindowInfo will fail if the window would exceed the buffer.
        // Buffer is already >= requiredRows >= targetWinH, so this is safe.
        SetConsoleWindowInfo(hOut, TRUE, &win);

        // ── Step 3: cursor to (1,1) — do NOT emit newlines ───────────────────
        // Newlines would scroll the viewport down (raise srWindow.Top),
        // breaking the Top=0 anchor we just established.
        std::wcout << L"\x1b[H";
        std::wcout.flush();

        return true;
    }

    // -------------------------------------------------------------------------
    // WriteLineAbsolute
    //
    // Positions the cursor at absolute buffer row `row` (1-based, same as the
    // row numbers used throughout CoreSelector) using Win32
    // SetConsoleCursorPosition, then writes `text` with VT colour sequences.
    //
    // Why this exists:
    //   VT \x1b[row;colH is viewport-relative.  Even after EnsureConsoleBufferHeight
    //   pins the viewport to Top=0, the visible window is clamped to the physical
    //   display height.  For very large trees where requiredRows > display height,
    //   VT rows beyond the window height cannot be addressed with \x1b[row;colH.
    //   SetConsoleCursorPosition addresses the full buffer unconditionally, so the
    //   render always works — the user can scroll the terminal to see all rows.
    // -------------------------------------------------------------------------
    void WriteLineAbsolute(std::size_t row, const std::wstring& text)
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;

        // Convert 1-based row to 0-based buffer coordinate
        const COORD pos{ 0, static_cast<SHORT>(row - 1) };
        SetConsoleCursorPosition(hOut, pos);

        // Clear to end-of-line then write text with any embedded VT sequences
        std::wcout << L"\x1b[2K" << text;
        std::wcout.flush();
    }

    void ClearRenderedRegion(std::size_t firstRow, std::size_t lastRow)
    {
        if (firstRow == 0 || lastRow < firstRow) return;

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;

        for (std::size_t row = firstRow; row <= lastRow; ++row)
        {
            const COORD pos{ 0, static_cast<SHORT>(row - 1) };
            SetConsoleCursorPosition(hOut, pos);
            std::wcout << L"\x1b[2K";
        }

        // Return to top-left
        SetConsoleCursorPosition(hOut, { 0, 0 });
        std::wcout.flush();
    }

    std::wstring InsertCursorMarker(const std::wstring& text, std::size_t cursor)
    {
        const std::size_t pos = std::min(cursor, text.size());
        std::wstring out;
        out.reserve(text.size() + 1);
        out.append(text.substr(0, pos));
        out.push_back(L'|');
        out.append(text.substr(pos));
        return out;
    }

    void InsertText(std::wstring& text, std::size_t& cursor, std::wstring_view value)
    {
        cursor = std::min(cursor, text.size());
        text.insert(cursor, value);
        cursor += value.size();
    }

    void EraseBeforeCursor(std::wstring& text, std::size_t& cursor)
    {
        if (cursor == 0 || text.empty()) return;
        --cursor;
        text.erase(cursor, 1);
    }

    void EraseAtCursor(std::wstring& text, std::size_t& cursor)
    {
        if (cursor >= text.size()) return;
        text.erase(cursor, 1);
    }

    void ClampCursor(std::wstring& text, std::size_t& cursor)
    {
        if (cursor > text.size()) cursor = text.size();
    }

    std::wstring ToWide(std::size_t value)
    {
        return std::to_wstring(value);
    }
}
