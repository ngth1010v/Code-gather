#include "app/AppRunner.h"
#include "app/AppUtils.h"

#include "app/CoreRunner.h"
#include "app/TemplateRunner.h"
#include "cli/ArgParser.h"
#include "cli/ParsedArgs.h"
#include "config/Config.h"
#include "log/ConsolePrompt.h"

#include <filesystem>

namespace cgt::app
{
    namespace fs = std::filesystem;

    int AppRunner::Run(int argc, wchar_t* argv[])
    {
        const fs::path rootDir = GetCallerDir();

        cgt::config::Init(rootDir);
        cgt::config::Parse();

        cgt::cli::ArgParser parser;
        const cgt::cli::ParsedArgs args = parser.Parse(argc, argv);

        if (args.mode == cgt::cli::ParsedArgs::Mode::Help)
        {
            cgt::log::ConsolePrompt::PrintHelp();
            return 0;
        }

        switch (args.mode)
        {
        case cgt::cli::ParsedArgs::Mode::SetTemplate:
        {
            TemplateRunner runner;
            return runner.RunSetTemplate(args);
        }

        case cgt::cli::ParsedArgs::Mode::RemoveTemplate:
        {
            TemplateRunner runner;
            return runner.RunRemoveTemplate(args);
        }

        case cgt::cli::ParsedArgs::Mode::Core:
        default:
        {
            CoreRunner runner;
            return runner.Run(args, rootDir);
        }
        }
    }
}
