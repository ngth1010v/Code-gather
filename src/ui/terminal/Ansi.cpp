#include "ui/terminal/Ansi.h"

#include <cstdio>
#include <cstdint>
#include <sstream>
#include <string>

#include "ui/terminal/State.h"

namespace cgt::ui::teminal
{
    namespace
    {
        void AppendRaw(std::string_view s)
        {
            State().out.append(s.data(), s.size());
        }

        void AppendAscii(const char* s)
        {
            State().out.append(s);
        }

        void AppendInt(int value)
        {
            char buf[32];
            const int n = std::snprintf(buf, sizeof(buf), "%d", value);
            if (n > 0)
            {
                State().out.append(buf, static_cast<std::size_t>(n));
            }
        }

        void AppendUtf8Codepoint(std::string& out, std::uint32_t cp)
        {
            if (cp <= 0x7F)
            {
                out.push_back(static_cast<char>(cp));
            }
            else if (cp <= 0x7FF)
            {
                out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else if (cp <= 0xFFFF)
            {
                out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
            else
            {
                out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }

        void AppendWideUtf8(std::string& out, std::wstring_view text)
        {
            if constexpr (sizeof(wchar_t) == 2)
            {
                for (std::size_t i = 0; i < text.size(); ++i)
                {
                    std::uint32_t cp = static_cast<std::uint32_t>(text[i]);
                    if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < text.size())
                    {
                        const std::uint32_t low = static_cast<std::uint32_t>(text[i + 1]);
                        if (low >= 0xDC00 && low <= 0xDFFF)
                        {
                            cp = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
                            ++i;
                        }
                    }
                    AppendUtf8Codepoint(out, cp);
                }
            }
            else
            {
                for (wchar_t ch : text)
                {
                    AppendUtf8Codepoint(out, static_cast<std::uint32_t>(ch));
                }
            }
        }
    }

    void AppendAltEnter()
    {
        AppendAscii("\x1b[?1049h");
    }

    void AppendAltLeave()
    {
        AppendAscii("\x1b[?1049l");
    }

    void AppendHideCursor()
    {
        AppendAscii("\x1b[?25l");
    }

    void AppendShowCursor()
    {
        AppendAscii("\x1b[?25h");
    }

    void AppendClear()
    {
        AppendAscii("\x1b[2J\x1b[H");
    }

    void AppendReset()
    {
        auto& s = State();
        AppendAscii("\x1b[0m");
        s.fg_set = false;
        s.bg_set = false;
    }

    void AppendMoveTo(const CursorPos& pos)
    {
        AppendAscii("\x1b[");
        AppendInt(pos.y + 1);
        AppendAscii(";");
        AppendInt(pos.x + 1);
        AppendAscii("H");
    }

    void AppendFontColor(const RGB& rgb)
    {
        auto& s = State();
        if (s.fg_set && s.fg.r == rgb.r && s.fg.g == rgb.g && s.fg.b == rgb.b)
        {
            return;
        }
        s.fg = rgb;
        s.fg_set = true;
        AppendAscii("\x1b[38;2;");
        AppendInt(static_cast<int>(rgb.r));
        AppendAscii(";");
        AppendInt(static_cast<int>(rgb.g));
        AppendAscii(";");
        AppendInt(static_cast<int>(rgb.b));
        AppendAscii("m");
    }

    void AppendBackgroundColor(const RGB& rgb)
    {
        auto& s = State();
        if (s.bg_set && s.bg.r == rgb.r && s.bg.g == rgb.g && s.bg.b == rgb.b)
        {
            return;
        }
        s.bg = rgb;
        s.bg_set = true;
        AppendAscii("\x1b[48;2;");
        AppendInt(static_cast<int>(rgb.r));
        AppendAscii(";");
        AppendInt(static_cast<int>(rgb.g));
        AppendAscii(";");
        AppendInt(static_cast<int>(rgb.b));
        AppendAscii("m");
    }

    void AppendTitle(std::wstring_view title)
    {
        auto& out = State().out;
        out.append("\x1b]0;");
        AppendWideUtf8(out, title);
        out.push_back('\x07');
    }

    void AppendText(std::wstring_view text)
    {
        AppendWideUtf8(State().out, text);
    }
}
