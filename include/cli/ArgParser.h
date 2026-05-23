#pragma once

#include "cli/ParsedArgs.h"

namespace cgt::cli
{
    class ArgParser
    {
    public:
        ParsedArgs Parse(int argc, wchar_t* argv[]) const;
    };
}
