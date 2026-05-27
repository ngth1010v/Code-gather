#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <string_view>

namespace cgt::config
{
    inline constexpr std::wstring_view kConfigFileName = L".cgtconfig";
    inline constexpr std::wstring_view kDefaultOutputFileName = L"cgt-result.txt";

    inline constexpr int kStatusOk = 0;
    inline constexpr int kStatusNotInitialized = -1;
    inline constexpr int kStatusInvalidArg = -2;
    inline constexpr int kStatusIoError = -3;
    inline constexpr int kStatusParseError = -4;

    inline constexpr int kRgbMinTotal = 500;


    inline const std::map<std::wstring, RGB> kDefaultExtColors =
    {
        {L"cpp",       {255, 200, 200}},
        {L"h",         {200, 255, 255}},
        {L"md",        {255, 255, 180}},
        {L"txt",       {220, 220, 220}},
        {L"gitignore", {255, 180, 120}},
        {L"cgtconfig", {180, 255, 180}},
        {L"py",        {180, 220, 255}},
        {L"js",        {255, 235, 140}},
        {L"jsx",       {140, 220, 255}},
        {L"tsx",       {120, 200, 255}},
        {L"bat",       {180, 180, 180}},
        {L"c",         {200, 220, 255}}
    };

    inline const std::vector<std::wstring> kDefaultIgnoreRules =
    {
        L".vscode/",
        L".git/"
    };
}
