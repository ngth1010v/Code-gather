#include "io/FileReader.h"

#include <windows.h>
#include <fstream>
#include <vector>
#include <cstddef>
#include <algorithm>

namespace cgt::io
{
    namespace
    {
        enum class DetectedEncoding
        {
            Utf8,
            Utf16Le,
            Utf16Be,
            Ansi
        };

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
            std::wstring out(codeUnits, L'\0');

            for (std::size_t i = 0; i < codeUnits; ++i)
            {
                const unsigned char lo = static_cast<unsigned char>(bytes[i * 2]);
                const unsigned char hi = static_cast<unsigned char>(bytes[i * 2 + 1]);
                out[i] = static_cast<wchar_t>((static_cast<unsigned int>(hi) << 8) |
                                              static_cast<unsigned int>(lo));
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
            std::wstring out(codeUnits, L'\0');

            for (std::size_t i = 0; i < codeUnits; ++i)
            {
                const unsigned char hi = static_cast<unsigned char>(bytes[i * 2]);
                const unsigned char lo = static_cast<unsigned char>(bytes[i * 2 + 1]);
                out[i] = static_cast<wchar_t>((static_cast<unsigned int>(hi) << 8) |
                                              static_cast<unsigned int>(lo));
            }

            return out;
        }

        static bool IsValidUtf8(const std::string& bytes)
        {
            if (bytes.empty())
            {
                return true;
            }

            const int required = MultiByteToWideChar(
                CP_UTF8,
                MB_ERR_INVALID_CHARS,
                bytes.data(),
                static_cast<int>(bytes.size()),
                nullptr,
                0);

            return required > 0;
        }

        static double ZeroRatioAtParity(const std::string& bytes, std::size_t parity)
        {
            if (bytes.size() < 4)
            {
                return 0.0;
            }

            std::size_t hits = 0;
            std::size_t total = 0;

            for (std::size_t i = parity; i < bytes.size(); i += 2)
            {
                ++total;
                if (static_cast<unsigned char>(bytes[i]) == 0x00)
                {
                    ++hits;
                }
            }

            if (total == 0)
            {
                return 0.0;
            }

            return static_cast<double>(hits) / static_cast<double>(total);
        }

        static DetectedEncoding DetectEncoding(const std::string& bytes)
        {
            if (bytes.size() >= 3 &&
                static_cast<unsigned char>(bytes[0]) == 0xEF &&
                static_cast<unsigned char>(bytes[1]) == 0xBB &&
                static_cast<unsigned char>(bytes[2]) == 0xBF)
            {
                return DetectedEncoding::Utf8;
            }

            if (bytes.size() >= 2 &&
                static_cast<unsigned char>(bytes[0]) == 0xFF &&
                static_cast<unsigned char>(bytes[1]) == 0xFE)
            {
                return DetectedEncoding::Utf16Le;
            }

            if (bytes.size() >= 2 &&
                static_cast<unsigned char>(bytes[0]) == 0xFE &&
                static_cast<unsigned char>(bytes[1]) == 0xFF)
            {
                return DetectedEncoding::Utf16Be;
            }

            if (IsValidUtf8(bytes))
            {
                return DetectedEncoding::Utf8;
            }

            const double oddZeroRatio  = ZeroRatioAtParity(bytes, 1);
            const double evenZeroRatio = ZeroRatioAtParity(bytes, 0);

            if (oddZeroRatio > 0.30 && evenZeroRatio < 0.15)
            {
                return DetectedEncoding::Utf16Le;
            }

            if (evenZeroRatio > 0.30 && oddZeroRatio < 0.15)
            {
                return DetectedEncoding::Utf16Be;
            }

            return DetectedEncoding::Ansi;
        }
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

        if (bytes.empty())
        {
            result.success = true;
            result.content.clear();
            return result;
        }

        const DetectedEncoding encoding = DetectEncoding(bytes);

        switch (encoding)
        {
            case DetectedEncoding::Utf8:
                if (bytes.size() >= 3 &&
                    static_cast<unsigned char>(bytes[0]) == 0xEF &&
                    static_cast<unsigned char>(bytes[1]) == 0xBB &&
                    static_cast<unsigned char>(bytes[2]) == 0xBF)
                {
                    result.content = Utf8BytesToWide(bytes.substr(3));
                }
                else
                {
                    result.content = Utf8BytesToWide(bytes);
                }
                break;

            case DetectedEncoding::Utf16Le:
                if (bytes.size() >= 2 &&
                    static_cast<unsigned char>(bytes[0]) == 0xFF &&
                    static_cast<unsigned char>(bytes[1]) == 0xFE)
                {
                    result.content = Utf16LeBytesToWide(bytes.substr(2));
                }
                else
                {
                    result.content = Utf16LeBytesToWide(bytes);
                }
                break;

            case DetectedEncoding::Utf16Be:
                if (bytes.size() >= 2 &&
                    static_cast<unsigned char>(bytes[0]) == 0xFE &&
                    static_cast<unsigned char>(bytes[1]) == 0xFF)
                {
                    result.content = Utf16BeBytesToWide(bytes.substr(2));
                }
                else
                {
                    result.content = Utf16BeBytesToWide(bytes);
                }
                break;

            case DetectedEncoding::Ansi:
            default:
                result.content = AnsiBytesToWide(bytes);
                break;
        }

        result.success = true;
        return result;
    }
}