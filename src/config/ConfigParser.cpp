#include "config/ConfigState.h"
#include "config/ConfigParserHelpers.h"

#include <fstream>

#include "config/Defaults.h"
#include "filter/Filter.h"

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

        state.ignoreRules.clear();

        detail::Section section = detail::Section::None;
        std::wstring currentTemplateName;
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
                auto inside = detail::Trim(line.substr(1, line.size() - 2));
                auto lowered = detail::ToLower(inside);

                if (lowered == L"general")
                {
                    section = detail::Section::General;
                    currentTemplateName.clear();
                }
                else if (lowered == L"ignore")
                {
                    section = detail::Section::Ignore;
                    currentTemplateName.clear();
                }
                else if (lowered == L"color")
                {
                    section = detail::Section::Color;
                    currentTemplateName.clear();
                }
                else if (lowered.rfind(L"template:", 0) == 0)
                {
                    auto pos = inside.find(L':');
                    currentTemplateName = detail::NormalizeTemplateName(
                        pos == std::wstring::npos ? L"" : inside.substr(pos + 1));

                    if (currentTemplateName.empty())
                    {
                        section = detail::Section::None;
                        detail::LogWarn(L"Template section has empty name: " + inside);
                    }
                    else
                    {
                        section = detail::Section::Template;
                    }
                }
                else
                {
                    section = detail::Section::None;
                    currentTemplateName.clear();
                    detail::LogWarn(L"Unknown section: " + inside);
                }
                continue;
            }

            if (section == detail::Section::General)
            {
                detail::HandleGeneralLine(line);
            }
            else if (section == detail::Section::Ignore)
            {
                state.ignoreRules.push_back(line);
            }
            else if (section == detail::Section::Color)
            {
                detail::HandleColorLine(line);
            }
            else if (section == detail::Section::Template)
            {
                if (currentTemplateName.empty())
                {
                    detail::LogWarn(L"Template line ignored without valid template name: " + line);
                }
                else
                {
                    detail::HandleTemplateLine(currentTemplateName, line);
                }
            }
            else
            {
                detail::LogWarn(L"Line outside section ignored: " + line);
            }
        }

        for (const auto& rule : state.ignoreRules)
        {
            cgt::filter::AddIgnore(rule);
        }

        state.parsed = true;
        return kStatusOk;
    }
}
