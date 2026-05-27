#include "app/CoreRunner.h"

#include "app/AppUtils.h"
#include "cli/ParsedArgs.h"
#include "config/Config.h"
#include "io/TargetWriter.h"
#include "log/ConsolePrompt.h"
#include "log/Logger.h"
#include "log/PrintTree.h"
#include "scan/DiscoveryScanner.h"
#include "util/PathUtil.h"

#include <stdexcept>
#include <vector>

namespace cgt::app
{
    int CoreRunner::Run(const cgt::cli::ParsedArgs& args, const fs::path& rootDir)
    {
        WarnUnknownTokens(args);

        RuntimeState state;
        state.outputToken = cgt::config::GetOutputFilePath().wstring();
        state.filePrefix = cgt::config::GetFilePrefix();

        ApplyFirstExistingTemplate(args, state);
        ApplyConfigOverrides(args, state);
        CollectCliFilters(args, state);

        cgt::config::Write();
        SyncGlobalFilters(state.filters);

        const bool replace = args.HasFlag(L"replace");

        const fs::path outputPath = ResolveTarget(rootDir, state.outputToken);
        const std::wstring filePrefix = state.filePrefix;

        scan::DiscoveryScanner scanner(rootDir);
        std::vector<scan::DiscoveredFile> discovered = scanner.Scan();

        if (args.sourceArgs.empty() && discovered.empty())
        {
            cgt::log::Logger::Warning(L"Discovery", L"No readable text files were found.");
            return 0;
        }

        cgt::log::PrintTree::PrintFoundFilesTree(discovered, rootDir);

        if (discovered.empty())
        {
            cgt::log::Logger::Warning(L"Discovery", L"No files matched the current filters or ignore rules.");
            return 0;
        }

        std::vector<std::size_t> selectionIds;
        while (true)
        {
            const std::wstring input = cgt::log::ConsolePrompt::AskSelection();
            if (input.empty())
            {
                cgt::log::Logger::Warning(L"Selection", L"No valid selection was made.");
                return 0;
            }

            bool hadAnyValid = false;
            selectionIds = ParseSelectionIds(input, discovered.size(), hadAnyValid);
            if (hadAnyValid && !selectionIds.empty())
            {
                break;
            }

            cgt::log::Logger::Warning(L"Selection", L"No valid selection was made.");
        }

        std::vector<io::GatheredBlock> blocks;
        bool hadAnyReadAttempt = false;
        try
        {
            blocks = BuildBlocks(rootDir, discovered, selectionIds, hadAnyReadAttempt);
        }
        catch (...)
        {
            cgt::log::Logger::Warning(L"FileReader", L"Run cancelled by user.");
            return 0;
        }

        if (blocks.empty())
        {
            cgt::log::Logger::Warning(L"Gather", L"No file matched the current filters / ignores / source constraints.");
            return 0;
        }

        io::TargetWriter writer;
        std::wstring errorMessage;
        while (true)
        {
            if (writer.Write(outputPath, filePrefix, replace, blocks, errorMessage))
            {
                cgt::log::Logger::Info(L"TargetWriter", L"Wrote output to " + cgt::util::RelativeDisplayPath(rootDir, outputPath));
                return 0;
            }

            cgt::log::Logger::Warning(L"TargetWriter", errorMessage);
            const auto choice = cgt::log::ConsolePrompt::AskWriteFailure();
            if (choice == cgt::log::WriteFailureChoice::Cancel)
            {
                return 0;
            }
        }
    }
}
