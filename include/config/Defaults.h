#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace ctg::config
{
    inline constexpr std::wstring_view kConfigFileName = L".cgtconfig";
    inline constexpr std::wstring_view kDefaultOutputFileName = L"cgt-result.txt";
    inline constexpr std::wstring_view kDefaultFilePrefix = L"**";

    inline constexpr int kStatusOk = 0;
    inline constexpr int kStatusNotInitialized = -1;
    inline constexpr int kStatusInvalidArg = -2;
    inline constexpr int kStatusIoError = -3;
    inline constexpr int kStatusParseError = -4;

    inline constexpr int kRgbMinTotal = 500;
}
