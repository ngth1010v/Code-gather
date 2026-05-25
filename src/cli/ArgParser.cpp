#include "cli/ArgParser.h"

#include "util/StringUtil.h"

namespace cgt::cli
{
    ParsedArgs ArgParser::Parse(int argc, wchar_t* argv[]) const
    {
        ParsedArgs parsed;

        for (int i = 1; i < argc; ++i)
        {
            std::wstring token = util::Trim(argv[i]);
            if (token.empty())
            {
                continue;
            }

            // Xử lý các tham số dạng --key=value hoặc --flag
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
                    continue;
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
                    parsed.unknownTokens.push_back(argv[i]);
                }
                continue;
            }

            // Xử lý các flag hoặc token không hợp lệ bắt đầu bằng '-'
            if (token[0] == L'-')
            {
                parsed.unknownTokens.push_back(token);
                continue;
            }

            // Lưu lại token gốc vào sourceArgs trước khi phân loại sâu hơn
            parsed.sourceArgs.push_back(token);

            if (token[0] == L'.')
            {
                // Arg bắt đầu bằng "." -> đưa vào extFilters
                parsed.extFilters.push_back(token);
            }
            else
            {
                // Arg không bắt đầu bằng "." hay "-" -> đưa vào dirFilters
                // Xử lý loại bỏ dấu nháy (nếu có) phòng trường hợp đường dẫn chứa khoảng trắng
                parsed.dirFilters.push_back(util::StripQuotes(token));
            }
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