#pragma once

#include <filesystem>

namespace cgt::cli
{
    struct ParsedArgs;
}

namespace cgt::app
{
    class CoreRunner
    {
    public:
        int Run(const cgt::cli::ParsedArgs& args, const std::filesystem::path& rootDir);
    };
}
