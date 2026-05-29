#include "ui/terminal/State.h"

namespace cgt::ui::teminal
{
    TerminalState& State()
    {
        static TerminalState s{};
        return s;
    }
}
