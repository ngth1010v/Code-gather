#include "io/TargetWriter.h"

#include "util/EncodingUtil.h"

#include <filesystem>
#include <fstream>
#include <system_error>

namespace cgt::io
{
    static std::string ToUtf8(const std::wstring& text)
    {
        return cgt::util::WideToUtf8(text);
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

        const bool needHeader = replace || !exists || size == 0;
        if (needHeader)
        {
            const std::string bom = "\xEF\xBB\xBF";
            file.write(bom.data(), static_cast<std::streamsize>(bom.size()));
            const std::wstring header = L"*CODE:\n";
            const std::string headerBytes = ToUtf8(header);
            file.write(headerBytes.data(), static_cast<std::streamsize>(headerBytes.size()));
        }
        else
        {
            file << '\n';
        }

        for (const auto& block : blocks)
        {
            std::wstring startLine = prefix + L"START " + block.relativePath + L"\n";
            std::wstring endLine = prefix + L"END " + block.relativePath + L"\n";

            const std::string startBytes = ToUtf8(startLine);
            const std::string bodyBytes = ToUtf8(block.content);
            const std::string endBytes = ToUtf8(endLine);

            file.write(startBytes.data(), static_cast<std::streamsize>(startBytes.size()));
            file.write(bodyBytes.data(), static_cast<std::streamsize>(bodyBytes.size()));
            if (block.content.empty() || (block.content.back() != L'\n' && block.content.back() != L'\r'))
            {
                const std::string newline = "\n";
                file.write(newline.data(), 1);
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
