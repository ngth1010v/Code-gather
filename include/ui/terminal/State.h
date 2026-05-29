#pragma once

#include <chrono>
#include <deque>
#include <string>
#include <vector>

#include "ui/Terminal.h"
#include "util/Types.h"

namespace cgt::ui::teminal
{
    struct TerminalState
    {
        bool initialized{false};

        bool cursor_visible{true};
        bool fg_set{false};
        bool bg_set{false};

        RGB fg{};
        RGB bg{};

        WindowSize last_size{};
        bool size_valid{false};

        std::string out;

        struct MouseAction {
            CursorPos pos;
            std::vector<MouseKey> keys;
        };
        std::deque<MouseAction> mouse_events;
        int pending_scroll_delta{0};

        std::deque<WindowSize> resize_events;
        std::deque<std::vector<wchar>> key_events;
        std::deque<std::vector<CmdKey>> cmd_events;


        std::string pending_utf8;
        bool esc_pending{false};
        std::string esc_buffer;
        std::chrono::steady_clock::time_point esc_started{};
    };

    TerminalState& State();
}
