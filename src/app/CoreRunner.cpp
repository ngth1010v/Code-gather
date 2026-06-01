#include "app/CoreRunner.h"

#include "app/AppUtils.h"
#include "app/coreRunner/CoreSelectionUi.h"
#include "cli/ParsedArgs.h"
#include "config/Config.h"
#include "io/TargetWriter.h"
#include "log/ConsolePrompt.h"
#include "log/Logger.h"
#include "scan/DiscoveryScanner.h"
#include "util/PathUtil.h"
#include "app/coreRunner/PrintCoreResult.h"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <iostream>

namespace cgt::app
{


    int CoreRunner::Run(const cgt::cli::ParsedArgs& args, const fs::path& rootDir)
    {
        WarnUnknownTokens(args);

        RuntimeState state;
        state.outputToken = cgt::config::GetOutputFilePath().wstring();

        ApplyFirstExistingTemplate(args, state);
        ApplyConfigOverrides(args, state);
        CollectCliFilters(args, state);

        cgt::config::Write();
        SyncGlobalFilters(state.filters);

        const bool replace = args.HasFlag(L"replace") || args.HasFlag(L"r");
        const bool wrapped = !(args.HasFlag(L"nowrap") || args.HasFlag(L"nowrapped") || args.HasFlag(L"nw"));

        scan::DiscoveryScanner scanner(rootDir);
        std::vector<scan::DiscoveredFile> discovered = scanner.Scan();

        if (discovered.empty())
        {
            return cgt::app::coreRunner::printResult::PrintDetectedNothing();
        }

        cgt::app::coreRunner::CoreSelectionUi selectionUi;
        const std::vector<std::size_t> selectedIds = selectionUi.Run(discovered, rootDir);
        if (selectedIds.empty())
        {
            return cgt::app::coreRunner::printResult::PrintNothing();
        }

        const fs::path outputPath = ResolveTarget(rootDir, state.outputToken);

        std::vector<io::GatheredBlock> blocks;
        bool hadAnyReadAttempt = false;
        try
        {
            blocks = BuildBlocks(rootDir, discovered, selectedIds, hadAnyReadAttempt);
        }
        catch (...)
        {
            return cgt::app::coreRunner::printResult::PrintNothing();
        }

        if (blocks.empty())
        {
            return cgt::app::coreRunner::printResult::PrintNothing();
        }

        io::TargetWriter writer;
        std::wstring errorMessage;
        while (true)
        {
            if (writer.Write(outputPath, wrapped, replace, blocks, errorMessage))
            {
                return cgt::app::coreRunner::printResult::PrintSuccess(
                    discovered,
                    selectedIds,
                    rootDir,
                    outputPath);
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
