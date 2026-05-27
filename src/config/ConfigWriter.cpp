#include "config/ConfigState.h"

#include <fstream>

#include "config/Defaults.h"
#include "filter/Filter.h"

namespace cgt::config
{
    static std::wstring JoinList(const std::vector<std::wstring>& items)
    {
        std::wstring result;
        for (size_t i = 0; i < items.size(); ++i)
        {
            if (i > 0)
            {
                result += L",";
            }
            result += items[i];
        }
        return result;
    }

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
        for (const auto& rule : cgt::filter::GetIgnores())
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

        if (!state.templates.empty())
        {
            fout << "\n";
            bool firstTemplate = true;
            for (const auto& [name, tl] : state.templates)
            {
                if (!firstTemplate)
                {
                    fout << "\n";
                }
                firstTemplate = false;

                fout << "[TEMPLATE:" << detail::ToNarrow(name) << "]\n";
                fout << "output=" << detail::ToNarrow(detail::PathToWide(tl.output)) << "\n";
                fout << "filePrefix=" << detail::ToNarrow(tl.filePrefix) << "\n";
                fout << "filters=" << detail::ToNarrow(JoinList(tl.filters)) << "\n";
            }
        }

        return kStatusOk;
    }
}
