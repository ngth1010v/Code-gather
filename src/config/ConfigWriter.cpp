#include "config/ConfigState.h"

#include <fstream>

#include "config/Defaults.h"

namespace cgt::config
{
    int Write()
    {
        auto& state = detail::State();
        if (!state.initialized)
        {
            detail::LogError(L"Write() called before Init().");
            return kStatusNotInitialized;
        }

        std::ofstream fout(state.configPath, std::ios::trunc);
        if (!fout.is_open())
        {
            detail::LogError(L"Failed to open config file for write.");
            return kStatusIoError;
        }

        fout << "[GENERAL]\n";
        fout << "output=" << detail::ToNarrow(detail::PathToWide(state.outputFilePath)) << "\n";
        fout << "filePrefix=" << detail::ToNarrow(state.filePrefix) << "\n\n";

        fout << "[IGNORE]\n";
        for (const auto& rule : state.ignoreRules)
        {
            fout << detail::ToNarrow(rule) << "\n";
        }
        fout << "\n";

        fout << "[COLOR]\n";
        for (const auto& [ext, rgb] : state.extColors)
        {
            fout << detail::ToNarrow(ext) << "="
                 << rgb.r << "," << rgb.g << "," << rgb.b << "\n";
        }

        return kStatusOk;
    }
}
