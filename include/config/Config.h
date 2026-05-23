#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace cgt::config
{
    struct RGB
    {
        int r;
        int g;
        int b;
    };

    int Init(fs::path workspaceDir);
    int Parse();
    int Write();

    bool IsIgnored(fs::path p);

    RGB GetExtColor(std::wstring ext);

    fs::path GetOutputFilePath();
    int SetOutputFilePath(fs::path p);

    std::wstring GetFilePrefix();
    int SetFilePrefix(std::wstring prefix);
}
