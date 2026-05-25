#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "config/Config.h"

namespace fs = std::filesystem;

namespace cgt::config::detail
{
    struct ConfigState
    {
        bool initialized = false;
        bool parsed = false;
        fs::path workspaceDir;
        fs::path configPath;
        fs::path outputFilePath;
        std::wstring filePrefix;
        std::vector<std::wstring> ignoreRules;
        std::vector<std::vector<std::wstring>> ruleComponentList;
        std::map<std::wstring, RGB> extColors;
        std::map<std::wstring, CgtTemplate> templates;
    };

    ConfigState& State();
    void ResetToDefaults();
    fs::path MakeConfigPath(const fs::path& workspaceDir);

    std::wstring NormalizeExt(std::wstring ext);
    std::wstring NormalizeTemplateName(std::wstring templateName);
    std::wstring NormalizeLine(std::wstring line);
    std::wstring ToLower(std::wstring s);
    std::wstring Trim(const std::wstring& s);
    std::wstring PathToWide(const fs::path& p);
    std::string ToNarrow(const std::wstring& s);
    fs::path WideToPath(const std::wstring& s);

    std::wstring RelativeGeneric(const fs::path& p);
    bool PathExists(const fs::path& p);

    RGB RandomBrightRGB();
    void LogInfo(const std::wstring& msg);
    void LogWarn(const std::wstring& msg);
    void LogError(const std::wstring& msg);
}
