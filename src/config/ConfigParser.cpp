#include "config/ConfigState.h"
#include "config/ConfigParserHelpers.h"
#include "config/ConfigIgnoreHelpers.h"   // thêm

#include <fstream>

#include "config/Defaults.h"

namespace cgt::config
{
    int Parse()
    {
        auto& state = detail::State();
        if (!state.initialized)
        {
            detail::LogError(L"Parse() called before Init().");
            return kStatusNotInitialized;
        }

        detail::ResetToDefaults();

        std::ifstream fin(state.configPath);
        if (!fin.is_open())
        {
            detail::LogWarn(L"Config file missing. Defaults loaded.");
            state.parsed = true;
            return kStatusOk;
        }

        detail::Section section = detail::Section::None;
        std::string raw;

        while (std::getline(fin, raw))
        {
            std::wstring line(raw.begin(), raw.end());
            line = detail::NormalizeLine(line);

            if (line.empty() || line.front() == L'#' || line.front() == L';')
            {
                continue;
            }

            if (line.front() == L'[' && line.back() == L']')
            {
                auto name = detail::ToLower(detail::Trim(line.substr(1, line.size() - 2)));
                if (name == L"general") section = detail::Section::General;
                else if (name == L"ignore") section = detail::Section::Ignore;
                else if (name == L"color") section = detail::Section::Color;
                else
                {
                    section = detail::Section::None;
                    detail::LogWarn(L"Unknown section: " + name);
                }
                continue;
            }

            if (section == detail::Section::General)
            {
                detail::HandleGeneralLine(line);
            }
            else if (section == detail::Section::Ignore)
            {
                auto rule = detail::ToLower(line);
                state.ignoreRules.push_back(rule);
                state.ruleComponentList.push_back(detail::ComponentSpliter(rule));
            }
            else if (section == detail::Section::Color)
            {
                detail::HandleColorLine(line);
            }
            else
            {
                detail::LogWarn(L"Line outside section ignored: " + line);
            }
        }

        state.parsed = true;
        return kStatusOk;
    }
}