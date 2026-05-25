#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace cgt::config
{
    struct RGB
    {
        int r;
        int g;
        int b;
    };

    struct CgtTemplate
    {
        std::wstring         output     = L"cgt-result.txt";
        std::wstring         filePrefix = L"**";
        std::vector<std::wstring> extFilters;
        std::vector<std::wstring> dirFilters;
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

    int SetTemplate(std::wstring templateName, CgtTemplate tl);
    int RemoveTemplate(std::wstring templateName);
    CgtTemplate GetTemplate(std::wstring templateName);
}
