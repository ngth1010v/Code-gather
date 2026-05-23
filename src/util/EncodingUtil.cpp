#include "util/EncodingUtil.h"

#include <windows.h>

namespace cgt::util
{
    std::string WideToUtf8(std::wstring_view text)
    {
        if (text.empty())
        {
            return {};
        }

        const int required = WideCharToMultiByte(
            CP_UTF8,
            0,
            text.data(),
            static_cast<int>(text.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

        if (required <= 0)
        {
            return {};
        }

        std::string out(static_cast<std::size_t>(required), '\0');
        WideCharToMultiByte(
            CP_UTF8,
            0,
            text.data(),
            static_cast<int>(text.size()),
            out.data(),
            required,
            nullptr,
            nullptr);

        return out;
    }

    std::wstring Utf8ToWide(std::string_view text)
    {
        if (text.empty())
        {
            return {};
        }

        const int required = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            nullptr,
            0);

        if (required <= 0)
        {
            return {};
        }

        std::wstring out(static_cast<std::size_t>(required), L'\0');
        MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            text.data(),
            static_cast<int>(text.size()),
            out.data(),
            required);

        return out;
    }

}
