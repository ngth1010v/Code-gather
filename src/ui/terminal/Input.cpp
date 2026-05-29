#include "ui/terminal/Input.h"

#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "ui/terminal/State.h"
#include "ui/terminal/Platform.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <cerrno>
#endif

namespace cgt::ui::teminal
{
    namespace
    {
        void PushResize(const WindowSize& size)
        {
            auto& s = State();
            if (!s.size_valid || s.last_size.w != size.w || s.last_size.h != size.h)
            {
                s.last_size = size;
                s.size_valid = true;
                s.resize_events.push_back(size);
            }
        }

        void PushMouse(const CursorPos& pos)
        {
            State().mouse_events.push_back(pos);
        }

        void PushKey(const std::vector<wchar>& key)
        {
            if (!key.empty())
            {
                State().key_events.push_back(key);
            }
        }

        void PushCmd(const std::vector<CmdKey>& key)
        {
            if (!key.empty())
            {
                State().cmd_events.push_back(key);
            }
        }

        void EmitCmd(CmdKey key)
        {
            PushCmd(std::vector<CmdKey>{key});
        }

        bool IsPrintableAscii(unsigned char c)
        {
            return c >= 0x20 && c <= 0x7E;
        }

#ifndef _WIN32
        std::uint32_t DecodeUtf8(const std::string& s, std::size_t& i, bool& need_more)
        {
            need_more = false;
            const unsigned char c0 = static_cast<unsigned char>(s[i]);
            if (c0 < 0x80)
            {
                return c0;
            }

            int len = 0;
            std::uint32_t cp = 0;
            if ((c0 & 0xE0) == 0xC0)
            {
                len = 2;
                cp = c0 & 0x1F;
            }
            else if ((c0 & 0xF0) == 0xE0)
            {
                len = 3;
                cp = c0 & 0x0F;
            }
            else if ((c0 & 0xF8) == 0xF0)
            {
                len = 4;
                cp = c0 & 0x07;
            }
            else
            {
                ++i;
                return 0xFFFD;
            }

            if (i + static_cast<std::size_t>(len) > s.size())
            {
                need_more = true;
                return 0;
            }

            for (int j = 1; j < len; ++j)
            {
                const unsigned char cx = static_cast<unsigned char>(s[i + j]);
                if ((cx & 0xC0) != 0x80)
                {
                    ++i;
                    return 0xFFFD;
                }
                cp = (cp << 6) | (cx & 0x3F);
            }

            if ((len == 2 && cp < 0x80) ||
                (len == 3 && cp < 0x800) ||
                (len == 4 && cp < 0x10000) ||
                cp > 0x10FFFF ||
                (cp >= 0xD800 && cp <= 0xDFFF))
            {
                ++i;
                return 0xFFFD;
            }

            i += static_cast<std::size_t>(len);
            return cp;
        }

        void FeedUtf8Char(std::uint32_t cp)
        {
            if (cp == 0)
            {
                return;
            }

            std::vector<wchar> key;
            if constexpr (sizeof(wchar_t) == 2)
            {
                if (cp <= 0xFFFF)
                {
                    key.push_back(static_cast<wchar>(cp));
                }
                else
                {
                    cp -= 0x10000;
                    key.push_back(static_cast<wchar>(0xD800 + ((cp >> 10) & 0x3FF)));
                    key.push_back(static_cast<wchar>(0xDC00 + (cp & 0x3FF)));
                }
            }
            else
            {
                key.push_back(static_cast<wchar>(cp));
            }
            PushKey(key);
        }

        void ParseEscapeSequence(const std::string& seq)
        {
            if (seq.size() < 2 || seq[0] != '\x1b')
            {
                return;
            }

            if (seq.size() == 1)
            {
                EmitCmd(CmdKey::Escape);
                return;
            }

            if (seq[1] == '[')
            {
                if (seq == "\x1b[A") { EmitCmd(CmdKey::Up); return; }
                if (seq == "\x1b[B") { EmitCmd(CmdKey::Down); return; }
                if (seq == "\x1b[C") { EmitCmd(CmdKey::Right); return; }
                if (seq == "\x1b[D") { EmitCmd(CmdKey::Left); return; }
                if (seq == "\x1b[H") { EmitCmd(CmdKey::Home); return; }
                if (seq == "\x1b[F") { EmitCmd(CmdKey::End); return; }
                if (seq == "\x1b[5~") { EmitCmd(CmdKey::PageUp); return; }
                if (seq == "\x1b[6~") { EmitCmd(CmdKey::PageDown); return; }
                if (seq == "\x1b[2~") { EmitCmd(CmdKey::Insert); return; }
                if (seq == "\x1b[3~") { EmitCmd(CmdKey::Delete); return; }
                if (seq == "\x1b[1~") { EmitCmd(CmdKey::Home); return; }
                if (seq == "\x1b[4~") { EmitCmd(CmdKey::End); return; }
                if (seq == "\x1bOP") { EmitCmd(CmdKey::F1); return; }
                if (seq == "\x1bOQ") { EmitCmd(CmdKey::F2); return; }
                if (seq == "\x1bOR") { EmitCmd(CmdKey::F3); return; }
                if (seq == "\x1bOS") { EmitCmd(CmdKey::F4); return; }

                if (seq.rfind("\x1b[<", 0) == 0)
                {
                    const char* p = seq.c_str() + 3;
                    char* end = nullptr;
                    const long btn = std::strtol(p, &end, 10);
                    if (end && *end == ';')
                    {
                        const long x = std::strtol(end + 1, &end, 10);
                        if (end && *end == ';')
                        {
                            const long y = std::strtol(end + 1, &end, 10);
                            if (end && (*end == 'M' || *end == 'm'))
                            {
                                if (*end == 'M')
                                {
                                    PushMouse(CursorPos{static_cast<int>(x - 1), static_cast<int>(y - 1)});
                                }
                                return;
                            }
                        }
                    }
                }

                EmitCmd(CmdKey::Escape);
                return;
            }

            if (seq[1] == 'O')
            {
                if (seq == "\x1bOH") { EmitCmd(CmdKey::Home); return; }
                if (seq == "\x1bOF") { EmitCmd(CmdKey::End); return; }
                if (seq == "\x1bOP") { EmitCmd(CmdKey::F1); return; }
                if (seq == "\x1bOQ") { EmitCmd(CmdKey::F2); return; }
                if (seq == "\x1bOR") { EmitCmd(CmdKey::F3); return; }
                if (seq == "\x1bOS") { EmitCmd(CmdKey::F4); return; }
            }

            EmitCmd(CmdKey::Escape);
        }
#endif
    }

    int UpdateInput()
    {
        int count = 0;
        auto& s = State();

#ifdef _WIN32
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        if (hIn == INVALID_HANDLE_VALUE)
        {
            return -1;
        }

        DWORD available = 0;
        while (GetNumberOfConsoleInputEvents(hIn, &available) && available > 0)
        {
            INPUT_RECORD rec{};
            DWORD read = 0;
            if (!ReadConsoleInputW(hIn, &rec, 1, &read) || read == 0)
            {
                break;
            }

            if (rec.EventType == KEY_EVENT)
            {
                const KEY_EVENT_RECORD& k = rec.Event.KeyEvent;
                if (!k.bKeyDown)
                {
                    continue;
                }

                const WORD vk = k.wVirtualKeyCode;
                const WCHAR ch = k.uChar.UnicodeChar;

                switch (vk)
                {
                    case VK_RETURN: EmitCmd(CmdKey::Enter); ++count; continue;
                    case VK_ESCAPE: EmitCmd(CmdKey::Escape); ++count; continue;
                    case VK_BACK: EmitCmd(CmdKey::Backspace); ++count; continue;
                    case VK_TAB: EmitCmd(CmdKey::Tab); ++count; continue;
                    case VK_LEFT: EmitCmd(CmdKey::Left); ++count; continue;
                    case VK_RIGHT: EmitCmd(CmdKey::Right); ++count; continue;
                    case VK_UP: EmitCmd(CmdKey::Up); ++count; continue;
                    case VK_DOWN: EmitCmd(CmdKey::Down); ++count; continue;
                    case VK_HOME: EmitCmd(CmdKey::Home); ++count; continue;
                    case VK_END: EmitCmd(CmdKey::End); ++count; continue;
                    case VK_PRIOR: EmitCmd(CmdKey::PageUp); ++count; continue;
                    case VK_NEXT: EmitCmd(CmdKey::PageDown); ++count; continue;
                    case VK_INSERT: EmitCmd(CmdKey::Insert); ++count; continue;
                    case VK_DELETE: EmitCmd(CmdKey::Delete); ++count; continue;
                    case VK_F1: EmitCmd(CmdKey::F1); ++count; continue;
                    case VK_F2: EmitCmd(CmdKey::F2); ++count; continue;
                    case VK_F3: EmitCmd(CmdKey::F3); ++count; continue;
                    case VK_F4: EmitCmd(CmdKey::F4); ++count; continue;
                    case VK_F5: EmitCmd(CmdKey::F5); ++count; continue;
                    case VK_F6: EmitCmd(CmdKey::F6); ++count; continue;
                    case VK_F7: EmitCmd(CmdKey::F7); ++count; continue;
                    case VK_F8: EmitCmd(CmdKey::F8); ++count; continue;
                    case VK_F9: EmitCmd(CmdKey::F9); ++count; continue;
                    case VK_F10: EmitCmd(CmdKey::F10); ++count; continue;
                    case VK_F11: EmitCmd(CmdKey::F11); ++count; continue;
                    case VK_F12: EmitCmd(CmdKey::F12); ++count; continue;
                    default:
                        break;
                }

                if (ch != 0)
                {
                    if (ch >= 32 || ch == L'\r' || ch == L'\n')
                    {
                        if (ch == L'\r' || ch == L'\n')
                        {
                            EmitCmd(CmdKey::Enter);
                            ++count;
                            continue;
                        }

                        PushKey(std::vector<wchar>{static_cast<wchar>(ch)});
                        ++count;
                    }
                }
            }
            else if (rec.EventType == MOUSE_EVENT)
            {
                const MOUSE_EVENT_RECORD& m = rec.Event.MouseEvent;
                const bool pressed = (m.dwEventFlags == 0) && (m.dwButtonState != 0);
                if (pressed)
                {
                    PushMouse(CursorPos{static_cast<int>(m.dwMousePosition.X), static_cast<int>(m.dwMousePosition.Y)});
                    ++count;
                }
            }
            else if (rec.EventType == WINDOW_BUFFER_SIZE_EVENT)
            {
                const auto& ws = rec.Event.WindowBufferSizeEvent.dwSize;
                PushResize(WindowSize{static_cast<int>(ws.X), static_cast<int>(ws.Y)});
                ++count;
            }
        }
#else
        std::string bytes;
        if (!PlatformReadBytes(bytes))
        {
            return -1;
        }

        for (std::size_t i = 0; i < bytes.size();)
        {
            const unsigned char c = static_cast<unsigned char>(bytes[i]);

            if (s.esc_pending)
            {
                s.esc_buffer.push_back(static_cast<char>(c));

                if (s.esc_buffer.size() == 2 && s.esc_buffer[1] != '[' && s.esc_buffer[1] != 'O')
                {
                    ParseEscapeSequence(std::string(1, '\x1b') + s.esc_buffer[1]);
                    s.esc_pending = false;
                    s.esc_buffer.clear();
                    ++i;
                    ++count;
                    continue;
                }

                if (s.esc_buffer.size() >= 2)
                {
                    const char last = s.esc_buffer.back();
                    const bool done = (last >= 0x40 && last <= 0x7E && s.esc_buffer[1] == '[') ||
                                      (s.esc_buffer[1] == 'O' && s.esc_buffer.size() >= 3);
                    if (done)
                    {
                        ParseEscapeSequence(std::string(1, '\x1b') + s.esc_buffer);
                        s.esc_pending = false;
                        s.esc_buffer.clear();
                        ++i;
                        ++count;
                        continue;
                    }
                }

                ++i;
                continue;
            }

            if (c == 0x1b)
            {
                s.esc_pending = true;
                s.esc_buffer.clear();
                s.esc_buffer.push_back('\x1b');
                s.esc_started = std::chrono::steady_clock::now();
                ++i;
                continue;
            }

            if (c < 0x80)
            {
                if (c == '\r' || c == '\n')
                {
                    EmitCmd(CmdKey::Enter);
                    ++count;
                }
                else if (c == 0x7F || c == 0x08)
                {
                    EmitCmd(CmdKey::Backspace);
                    ++count;
                }
                else if (c == '\t')
                {
                    EmitCmd(CmdKey::Tab);
                    ++count;
                }
                else if (IsPrintableAscii(c))
                {
                    PushKey(std::vector<wchar>{static_cast<wchar>(c)});
                    ++count;
                }
                ++i;
                continue;
            }

            s.pending_utf8.push_back(static_cast<char>(c));
            bool need_more = false;
            std::size_t j = 0;
            const std::uint32_t cp = DecodeUtf8(s.pending_utf8, j, need_more);
            if (need_more)
            {
                ++i;
                continue;
            }

            if (cp == 0xFFFD)
            {
                s.pending_utf8.erase(0, 1);
                ++i;
                continue;
            }

            s.pending_utf8.erase(0, j);
            FeedUtf8Char(cp);
            ++count;
            ++i;
        }

        if (s.esc_pending)
        {
            const auto now = std::chrono::steady_clock::now();
            if (s.esc_buffer.size() == 1)
            {
                if (s.esc_started.time_since_epoch().count() == 0)
                {
                    s.esc_started = now;
                }
                else if (now - s.esc_started > std::chrono::milliseconds(15))
                {
                    EmitCmd(CmdKey::Escape);
                    s.esc_pending = false;
                    s.esc_buffer.clear();
                    s.esc_started = {};
                    ++count;
                }
            }
        }
#endif

        return count;
    }
}
