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
        std::wstring output = L"cgt-result.txt";
        std::vector<std::wstring> filters;
    };

    int Init(fs::path workspaceDir);
    int Parse();
    int Write();

    RGB GetExtColor(std::wstring ext);

    fs::path GetOutputFilePath();
    int SetOutputFilePath(fs::path p);

    bool HasTemplate(std::wstring templateName);
    std::vector<std::wstring> GetTemplateNames();
    int SetTemplate(std::wstring templateName, CgtTemplate tl);
    int RemoveTemplate(std::wstring templateName);
    CgtTemplate GetTemplate(std::wstring templateName);
}
