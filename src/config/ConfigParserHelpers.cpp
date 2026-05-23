#include "config/ConfigParserHelpers.h"

#include <sstream>

#include "config/ConfigState.h"

namespace ctg::config::detail
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
}
