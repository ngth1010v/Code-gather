#pragma once

#include "util/Types.h"

#include <string_view>

namespace cgt::app::coreRunner
{
    inline constexpr cgt::RGB kPanelText{ 240, 240, 240 };
    inline constexpr cgt::RGB kHintText{ 180, 180, 180 };

    inline constexpr cgt::RGB kHeaderBackground{ 24, 24, 24 };
    inline constexpr cgt::RGB kPanelBackground{ 18, 18, 18 };

    inline constexpr cgt::RGB kDetectedHeaderFg{ 80, 255, 255 };
    inline constexpr cgt::RGB kSelectedHeaderFg{ 80, 255, 255 };
    inline constexpr cgt::RGB kInputHeaderFg   { 80, 255, 255 };

    inline constexpr cgt::RGB kSelectedBackground{ 40, 96, 180 };
    inline constexpr cgt::RGB kDetectedSelectedBackground{ 44, 88, 60 };
    inline constexpr cgt::RGB kDetectedActiveBackground{ 60, 84, 140 };

    inline constexpr int kTargetFps = 60;

    inline constexpr int kMinLeftPanelWidth = 18;
    inline constexpr int kMinRightPanelWidth = 18;
    inline constexpr int kMinSelectedPanelHeight = 3;
    inline constexpr int kMinInputPanelHeight = 2;

    inline constexpr std::wstring_view kDetectedHeaderLabel = L"[Detected files]";
    inline constexpr std::wstring_view kSelectedHeaderLabel = L"[Selected tree]";
    inline constexpr std::wstring_view kInputHeaderLabel = L"[Selected files]";
}
