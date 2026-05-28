#include "app/CoreRunner.h"

#include "app/AppUtils.h"
#include "app/coreRunner/CoreConfirmer.h"
#include "app/coreRunner/CoreSelector.h"
#include "app/coreRunner/CoreUtils.h"
#include "cli/ParsedArgs.h"
#include "config/Config.h"
#include "io/TargetWriter.h"
#include "log/ConsolePrompt.h"
#include "log/Logger.h"
#include "scan/DiscoveryScanner.h"
#include "util/PathUtil.h"

#include <stdexcept>
#include <vector>

namespace cgt::app
{
    namespace
    {
        void PrintNoChangesAndClear()
        {
            cgt::app::coreRunner::ClearScreen();
            cgt::log::Logger::Plain(L"CoreGather: No changes were made.");
        }
    }

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
        const bool wrapped = !(args.HasFlag(L"nowrapped") || args.HasFlag(L"nw"));

        scan::DiscoveryScanner scanner(rootDir);
        std::vector<scan::DiscoveredFile> discovered = scanner.Scan();

        if (args.sourceArgs.empty() && discovered.empty())
        {
            PrintNoChangesAndClear();
            return 0;
        }

        cgt::app::coreRunner::CoreSelector selector;
        const std::vector<std::size_t> selectedIds = selector.Run(discovered, rootDir);
        if (selectedIds.empty())
        {
            PrintNoChangesAndClear();
            return 0;
        }

        cgt::app::coreRunner::CoreConfirmer confirmer;
        const cgt::app::coreRunner::CoreConfirmResult confirm = confirmer.Run(discovered,
                                                                              selectedIds,
                                                                              rootDir,
                                                                              state.outputToken,
                                                                              replace,
                                                                              wrapped);
        if (confirm.cancelled)
        {
            PrintNoChangesAndClear();
            return 0;
        }

        const fs::path outputPath = ResolveTarget(rootDir, confirm.outputToken);
        cgt::app::coreRunner::ClearScreen();
        cgt::log::Logger::Plain(L"Merging from " + std::to_wstring(selectedIds.size()) + L" files into file [" + cgt::util::RelativeDisplayPath(rootDir, outputPath) + L"]...");

        std::vector<io::GatheredBlock> blocks;
        bool hadAnyReadAttempt = false;
        try
        {
            blocks = BuildBlocks(rootDir, discovered, selectedIds, hadAnyReadAttempt);
        }
        catch (...)
        {
            cgt::app::coreRunner::ClearScreen();
            cgt::log::Logger::Plain(L"CoreGather: No changes were made.");
            return 0;
        }

        if (blocks.empty())
        {
            cgt::app::coreRunner::ClearScreen();
            cgt::log::Logger::Plain(L"CoreGather: No changes were made.");
            return 0;
        }

        io::TargetWriter writer;
        std::wstring errorMessage;
        while (true)
        {
            if (writer.Write(outputPath, confirm.wrapped, confirm.replace, blocks, errorMessage))
            {
                cgt::log::Logger::Plain(L"Successfully merged from " + std::to_wstring(selectedIds.size()) + L" files into file [" + cgt::util::RelativeDisplayPath(rootDir, outputPath) + L"].");
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
