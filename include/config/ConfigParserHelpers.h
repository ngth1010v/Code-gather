#pragma once

#include <string>
#include <vector>

#include "config/Config.h"

namespace cgt::config::detail
{
    enum class Section
    {
        None,
        General,
        Ignore,
        Color,
        Template
    };

    RGB ParseRGBLine(const std::wstring& value, bool& ok);
    std::vector<std::wstring> ParseListLine(const std::wstring& value);

    int HandleGeneralLine(const std::wstring& line);
    int HandleColorLine(const std::wstring& line);
    int HandleTemplateLine(const std::wstring& templateName, const std::wstring& line);
}
