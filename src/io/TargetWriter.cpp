#include "io/TargetWriter.h"

#include <windows.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <vector>
#include <algorithm>

namespace cgt::io
{
    namespace fs = std::filesystem;

    namespace
    {
        enum class FileEncoding
        {
            Utf8,
            Utf16Le,
            Utf16Be
        };

        static std::string WideToUtf8Bytes(const std::wstring& wstr)
        {
            if (wstr.empty())
            {
                return {};
            }

            const int required = WideCharToMultiByte(
                CP_UTF8,
                0,
                wstr.data(),
                static_cast<int>(wstr.size()),
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
                wstr.data(),
                static_cast<int>(wstr.size()),
                out.data(),
                required,
                nullptr,
                nullptr);

            return out;
        }

        static std::string WideToUtf16LeBytes(const std::wstring& wstr)
        {
            std::string out;
            out.resize(wstr.size() * 2);

            for (std::size_t i = 0; i < wstr.size(); ++i)
            {
                const unsigned int cp = static_cast<unsigned int>(wstr[i]);

                out[i * 2]     = static_cast<char>(cp & 0xFF);
                out[i * 2 + 1] = static_cast<char>((cp >> 8) & 0xFF);
            }

            return out;
        }

        static std::string WideToUtf16BeBytes(const std::wstring& wstr)
        {
            std::string out;
            out.resize(wstr.size() * 2);

            for (std::size_t i = 0; i < wstr.size(); ++i)
            {
                const unsigned int cp = static_cast<unsigned int>(wstr[i]);

                out[i * 2]     = static_cast<char>((cp >> 8) & 0xFF);
                out[i * 2 + 1] = static_cast<char>(cp & 0xFF);
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

        static FileEncoding DetectExistingFileEncoding(const fs::path& path)
        {
            std::ifstream file(path, std::ios::binary);

            if (!file)
            {
                return FileEncoding::Utf8;
            }

            std::vector<char> buffer(8192);

            file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

            const std::streamsize bytesRead = file.gcount();

            buffer.resize(static_cast<std::size_t>(
                std::max<std::streamsize>(0, bytesRead)));

            if (buffer.empty())
            {
                return FileEncoding::Utf8;
            }

            const std::string bytes(buffer.begin(), buffer.end());

            if (bytes.size() >= 3 &&
                static_cast<unsigned char>(bytes[0]) == 0xEF &&
                static_cast<unsigned char>(bytes[1]) == 0xBB &&
                static_cast<unsigned char>(bytes[2]) == 0xBF)
            {
                return FileEncoding::Utf8;
            }

            if (bytes.size() >= 2 &&
                static_cast<unsigned char>(bytes[0]) == 0xFF &&
                static_cast<unsigned char>(bytes[1]) == 0xFE)
            {
                return FileEncoding::Utf16Le;
            }

            if (bytes.size() >= 2 &&
                static_cast<unsigned char>(bytes[0]) == 0xFE &&
                static_cast<unsigned char>(bytes[1]) == 0xFF)
            {
                return FileEncoding::Utf16Be;
            }

            if (IsValidUtf8(bytes))
            {
                return FileEncoding::Utf8;
            }

            const double oddZeroRatio  = ZeroRatioAtParity(bytes, 1);
            const double evenZeroRatio = ZeroRatioAtParity(bytes, 0);

            if (oddZeroRatio > 0.30 && evenZeroRatio < 0.15)
            {
                return FileEncoding::Utf16Le;
            }

            if (evenZeroRatio > 0.30 && oddZeroRatio < 0.15)
            {
                return FileEncoding::Utf16Be;
            }

            return FileEncoding::Utf8;
        }

        static std::string EncodeText(const std::wstring& text,
                                      FileEncoding encoding)
        {
            switch (encoding)
            {
                case FileEncoding::Utf16Le:
                    return WideToUtf16LeBytes(text);

                case FileEncoding::Utf16Be:
                    return WideToUtf16BeBytes(text);

                case FileEncoding::Utf8:
                default:
                    return WideToUtf8Bytes(text);
            }
        }

        static std::string GetBom(FileEncoding encoding)
        {
            switch (encoding)
            {
                case FileEncoding::Utf8:
                    return std::string("\xEF\xBB\xBF", 3);

                case FileEncoding::Utf16Le:
                    return std::string("\xFF\xFE", 2);

                case FileEncoding::Utf16Be:
                    return std::string("\xFE\xFF", 2);

                default:
                    return {};
            }
        }

        static bool WriteBytes(std::ofstream& file,
                               const std::string& bytes)
        {
            if (bytes.empty())
            {
                return true;
            }

            file.write(bytes.data(),
                       static_cast<std::streamsize>(bytes.size()));

            return static_cast<bool>(file);
        }

        static bool WriteText(std::ofstream& file,
                              const std::wstring& text,
                              FileEncoding encoding)
        {
            return WriteBytes(file, EncodeText(text, encoding));
        }
    }

    bool TargetWriter::Write(const fs::path& targetPath,
                             const std::wstring& prefix,
                             bool replace,
                             const std::vector<GatheredBlock>& blocks,
                             std::wstring& errorMessage) const
    {
        errorMessage.clear();

        std::error_code ec;

        if (!targetPath.parent_path().empty())
        {
            fs::create_directories(targetPath.parent_path(), ec);

            if (ec)
            {
                errorMessage = L"Cannot create target directory.";
                return false;
            }
        }

        const bool exists = fs::exists(targetPath, ec);

        if (ec)
        {
            errorMessage = L"Cannot inspect target file.";
            return false;
        }

        std::uintmax_t size = 0;

        if (exists)
        {
            size = fs::file_size(targetPath, ec);

            if (ec)
            {
                size = 0;
                ec.clear();
            }
        }

        const bool needHeader =
            replace || !exists || size == 0;

        FileEncoding encoding = FileEncoding::Utf8;

        if (!needHeader && exists)
        {
            encoding = DetectExistingFileEncoding(targetPath);
        }

        std::ios::openmode mode =
            std::ios::binary |
            std::ios::out;

        if (replace)
        {
            mode |= std::ios::trunc;
        }
        else
        {
            mode |= std::ios::app;
        }

        std::ofstream file(targetPath, mode);

        if (!file)
        {
            errorMessage = L"Cannot open target file for writing.";
            return false;
        }

        if (needHeader)
        {
            if (!WriteBytes(file, GetBom(encoding)))
            {
                errorMessage = L"Failed while writing BOM.";
                return false;
            }

            if (!WriteText(file, L"*CODE:\n", encoding))
            {
                errorMessage = L"Failed while writing header.";
                return false;
            }
        }
        else
        {
            if (!WriteText(file, L"\n", encoding))
            {
                errorMessage = L"Failed while preparing append.";
                return false;
            }
        }

        for (const auto& block : blocks)
        {
            const std::wstring startLine =
                prefix + L"START " + block.relativePath + L"\n";

            const std::wstring endLine =
                prefix + L"END " + block.relativePath + L"\n";

            if (!WriteText(file, startLine, encoding))
            {
                errorMessage = L"Failed while writing block header.";
                return false;
            }

            if (!WriteText(file, block.content, encoding))
            {
                errorMessage = L"Failed while writing block body.";
                return false;
            }

            if (block.content.empty() ||
                (block.content.back() != L'\n' &&
                 block.content.back() != L'\r'))
            {
                if (!WriteText(file, L"\n", encoding))
                {
                    errorMessage = L"Failed while writing newline.";
                    return false;
                }
            }

            if (!WriteText(file, endLine, encoding))
            {
                errorMessage = L"Failed while writing block footer.";
                return false;
            }
        }

        if (!file)
        {
            errorMessage = L"Failed while writing target file.";
            return false;
        }

        return true;
    }
}