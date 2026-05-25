#include "app/AppRunner.h"

#include "cli/ArgParser.h"
#include "cli/ParsedArgs.h"
#include "config/Config.h"
#include "io/FileReader.h"
#include "io/TargetWriter.h"
#include "log/ConsolePrompt.h"
#include "log/Logger.h"
#include "log/PrintTree.h"
#include "scan/DiscoveryScanner.h"
#include "util/PathUtil.h"
#include "util/StringUtil.h"
#include "util/TextFilePolicy.h"

#include <algorithm>
#include <cmath>
#include <cwctype>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <windows.h>

namespace cgt::app
{
    namespace fs = std::filesystem;

    namespace
    {
        struct RuntimeState
        {
            std::wstring outputToken;
            std::wstring filePrefix;
            std::vector<std::wstring> extFilters;
            std::vector<std::wstring> dirFilters;
        };

        static fs::path GetCallerDir()
        {
            wchar_t buffer[MAX_PATH * 4]{};
            DWORD len = GetCurrentDirectoryW(static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])), buffer);
            if (len == 0)
            {
                return fs::current_path();
            }
            return fs::path(buffer, buffer + len);
        }

        static std::wstring NormalizeExtKey(const std::wstring& ext)
        {
            return cgt::util::ToLower(cgt::util::Trim(cgt::util::StripQuotes(ext)));
        }

        static std::wstring NormalizeDirKey(const std::wstring& dir)
        {
            return cgt::util::NormalizeForCompare(fs::path(cgt::util::StripQuotes(cgt::util::Trim(dir))));
        }

        static void AppendUniqueExt(std::vector<std::wstring>& dst, const std::wstring& ext)
        {
            const std::wstring value = cgt::util::StripQuotes(cgt::util::Trim(ext));
            const std::wstring key = NormalizeExtKey(value);

            if (key.empty())
            {
                return;
            }

            for (const auto& item : dst)
            {
                if (NormalizeExtKey(item) == key)
                {
                    return;
                }
            }

            dst.push_back(value);
        }

        static void AppendUniqueDir(std::vector<std::wstring>& dst, const std::wstring& dir)
        {
            const std::wstring value = cgt::util::StripQuotes(cgt::util::Trim(dir));
            const std::wstring key = NormalizeDirKey(value);

            if (key.empty())
            {
                return;
            }

            for (const auto& item : dst)
            {
                if (NormalizeDirKey(item) == key)
                {
                    return;
                }
            }

            dst.push_back(value);
        }

        static void MergeCliFilters(const cgt::cli::ParsedArgs& args, RuntimeState& state)
        {
            for (const auto& ext : args.GetExtFilters())
            {
                AppendUniqueExt(state.extFilters, ext);
            }

            for (const auto& dir : args.GetDirFilters())
            {
                AppendUniqueDir(state.dirFilters, dir);
            }
        }

        static void ApplyConfigOverrides(const cgt::cli::ParsedArgs& args, RuntimeState& state)
        {
            const std::wstring outputToken = args.GetFirstConfigValue(L"output", L"");
            if (!outputToken.empty())
            {
                state.outputToken = outputToken;
                cgt::config::SetOutputFilePath(fs::path(outputToken));
            }

            const std::wstring filePrefix = args.GetFirstConfigValue(L"fileprefix", L"");
            if (!filePrefix.empty())
            {
                state.filePrefix = filePrefix;
                cgt::config::SetFilePrefix(filePrefix);
            }
        }

        static void ApplyFirstExistingTemplate(const cgt::cli::ParsedArgs& args, RuntimeState& state)
        {
            const auto& templateNames = args.GetTemplateNames();
            if (templateNames.empty())
            {
                return;
            }

            std::wstring chosenTemplate;
            for (const auto& name : templateNames)
            {
                if (!cgt::config::HasTemplate(name))
                {
                    cgt::log::Logger::Warning(L"Template", L"Template not found, skipped: " + name);
                    continue;
                }

                if (chosenTemplate.empty())
                {
                    chosenTemplate = name;
                    const cgt::config::CgtTemplate tl = cgt::config::GetTemplate(name);
                    state.outputToken = tl.output;
                    state.filePrefix = tl.filePrefix;
                    state.extFilters = tl.extFilters;
                    state.dirFilters = tl.dirFilters;
                }
            }

            if (chosenTemplate.empty())
            {
                cgt::log::Logger::Warning(L"Template", L"No existing template was selected.");
                return;
            }

            if (templateNames.size() > 1)
            {
                cgt::log::Logger::Warning(
                    L"Template",
                    L"Multiple templates were provided; only the first existing template was applied: " + chosenTemplate
                );
            }
        }

        static std::vector<std::size_t> ParseSelectionIds(const std::wstring& input,
                                                          std::size_t maxCount,
                                                          bool& hadAnyValid)
        {
            std::vector<std::size_t> selected;
            hadAnyValid = false;

            const auto tokens = cgt::util::SplitTokens(input);
            std::set<std::size_t> unique;
            for (const auto& token : tokens)
            {
                if (token.empty())
                {
                    continue;
                }

                bool numeric = true;
                for (wchar_t ch : token)
                {
                    if (!std::iswdigit(ch))
                    {
                        numeric = false;
                        break;
                    }
                }

                if (!numeric)
                {
                    cgt::log::Logger::Warning(L"Selection", L"Rejected invalid selection token: " + token);
                    continue;
                }

                std::size_t id = 0;
                try
                {
                    id = static_cast<std::size_t>(std::stoull(token));
                }
                catch (...)
                {
                    cgt::log::Logger::Warning(L"Selection", L"Rejected invalid selection token: " + token);
                    continue;
                }

                if (id == 0 || id > maxCount)
                {
                    cgt::log::Logger::Warning(L"Selection", L"Ignored out-of-range id: " + token);
                    continue;
                }

                unique.insert(id);
                hadAnyValid = true;
            }

            selected.assign(unique.begin(), unique.end());
            return selected;
        }

        static std::vector<io::GatheredBlock> BuildBlocks(const fs::path& root,
                                                          const std::vector<scan::DiscoveredFile>& discovered,
                                                          const std::vector<std::size_t>& selectionIds,
                                                          const std::vector<fs::path>& directPaths,
                                                          bool& hadAnyReadAttempt)
        {
            std::vector<io::GatheredBlock> blocks;
            io::FileReader reader;
            hadAnyReadAttempt = false;

            std::set<std::wstring> used;

            auto addPath = [&](const fs::path& path)
            {
                const std::wstring key = cgt::util::NormalizeForCompare(path);
                if (used.find(key) != used.end())
                {
                    return;
                }

                hadAnyReadAttempt = true;

                while (true)
                {
                    const io::ReadResult result = reader.Read(path);
                    if (result.success)
                    {
                        io::GatheredBlock block;
                        block.relativePath = cgt::util::RelativeDisplayPath(root, path);
                        block.content = result.content;
                        blocks.push_back(std::move(block));
                        used.insert(key);
                        return;
                    }

                    cgt::log::Logger::Warning(L"FileReader", L"Failed to read: " + cgt::util::RelativeDisplayPath(root, path));
                    const auto choice = cgt::log::ConsolePrompt::AskReadFailure();
                    if (choice == cgt::log::ReadFailureChoice::Retry)
                    {
                        continue;
                    }
                    if (choice == cgt::log::ReadFailureChoice::Skip)
                    {
                        return;
                    }
                    throw std::runtime_error("cancelled");
                }
            };

            for (std::size_t idx : selectionIds)
            {
                if (idx == 0 || idx > discovered.size())
                {
                    continue;
                }
                addPath(discovered[idx - 1].absolutePath);
            }

            for (const auto& direct : directPaths)
            {
                addPath(direct);
            }

            return blocks;
        }

        static fs::path ResolveTarget(const fs::path& root, const std::wstring& token)
        {
            fs::path input(token);
            return cgt::util::ResolveFromRoot(root, input);
        }

        static void WarnUnknownTokens(const cgt::cli::ParsedArgs& args)
        {
            if (args.unknownTokens.empty())
            {
                return;
            }

            for (const auto& token : args.unknownTokens)
            {
                cgt::log::Logger::Warning(L"ArgParser", L"Unknown token ignored: " + token);
            }
        }

        static int RunHelp()
        {
            cgt::log::ConsolePrompt::PrintHelp();
            return 0;
        }

        static int RunSetTemplate(const cgt::cli::ParsedArgs& args)
        {
            const auto& names = args.GetTemplateNames();
            if (names.empty() || names.front().empty())
            {
                cgt::log::Logger::Warning(L"Template", L"Missing template name for setTemplate.");
                return 0;
            }

            RuntimeState state;
            state.outputToken = cgt::config::GetOutputFilePath().wstring();
            state.filePrefix = cgt::config::GetFilePrefix();

            ApplyConfigOverrides(args, state);
            MergeCliFilters(args, state);

            cgt::config::CgtTemplate tl;
            tl.output = state.outputToken;
            tl.filePrefix = state.filePrefix;
            tl.extFilters = state.extFilters;
            tl.dirFilters = state.dirFilters;

            cgt::config::SetTemplate(names.front(), tl);
            cgt::config::Write();
            return 0;
        }

        static int RunRemoveTemplate(const cgt::cli::ParsedArgs& args)
        {
            if (args.HasFlag(L"all"))
            {
                const std::vector<std::wstring> names = cgt::config::GetTemplateNames();
                for (const auto& name : names)
                {
                    if (!name.empty())
                    {
                        cgt::config::RemoveTemplate(name);
                    }
                }

                cgt::config::Write();
                return 0;
            }

            const auto& names = args.GetTemplateNames();
            if (names.empty() || names.front().empty())
            {
                cgt::log::Logger::Warning(L"Template", L"Missing template name for rmTemplate.");
                return 0;
            }

            cgt::config::RemoveTemplate(names.front());
            cgt::config::Write();
            return 0;
        }

        static int RunCore(const cgt::cli::ParsedArgs& args, const fs::path& rootDir)
        {
            RuntimeState state;
            state.outputToken = cgt::config::GetOutputFilePath().wstring();
            state.filePrefix = cgt::config::GetFilePrefix();

            ApplyFirstExistingTemplate(args, state);
            ApplyConfigOverrides(args, state);
            MergeCliFilters(args, state);

            cgt::config::Write();

            const bool replace = args.HasFlag(L"replace");

            const fs::path outputPath = ResolveTarget(rootDir, state.outputToken);
            const std::wstring filePrefix = state.filePrefix;
            const std::vector<std::wstring> extFilters = state.extFilters;
            const std::vector<std::wstring> dirFilters = state.dirFilters;

            std::vector<fs::path> directPaths;
            for (const auto& token : dirFilters)
            {
                const fs::path resolved = cgt::util::ResolveFromRoot(rootDir, fs::path(token));
                fs::path normalPath = resolved.lexically_normal();

                if (fs::exists(normalPath) && !fs::is_directory(normalPath))
                {
                    directPaths.push_back(normalPath);
                }
            }

            const bool onlyDirectPaths = !directPaths.empty() && extFilters.empty() && (dirFilters.size() == directPaths.size());
            std::vector<scan::DiscoveredFile> discovered;

            if (!onlyDirectPaths)
            {
                scan::DiscoveryScanner scanner(rootDir, extFilters, dirFilters);
                discovered = scanner.Scan();

                if (args.sourceArgs.empty() && discovered.empty())
                {
                    cgt::log::Logger::Warning(L"Discovery", L"No readable text files were found.");
                    return 0;
                }

                cgt::log::PrintTree::PrintFoundFilesTree(discovered, rootDir);

                if (discovered.empty() && directPaths.empty())
                {
                    cgt::log::Logger::Warning(L"Discovery", L"No files matched the current filters or ignore rules.");
                    return 0;
                }
            }

            std::vector<std::size_t> selectionIds;
            if (!onlyDirectPaths)
            {
                if (!discovered.empty())
                {
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
                }
            }

            std::vector<io::GatheredBlock> blocks;
            bool hadAnyReadAttempt = false;
            try
            {
                blocks = BuildBlocks(rootDir, discovered, selectionIds, directPaths, hadAnyReadAttempt);
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

    int AppRunner::Run(int argc, wchar_t* argv[])
    {
        using namespace cgt;

        const fs::path rootDir = GetCallerDir();

        config::Init(rootDir);
        config::Parse();

        cli::ArgParser parser;
        const cli::ParsedArgs args = parser.Parse(argc, argv);

        if (args.mode == cli::ParsedArgs::Mode::Help)
        {
            return RunHelp();
        }

        WarnUnknownTokens(args);

        switch (args.mode)
        {
        case cli::ParsedArgs::Mode::SetTemplate:
            return RunSetTemplate(args);

        case cli::ParsedArgs::Mode::RemoveTemplate:
            return RunRemoveTemplate(args);

        case cli::ParsedArgs::Mode::Core:
        default:
            return RunCore(args, rootDir);
        }
    }
}