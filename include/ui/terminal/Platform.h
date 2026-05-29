#pragma once

#include <string>
#include <string_view>

#include "ui/Terminal.h"

namespace cgt::ui::teminal
{
    bool PlatformInit();
    void PlatformDestroy();

    bool PlatformWrite(std::string_view bytes);

    bool PlatformGetSize(WindowSize& size);
    bool PlatformGetCursorPos(CursorPos& pos);

#ifndef _WIN32
    bool PlatformReadBytes(std::string& bytes);
#endif
}
