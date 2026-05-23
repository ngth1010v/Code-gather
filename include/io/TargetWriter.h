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
                   const std::wstring& prefix,
                   bool replace,
                   const std::vector<GatheredBlock>& blocks,
                   std::wstring& errorMessage) const;
    };
}
