#include "config/ConfigState.h"

#include <algorithm>
#include <cctype>
#include <locale>
#include <random>

#include "config/Defaults.h"
#include "log/Logger.h"

namespace cgt::config::detail
{
    ConfigState& State()
    {
        static ConfigState state;
        return state;
    }

    void ResetToDefaults()
    {
        auto& s = State();
        s.parsed = false;
        s.ignoreRules.clear();
        s.extColors = kDefaultExtColors;
        s.outputFilePath = s.workspaceDir / std::wstring(kDefaultOutputFileName);
        s.filePrefix = std::wstring(kDefaultFilePrefix);
    }

    fs::path MakeConfigPath(const fs::path& workspaceDir)
    {
        return workspaceDir / std::wstring(kConfigFileName);
    }

    static std::wstring NarrowToWide(const std::string& s)
    {
        return std::wstring(s.begin(), s.end());
    }

    std::string ToNarrow(const std::wstring& s)
    {
        return std::string(s.begin(), s.end());
    }

    std::wstring PathToWide(const fs::path& p)
    {
        return NarrowToWide(p.generic_string());
    }

    fs::path WideToPath(const std::wstring& s)
    {
        return fs::path(std::string(s.begin(), s.end()));
    }

    std::wstring ToLower(std::wstring s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        return s;
    }

    std::wstring Trim(const std::wstring& s)
    {
        const auto start = s.find_first_not_of(L" \t\r\n");
        if (start == std::wstring::npos) return L"";
        const auto end = s.find_last_not_of(L" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    std::wstring NormalizeLine(std::wstring line)
    {
        line = Trim(line);
        while (!line.empty() && (line.back() == L'\r' || line.back() == L'\n'))
        {
            line.pop_back();
        }
        return line;
    }

    std::wstring NormalizeExt(std::wstring ext)
    {
        ext = Trim(ext);
        if (!ext.empty() && ext.front() == L'.')
        {
            ext.erase(ext.begin());
        }
        return ToLower(ext);
    }

    std::wstring RelativeGeneric(const fs::path& p)
    {
        auto& s = State();
        std::error_code ec;
        fs::path abs = fs::absolute(p, ec);
        if (ec) abs = p;
        fs::path rel = fs::relative(abs, s.workspaceDir, ec);
        if (ec) rel = abs.filename();
        return ToLower(PathToWide(rel));
    }

    bool PathExists(const fs::path& p)
    {
        std::error_code ec;
        return fs::exists(p, ec);
    }

    RGB RandomBrightRGB()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 255);
        RGB rgb{};
        do
        {
            rgb.r = dist(gen);
            rgb.g = dist(gen);
            rgb.b = dist(gen);
        }
        while (rgb.r + rgb.g + rgb.b < kRgbMinTotal);
        return rgb;
    }

    void LogInfo(const std::wstring& msg)
    {
        cgt::log::Logger::Info(L"cgt::config", msg);
    }

    void LogWarn(const std::wstring& msg)
    {
        cgt::log::Logger::Warning(L"cgt::config", msg);
    }

    void LogError(const std::wstring& msg)
    {
        cgt::log::Logger::Error(L"cgt::config", msg);
    }
}
