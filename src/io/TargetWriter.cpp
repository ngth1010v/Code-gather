#include "io/TargetWriter.h"

#include "util/EncodingUtil.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <system_error>

namespace cgt::io
{
    enum class FileEncoding
    {
        Utf16Le,
        Utf16Be,
        Utf8,
        Ansi
    };

    // Hàm chuyển đổi từ wstring sang UTF-8 bytes
    static std::string WideToUtf8Bytes(const std::wstring& wstr)
    {
        if (wstr.empty()) return {};
        int required = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        if (required <= 0) return {};
        std::string out(static_cast<std::size_t>(required), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), out.data(), required, nullptr, nullptr);
        return out;
    }

    // Hàm chuyển đổi từ wstring sang UTF-16LE bytes
    static std::string WideToUtf16LeBytes(const std::wstring& wstr)
    {
        std::string out;
        out.resize(wstr.size() * 2);
        for (std::size_t i = 0; i < wstr.size(); ++i)
        {
            unsigned int cp = static_cast<unsigned int>(wstr[i]);
            out[i * 2] = static_cast<char>(cp & 0xFF);
            out[i * 2 + 1] = static_cast<char>((cp >> 8) & 0xFF);
        }
        return out;
    }

    // Hàm chuyển đổi từ wstring sang UTF-16BE bytes
    static std::string WideToUtf16BeBytes(const std::wstring& wstr)
    {
        std::string out;
        out.resize(wstr.size() * 2);
        for (std::size_t i = 0; i < wstr.size(); ++i)
        {
            unsigned int cp = static_cast<unsigned int>(wstr[i]);
            out[i * 2] = static_cast<char>((cp >> 8) & 0xFF);
            out[i * 2 + 1] = static_cast<char>(cp & 0xFF);
        }
        return out;
    }

    // Hàm phụ trợ giúp nhận diện Encoding hiện tại của file (nếu có sẵn)
    static FileEncoding DetectExistingFileEncoding(const fs::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file) return FileEncoding::Utf16Le; // Mặc định cho file mới

        char bom[3] = {0};
        file.read(bom, 3);
        std::streamsize bytesRead = file.gcount();

        if (bytesRead >= 3 && 
            static_cast<unsigned char>(bom[0]) == 0xEF && 
            static_cast<unsigned char>(bom[1]) == 0xBB && 
            static_cast<unsigned char>(bom[2]) == 0xBF)
        {
            return FileEncoding::Utf8;
        }
        if (bytesRead >= 2 && 
            static_cast<unsigned char>(bom[0]) == 0xFF && 
            static_cast<unsigned char>(bom[1]) == 0xFE)
        {
            return FileEncoding::Utf16Le;
        }
        if (bytesRead >= 2 && 
            static_cast<unsigned char>(bom[0]) == 0xFE && 
            static_cast<unsigned char>(bom[1]) == 0xFF)
        {
            return FileEncoding::Utf16Be;
        }

        return FileEncoding::Utf16Le; // Fallback mặc định
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
        }

        const bool exists = fs::exists(targetPath, ec);
        const auto size = exists ? fs::file_size(targetPath, ec) : 0;

        // Xác định Encoding sẽ sử dụng để viết tiếp hoặc tạo mới
        FileEncoding encoding = FileEncoding::Utf16Le; // Mặc định cho file tạo mới hoàn toàn
        if (exists && !replace && size > 0)
        {
            encoding = DetectExistingFileEncoding(targetPath);
        }

        std::ios::openmode mode = std::ios::binary | std::ios::out;
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

        // Lambda mã hóa chuỗi wstring theo encoding đã chọn
        auto EncodeText = [encoding](const std::wstring& text) -> std::string {
            switch (encoding)
            {
                case FileEncoding::Utf8:     return WideToUtf8Bytes(text);
                case FileEncoding::Utf16Be:   return WideToUtf16BeBytes(text);
                case FileEncoding::Utf16Le:
                default:                     return WideToUtf16LeBytes(text);
            }
        };

        const bool needHeader = replace || !exists || size == 0;
        if (needHeader)
        {
            // Ghi BOM tương ứng cho từng loại Encoding file mới
            std::string bom;
            if (encoding == FileEncoding::Utf8)      bom = "\xEF\xBB\xBF";
            else if (encoding == FileEncoding::Utf16Le) bom = "\xFF\xFE";
            else if (encoding == FileEncoding::Utf16Be) bom = "\xFE\xFF";

            if (!bom.empty())
            {
                file.write(bom.data(), static_cast<std::streamsize>(bom.size()));
            }

            const std::wstring header = L"*CODE:\n";
            std::string headerBytes = EncodeText(header);
            file.write(headerBytes.data(), static_cast<std::streamsize>(headerBytes.size()));
        }
        else
        {
            std::string newlineBytes = EncodeText(L"\n");
            file.write(newlineBytes.data(), static_cast<std::streamsize>(newlineBytes.size()));
        }

        for (const auto& block : blocks)
        {
            std::wstring startLine = prefix + L"START " + block.relativePath + L"\n";
            std::wstring endLine = prefix + L"END " + block.relativePath + L"\n";

            const std::string startBytes = EncodeText(startLine);
            const std::string bodyBytes = EncodeText(block.content);
            const std::string endBytes = EncodeText(endLine);

            file.write(startBytes.data(), static_cast<std::streamsize>(startBytes.size()));
            file.write(bodyBytes.data(), static_cast<std::streamsize>(bodyBytes.size()));
            
            if (block.content.empty() || (block.content.back() != L'\n' && block.content.back() != L'\r'))
            {
                std::string blockNewline = EncodeText(L"\n");
                file.write(blockNewline.data(), static_cast<std::streamsize>(blockNewline.size()));
            }
            file.write(endBytes.data(), static_cast<std::streamsize>(endBytes.size()));
        }

        if (!file)
        {
            errorMessage = L"Failed while writing target file.";
            return false;
        }

        return true;
    }
}