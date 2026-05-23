#include "config/ConfigState.h"

#include <system_error>

#include "config/Defaults.h"

namespace ctg::config
{
    int Init(fs::path workspaceDir)
    {
        auto& state = detail::State();
        if (workspaceDir.empty())
        {
            detail::LogError(L"Init() workspaceDir is empty.");
            return kStatusInvalidArg;
        }

        state.workspaceDir = std::move(workspaceDir);
        state.configPath = detail::MakeConfigPath(state.workspaceDir);
        state.initialized = true;
        detail::ResetToDefaults();

        std::error_code ec;
        if (!fs::exists(state.workspaceDir, ec))
        {
            detail::LogWarn(L"workspaceDir does not exist yet.");
        }

        if (!detail::PathExists(state.configPath))
        {
            detail::LogInfo(L"Config file not found. Creating default .cgtconfig.");
            return Write();
        }

        return kStatusOk;
    }

    fs::path GetOutputFilePath()
    {
        return detail::State().outputFilePath;
    }

    int SetOutputFilePath(fs::path p)
    {
        auto& state = detail::State();
        if (!state.initialized)
        {
            detail::LogError(L"SetOutputFilePath() called before Init().");
            return kStatusNotInitialized;
        }

        if (p.empty())
        {
            detail::LogError(L"SetOutputFilePath() received empty path.");
            return kStatusInvalidArg;
        }

        state.outputFilePath = std::move(p);
        return Write();
    }

    std::wstring GetFilePrefix()
    {
        return detail::State().filePrefix;
    }

    int SetFilePrefix(std::wstring prefix)
    {
        auto& state = detail::State();
        if (!state.initialized)
        {
            detail::LogError(L"SetFilePrefix() called before Init().");
            return kStatusNotInitialized;
        }

        prefix = detail::Trim(prefix);
        if (prefix.empty())
        {
            detail::LogError(L"SetFilePrefix() received empty value.");
            return kStatusInvalidArg;
        }

        state.filePrefix = std::move(prefix);
        return Write();
    }
}
