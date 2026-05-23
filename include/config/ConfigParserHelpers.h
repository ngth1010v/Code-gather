#pragma once

#include <string>

#include "config/Config.h"

namespace ctg::config::detail
{
    enum class Section
    {
        None,
        General,
        Ignore,
        Color
    };

    RGB ParseRGBLine(const std::wstring& value, bool& ok);
    int HandleGeneralLine(const std::wstring& line);
    int HandleColorLine(const std::wstring& line);
}
