#include "ui/Terminal.h"

#include <utility>

#include "ui/terminal/Ansi.h"
#include "ui/terminal/Input.h"
#include "ui/terminal/Platform.h"
#include "ui/terminal/State.h"

namespace cgt::ui::teminal
{
    namespace
    {
        void EnsureInitOrNoop()
        {
            (void)0;
        }
    }

    int Init()
    {
        auto& s = State();
        if (s.initialized)
        {
            return 0;
        }

        if (!PlatformInit())
        {
            return -1;
        }

        s = TerminalState{};
        s.initialized = true;

        AppendAltEnter();
        AppendHideCursor();
        s.cursor_visible = false;
        AppendClear();
        return Flush();
    }

    int Destroy()
    {
        auto& s = State();
        if (!s.initialized)
        {
            return 0;
        }

        AppendReset();
        AppendShowCursor();
        AppendAltLeave();
        Flush();

        PlatformDestroy();
        s = TerminalState{};
        return 0;
    }

    int Update()
    {
        auto& s = State();
        if (!s.initialized)
        {
            return -1;
        }

        int count = 0;
        WindowSize size{};
        if (PlatformGetSize(size))
        {
            if (!s.size_valid || s.last_size.w != size.w || s.last_size.h != size.h)
            {
                s.last_size = size;
                s.size_valid = true;
                s.resize_events.push_back(size);
                ++count;
            }
        }

        const int input_count = UpdateInput();
        if (input_count > 0)
        {
            count += input_count;
        }
        return count;
    }

    int Clean()
    {
        auto& s = State();
        if (!s.initialized)
        {
            return -1;
        }

        AppendReset();
        AppendClear();
        return Flush();
    }

    int GetSize(WindowSize& size)
    {
        return PlatformGetSize(size) ? 0 : -1;
    }

    int GetCursorPos(CursorPos& pos)
    {
        Flush();
        return PlatformGetCursorPos(pos) ? 0 : -1;
    }

    int ShowCursor(bool visible)
    {
        auto& s = State();
        if (s.cursor_visible == visible)
        {
            return 0;
        }

        s.cursor_visible = visible;
        if (visible)
        {
            AppendShowCursor();
        }
        else
        {
            AppendHideCursor();
        }
        return 0;
    }

    int SetTitle(std::wstring_view title)
    {
        AppendTitle(title);
        return 0;
    }

    int MoveTo(const CursorPos& pos)
    {
        AppendMoveTo(pos);
        return 0;
    }

    int SetFontColor(const RGB& rgb)
    {
        AppendFontColor(rgb);
        return 0;
    }

    int SetBackgroundColor(const RGB& rgb)
    {
        AppendBackgroundColor(rgb);
        return 0;
    }

    int Print(std::wstring_view text)
    {
        AppendText(text);
        return 0;
    }

    int Flush()
    {
        auto& s = State();
        if (s.out.empty())
        {
            return 0;
        }

        const bool ok = PlatformWrite(s.out);
        s.out.clear();
        return ok ? 0 : -1;
    }

    bool IsResize(WindowSize& size)
    {
        auto& s = State();
        if (s.resize_events.empty())
        {
            return false;
        }
        size = s.resize_events.front();
        s.resize_events.pop_front();
        return true;
    }

    bool IsMouseDown(CursorPos& pos, std::vector<MouseKey>& key)
    {
        auto& s = State();
        if (s.mouse_events.empty())
        {
            return false;
        }
        
        // Match the structural change made in state tracking
        pos = s.mouse_events.front().pos;
        key = std::move(s.mouse_events.front().keys);
        
        s.mouse_events.pop_front();
        return true;
    }

    bool IsMouseScroll(CursorPos& pos, int& delta)
    {
        auto& s = State();
        if (s.scroll_events.empty())
        {
            return false;
        }
        
        pos = s.scroll_events.front().pos;
        delta = s.scroll_events.front().delta;
        
        s.scroll_events.pop_front();
        return true;
    }

    bool IsKeyDown(std::vector<wchar>& key)
    {
        auto& s = State();
        if (s.key_events.empty())
        {
            return false;
        }
        key = std::move(s.key_events.front());
        s.key_events.pop_front();
        return true;
    }

    bool IsCmdKeyDown(std::vector<CmdKey>& key)
    {
        auto& s = State();
        if (s.cmd_events.empty())
        {
            return false;
        }
        key = std::move(s.cmd_events.front());
        s.cmd_events.pop_front();
        return true;
    }
}
