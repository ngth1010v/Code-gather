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
        std::wstring MakeAnsiColorPrefix(const char* kind, const cgt::config::RGB& color)
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
        if (hIn == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        INPUT_RECORD record{};
        DWORD read = 0;
        for (;;)
        {
            if (!ReadConsoleInputW(hIn, &record, 1, &read) || read == 0)
            {
                return false;
            }

            if (record.EventType != KEY_EVENT)
            {
                continue;
            }

            const KEY_EVENT_RECORD& k = record.Event.KeyEvent;
            if (!k.bKeyDown)
            {
                continue;
            }

            key = {};
            key.ctrl = (k.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;
            key.alt = (k.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;
            key.shift = (k.dwControlKeyState & SHIFT_PRESSED) != 0;
            key.ch = k.uChar.UnicodeChar;

            switch (k.wVirtualKeyCode)
            {
                case VK_RETURN:   key.type = KeyEvent::Type::Enter; break;
                case VK_ESCAPE:   key.type = KeyEvent::Type::Escape; break;
                case VK_BACK:     key.type = KeyEvent::Type::Backspace; break;
                case VK_DELETE:   key.type = KeyEvent::Type::DeleteKey; break;
                case VK_LEFT:     key.type = KeyEvent::Type::Left; break;
                case VK_RIGHT:    key.type = KeyEvent::Type::Right; break;
                case VK_UP:       key.type = KeyEvent::Type::Up; break;
                case VK_DOWN:     key.type = KeyEvent::Type::Down; break;
                case VK_HOME:     key.type = KeyEvent::Type::Home; break;
                case VK_END:      key.type = KeyEvent::Type::End; break;
                default:
                    if (key.ch != 0)
                    {
                        key.type = KeyEvent::Type::Character;
                    }
                    break;
            }

            return true;
        }
    }

    std::wstring MakeAnsiFg(const cgt::config::RGB& color)
    {
        return MakeAnsiColorPrefix("fg", color);
    }

    std::wstring MakeAnsiBg(const cgt::config::RGB& color)
    {
        return MakeAnsiColorPrefix("bg", color);
    }

    std::wstring MakeAnsiReset()
    {
        return L"\x1b[0m";
    }

    std::wstring PaintText(const std::wstring& text,
                           const cgt::config::RGB* fg,
                           const cgt::config::RGB* bg)
    {
        std::wstring out;
        if (fg != nullptr)
        {
            out += MakeAnsiFg(*fg);
        }
        if (bg != nullptr)
        {
            out += MakeAnsiBg(*bg);
        }
        out += text;
        out += MakeAnsiReset();
        return out;
    }

    int GetTerminalWidth()
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE)
        {
            return 80;
        }

        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (!GetConsoleScreenBufferInfo(hOut, &info))
        {
            return 80;
        }

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
        std::wstring out;
        out.reserve(24);
        out += L"\x1b[";
        out += std::to_wstring(row);
        out += L';';
        out += std::to_wstring(col);
        out += L'H';
        return out;
    }

    std::wstring ClearCurrentLine()
    {
        return L"\x1b[2K";
    }

    std::wstring HideCursor()
    {
        return L"\x1b[?25l";
    }

    std::wstring ShowCursor()
    {
        return L"\x1b[?25h";
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
        if (cursor == 0 || text.empty())
        {
            return;
        }

        --cursor;
        text.erase(cursor, 1);
    }

    void EraseAtCursor(std::wstring& text, std::size_t& cursor)
    {
        if (cursor >= text.size())
        {
            return;
        }

        text.erase(cursor, 1);
    }

    void ClampCursor(std::wstring& text, std::size_t& cursor)
    {
        if (cursor > text.size())
        {
            cursor = text.size();
        }
    }

    std::wstring ToWide(std::size_t value)
    {
        return std::to_wstring(value);
    }
}
