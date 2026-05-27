#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace cgt::io
{
    namespace fs = std::filesystem;

    struct GatheredBlock
    {
        std::wstring relativePath;
        std::wstring content;
    };

    class TargetWriter
    {
    public:
        bool Write(const fs::path& targetPath,
                   bool wrapped,
                   bool replace,
                   const std::vector<GatheredBlock>& blocks,
                   std::wstring& errorMessage) const;
    };
}
