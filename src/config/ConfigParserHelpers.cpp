#include "config/ConfigParserHelpers.h"

#include <sstream>

#include "config/ConfigState.h"

namespace cgt::config::detail
{
    RGB ParseRGBLine(const std::wstring& value, bool& ok)
    {
        ok = false;
        std::wstringstream ss(value);
        std::wstring part;
        int nums[3] = {0, 0, 0};

        for (int i = 0; i < 3; ++i)
        {
            if (!std::getline(ss, part, L',')) return {};
            part = Trim(part);
            try
            {
                nums[i] = std::stoi(part);
            }
            catch (...)
            {
                return {};
            }
            if (nums[i] < 0) nums[i] = 0;
            if (nums[i] > 255) nums[i] = 255;
        }

        ok = true;
        return {nums[0], nums[1], nums[2]};
    }

    std::vector<std::wstring> ParseListLine(const std::wstring& value)
    {
        std::vector<std::wstring> items;
        std::wstring current;

        for (size_t i = 0; i <= value.size(); ++i)
        {
            const bool atEnd = (i == value.size());
            const wchar_t ch = atEnd ? L',' : value[i];

            if (ch == L',')
            {
                auto item = Trim(current);
                if (!item.empty())
                {
                    items.push_back(std::move(item));
                }
                current.clear();
            }
            else
            {
                current.push_back(ch);
            }
        }

        return items;
    }

    int HandleGeneralLine(const std::wstring& line)
    {
        auto pos = line.find(L'=');
        if (pos == std::wstring::npos)
        {
            LogWarn(L"GENERAL line ignored: " + line);
            return 0;
        }

        auto key = ToLower(Trim(line.substr(0, pos)));
        auto value = Trim(line.substr(pos + 1));
        auto& state = State();

        if (key == L"output")
        {
            state.outputFilePath = WideToPath(value);
        }
        else if (key == L"fileprefix")
        {
            state.filePrefix = value;
        }
        else
        {
            LogWarn(L"Unknown GENERAL key: " + key);
        }

        return 0;
    }

    int HandleColorLine(const std::wstring& line)
    {
        auto pos = line.find(L'=');
        if (pos == std::wstring::npos)
        {
            LogWarn(L"COLOR line ignored: " + line);
            return 0;
        }

        auto key = NormalizeExt(Trim(line.substr(0, pos)));
        auto value = Trim(line.substr(pos + 1));

        bool ok = false;
        auto rgb = ParseRGBLine(value, ok);
        if (!ok)
        {
            LogWarn(L"Bad COLOR value: " + line);
            return 0;
        }

        State().extColors[key] = rgb;
        return 0;
    }

    static std::wstring NormalizeDirFilter(std::wstring value)
    {
        value = Trim(value);
        for (auto& ch : value)
        {
            if (ch == L'\\') ch = L'/';
        }
        value = ToLower(value);

        while (!value.empty() && value.front() == L'/')
        {
            value.erase(value.begin());
        }

        while (!value.empty() && value.back() == L'/')
        {
            value.pop_back();
        }

        return value;
    }

    int HandleTemplateLine(const std::wstring& templateName, const std::wstring& line)
    {
        auto pos = line.find(L'=');
        if (pos == std::wstring::npos)
        {
            LogWarn(L"TEMPLATE line ignored: " + line);
            return 0;
        }

        auto key = ToLower(Trim(line.substr(0, pos)));
        auto value = Trim(line.substr(pos + 1));
        auto& tl = State().templates[templateName];

        if (key == L"output")
        {
            tl.output = WideToPath(value);
        }
        else if (key == L"fileprefix")
        {
            tl.filePrefix = value;
        }
        else if (key == L"extfilters")
        {
            tl.extFilters.clear();
            for (auto item : ParseListLine(value))
            {
                item = NormalizeExt(item);
                if (!item.empty())
                {
                    tl.extFilters.push_back(std::move(item));
                }
            }
        }
        else if (key == L"dirfilters")
        {
            tl.dirFilters.clear();
            for (auto item : ParseListLine(value))
            {
                item = NormalizeDirFilter(std::move(item));
                if (!item.empty())
                {
                    tl.dirFilters.push_back(std::move(item));
                }
            }
        }
        else
        {
            LogWarn(L"Unknown TEMPLATE key: " + key);
        }

        return 0;
    }
}
