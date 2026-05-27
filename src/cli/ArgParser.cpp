#include "cli/ArgParser.h"

#include "app/AppUtils.h"
#include "util/StringUtil.h"

namespace cgt::cli
{
    namespace
    {
        static std::wstring NormalizeToken(const std::wstring& token)
        {
            return util::StripQuotes(util::Trim(token));
        }

        static bool IsHelpToken(const std::wstring& token)
        {
            return util::ToLower(NormalizeToken(token)) == L"--help";
        }

        static void AddCoreFilterTokens(ParsedArgs& parsed, const std::wstring& token)
        {
            const std::wstring normalized = cgt::app::NormalizeCliFilterRule(token);
            if (normalized.empty())
            {
                return;
            }

            parsed.filters.push_back(normalized);

            if (token.rfind(L"./", 0) == 0 || token.rfind(L".\\", 0) == 0 || token[0] == L'/' || token[0] == L'\\')
            {
                parsed.dirFilters.push_back(token);
                return;
            }

            if (token[0] == L'.')
            {
                parsed.extFilters.push_back(token);
                return;
            }

            parsed.dirFilters.push_back(token);
        }

        static void ParseCoreToken(ParsedArgs& parsed, const std::wstring& rawToken)
        {
            std::wstring token = NormalizeToken(rawToken);
            if (token.empty())
            {
                return;
            }

            if (token.rfind(L"--", 0) == 0)
            {
                token = token.substr(2);
                const std::size_t eq = token.find(L'=');

                if (eq == std::wstring::npos)
                {
                    std::wstring flag = util::ToLower(util::Trim(token));
                    if (!flag.empty())
                    {
                        parsed.flags.insert(flag);
                        parsed.orderedFlags.push_back(flag);
                    }
                    return;
                }

                std::wstring key = util::Trim(token.substr(0, eq));
                std::wstring value = util::StripQuotes(token.substr(eq + 1));
                key = util::ToLower(key);
                value = util::StripQuotes(value);

                if (!key.empty())
                {
                    parsed.configValues[key].push_back(value);
                }
                else
                {
                    parsed.unknownTokens.push_back(rawToken);
                }
                return;
            }

            parsed.sourceArgs.push_back(token);

            if (token[0] == L'-' && token.size() > 1)
            {
                parsed.templateNames.push_back(token.substr(1));
                return;
            }

            AddCoreFilterTokens(parsed, token);
        }

        static void ParseSetTemplateToken(ParsedArgs& parsed, const std::wstring& rawToken)
        {
            std::wstring token = NormalizeToken(rawToken);
            if (token.empty())
            {
                return;
            }

            if (token.rfind(L"--", 0) == 0)
            {
                token = token.substr(2);
                const std::size_t eq = token.find(L'=');
                if (eq == std::wstring::npos)
                {
                    parsed.unknownTokens.push_back(rawToken);
                    return;
                }

                std::wstring key = util::Trim(token.substr(0, eq));
                std::wstring value = util::StripQuotes(token.substr(eq + 1));
                key = util::ToLower(key);
                value = util::StripQuotes(value);

                if (!key.empty())
                {
                    parsed.configValues[key].push_back(value);
                }
                else
                {
                    parsed.unknownTokens.push_back(rawToken);
                }
                return;
            }

            parsed.sourceArgs.push_back(token);

            if (token[0] == L'.' || token[0] == L'/' || token[0] == L'\\' || token[0] == L'*')
            {
                const std::wstring normalized = cgt::app::NormalizeCliFilterRule(token);
                if (!normalized.empty())
                {
                    parsed.filters.push_back(normalized);
                }
            }

            if (token[0] == L'.')
            {
                parsed.extFilters.push_back(token);
                return;
            }

            if (token[0] == L'-')
            {
                parsed.unknownTokens.push_back(rawToken);
                return;
            }

            parsed.dirFilters.push_back(token);
        }

        static void ParseRemoveTemplateToken(ParsedArgs& parsed, const std::wstring& rawToken)
        {
            std::wstring token = NormalizeToken(rawToken);
            if (token.empty())
            {
                return;
            }

            if (token.rfind(L"--", 0) != 0)
            {
                parsed.unknownTokens.push_back(rawToken);
                return;
            }

            token = token.substr(2);
            const std::size_t eq = token.find(L'=');

            if (eq != std::wstring::npos)
            {
                parsed.unknownTokens.push_back(rawToken);
                return;
            }

            std::wstring flag = util::ToLower(util::Trim(token));
            if (flag.empty())
            {
                parsed.unknownTokens.push_back(rawToken);
                return;
            }

            parsed.flags.insert(flag);
            parsed.orderedFlags.push_back(flag);
        }
    }

    ParsedArgs ArgParser::Parse(int argc, wchar_t* argv[]) const
    {
        ParsedArgs parsed;

        if (argc <= 1)
        {
            return parsed;
        }

        for (int i = 1; i < argc; ++i)
        {
            if (IsHelpToken(argv[i]))
            {
                parsed.mode = ParsedArgs::Mode::Help;
                return parsed;
            }
        }

        const std::wstring firstToken = NormalizeToken(argv[1]);
        const std::wstring firstLower = util::ToLower(firstToken);

        if (firstLower == L"settemplate")
        {
            parsed.mode = ParsedArgs::Mode::SetTemplate;

            if (argc >= 3)
            {
                parsed.templateNames.push_back(NormalizeToken(argv[2]));
            }

            for (int i = 3; i < argc; ++i)
            {
                ParseSetTemplateToken(parsed, argv[i]);
            }

            return parsed;
        }

        if (firstLower == L"rmtemplate")
        {
            parsed.mode = ParsedArgs::Mode::RemoveTemplate;

            for (int i = 2; i < argc; ++i)
            {
                std::wstring token = NormalizeToken(argv[i]);

                if (token.rfind(L"--", 0) == 0)
                {
                    ParseRemoveTemplateToken(parsed, token);
                    continue;
                }

                if (parsed.templateNames.empty())
                {
                    parsed.templateNames.push_back(token);
                }
                else
                {
                    parsed.unknownTokens.push_back(token);
                }
            }

            return parsed;
        }

        parsed.mode = ParsedArgs::Mode::Core;

        for (int i = 1; i < argc; ++i)
        {
            ParseCoreToken(parsed, argv[i]);
        }

        return parsed;
    }

    bool ParsedArgs::HasFlag(const std::wstring& flag) const
    {
        return flags.contains(util::ToLower(flag));
    }

    std::wstring ParsedArgs::GetFirstConfigValue(const std::wstring& key, const std::wstring& defaultValue) const
    {
        auto it = configValues.find(util::ToLower(key));
        if (it == configValues.end() || it->second.empty())
        {
            return defaultValue;
        }
        return it->second.front();
    }

    std::vector<std::wstring> ParsedArgs::GetConfigValues(const std::wstring& key) const
    {
        auto it = configValues.find(util::ToLower(key));
        if (it == configValues.end())
        {
            return {};
        }
        return it->second;
    }
}
