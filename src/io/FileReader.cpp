#include "io/FileReader.h"

#include "util/EncodingUtil.h"
#include "util/StringUtil.h"

#include <fstream>
#include <vector>
#include <windows.h>

namespace cgt::io
{
    static std::wstring Utf8BytesToWide(const std::string& bytes)
    {
        if (bytes.empty())
        {
            return {};
        }

        const int required = MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            bytes.data(),
            static_cast<int>(bytes.size()),
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
            bytes.data(),
            static_cast<int>(bytes.size()),
            out.data(),
            required);

        return out;
    }

    static std::wstring AnsiBytesToWide(const std::string& bytes)
    {
        if (bytes.empty())
        {
            return {};
        }

        const int required = MultiByteToWideChar(
            CP_ACP,
            0,
            bytes.data(),
            static_cast<int>(bytes.size()),
            nullptr,
            0);

        if (required <= 0)
        {
            return {};
        }

        std::wstring out(static_cast<std::size_t>(required), L'\0');
        MultiByteToWideChar(
            CP_ACP,
            0,
            bytes.data(),
            static_cast<int>(bytes.size()),
            out.data(),
            required);

        return out;
    }

    static std::wstring Utf16LeBytesToWide(const std::string& bytes)
    {
        if (bytes.size() < 2)
        {
            return {};
        }

        const std::size_t codeUnits = bytes.size() / 2;
        std::wstring out;
        out.resize(codeUnits);

        for (std::size_t i = 0; i < codeUnits; ++i)
        {
            const unsigned char lo = static_cast<unsigned char>(bytes[i * 2]);
            const unsigned char hi = static_cast<unsigned char>(bytes[i * 2 + 1]);
            out[i] = static_cast<wchar_t>((static_cast<unsigned int>(hi) << 8) | static_cast<unsigned int>(lo));
        }

        return out;
    }

    static std::wstring Utf16BeBytesToWide(const std::string& bytes)
    {
        if (bytes.size() < 2)
        {
            return {};
        }

        const std::size_t codeUnits = bytes.size() / 2;
        std::wstring out;
        out.resize(codeUnits);

        for (std::size_t i = 0; i < codeUnits; ++i)
        {
            const unsigned char hi = static_cast<unsigned char>(bytes[i * 2]);
            const unsigned char lo = static_cast<unsigned char>(bytes[i * 2 + 1]);
            out[i] = static_cast<wchar_t>((static_cast<unsigned int>(hi) << 8) | static_cast<unsigned int>(lo));
        }

        return out;
    }

    ReadResult FileReader::Read(const fs::path& path) const
    {
        ReadResult result;

        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            result.errorMessage = L"Cannot open file.";
            return result;
        }

        std::string bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (bytes.size() >= 3 &&
            static_cast<unsigned char>(bytes[0]) == 0xEF &&
            static_cast<unsigned char>(bytes[1]) == 0xBB &&
            static_cast<unsigned char>(bytes[2]) == 0xBF)
        {
            result.content = Utf8BytesToWide(bytes.substr(3));
            result.success = true;
            return result;
        }

        if (bytes.size() >= 2 &&
            static_cast<unsigned char>(bytes[0]) == 0xFF &&
            static_cast<unsigned char>(bytes[1]) == 0xFE)
        {
            result.content = Utf16LeBytesToWide(bytes.substr(2));
            result.success = true;
            return result;
        }

        if (bytes.size() >= 2 &&
            static_cast<unsigned char>(bytes[0]) == 0xFE &&
            static_cast<unsigned char>(bytes[1]) == 0xFF)
        {
            result.content = Utf16BeBytesToWide(bytes.substr(2));
            result.success = true;
            return result;
        }

        result.content = Utf8BytesToWide(bytes);
        if (!result.content.empty() || bytes.empty())
        {
            result.success = true;
            return result;
        }

        result.content = AnsiBytesToWide(bytes);
        if (!result.content.empty() || bytes.empty())
        {
            result.success = true;
            return result;
        }

        result.errorMessage = L"Failed to decode file as text.";
        return result;
    }
}
