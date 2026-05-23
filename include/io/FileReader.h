#pragma once

#include <filesystem>
#include <string>

namespace cgt::io
{
    namespace fs = std::filesystem;

    struct ReadResult
    {
        bool success = false;
        std::wstring content;
        std::wstring errorMessage;
    };

    class FileReader
    {
    public:
        ReadResult Read(const fs::path& path) const;
    };
}
