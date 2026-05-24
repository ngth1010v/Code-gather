#include "app/AppRunner.h"

#include "cli/ArgParser.h"
#include "cli/ParsedArgs.h"
#include "io/FileReader.h"
#include "io/TargetWriter.h"
#include "log/ConsolePrompt.h"
#include "log/Logger.h"
#include "log/PrintTree.h"
#include "scan/DiscoveryScanner.h"
#include "util/PathUtil.h"
#include "util/StringUtil.h"
#include "util/TextFilePolicy.h"
#include "config/Config.h"

#include <algorithm>
#include <filesystem>
#include <set>
#include <unordered_set>
#include <stdexcept>
#include <cwctype>
#include <iostream>
#include <windows.h>
#include <string>
#include <cmath>
#include <algorithm>

namespace cgt::app
{
    namespace fs = std::filesystem;

    static fs::path GetCallerDir()
    {
        wchar_t buffer[MAX_PATH * 4]{};
        DWORD len = GetCurrentDirectoryW(static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])), buffer);
        if (len == 0) return fs::current_path(); // Dự phòng nếu lỗi
        return fs::path(buffer, buffer + len);
    }

    static bool IsDirectPathToken(const std::wstring& token)
    {
        return !cgt::util::IsFilterToken(token);
    }

    static std::vector<std::wstring> ExtractFilters(const std::vector<std::wstring>& sourceArgs)
    {
        std::vector<std::wstring> filters;
        for (const auto& token : sourceArgs)
        {
            if (cgt::util::IsFilterToken(token))
            {
                filters.push_back(cgt::util::ToLower(token));
            }
        }
        return filters;
    }

    static std::vector<std::wstring> ExtractDirects(const std::vector<std::wstring>& sourceArgs)
    {
        std::vector<std::wstring> directs;
        for (const auto& token : sourceArgs)
        {
            if (!cgt::util::IsFilterToken(token))
            {
                directs.push_back(token);
            }
        }
        return directs;
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
            if (used.contains(key))
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

        try
        {
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
        }
        catch (...)
        {
            throw;
        }

        return blocks;
    }

    static fs::path ResolveTarget(const fs::path& root, const std::wstring& token)
    {
        fs::path input(token);
        return cgt::util::ResolveFromRoot(root, input);
    }

    int AppRunner::Run(int argc, wchar_t* argv[])
    {
        using namespace cgt;

        const fs::path rootDir = GetCallerDir();

        cgt::config::Init(rootDir);
        cgt::config::Parse();

        cli::ArgParser parser;
        const cli::ParsedArgs args = parser.Parse(argc, argv);

        if (!args.unknownTokens.empty())
        {
            for (const auto& token : args.unknownTokens)
            {
                log::Logger::Warning(L"ArgParser", L"Unknown token ignored: " + token);
            }
        }

        // Update config
        {
            const fs::path outputPath(args.GetFirstConfigValue(L"output", L"cgt-result.txt"));
            const std::wstring filePrefix = args.GetFirstConfigValue(L"fileprefix", L"**");      
            
            config::SetOutputFilePath(outputPath);
            config::SetFilePrefix(filePrefix);
        }
        
        const std::wstring outputToken = config::GetOutputFilePath().wstring();
        const std::wstring filePrefix  = config::GetFilePrefix();
        const bool replace = args.HasFlag(L"replace");
        const std::vector<std::wstring> filters = ExtractFilters(args.sourceArgs);
        const std::vector<std::wstring> directTokens = ExtractDirects(args.sourceArgs);

        const fs::path outputPath = ResolveTarget(rootDir, outputToken);
        std::vector<fs::path> directPaths;
        for (const auto& token : directTokens)
        {
            const fs::path resolved = cgt::util::ResolveFromRoot(rootDir, fs::path(token));
            directPaths.push_back(resolved.lexically_normal());
        }

        const bool onlyDirectSources = !directPaths.empty() && filters.empty();
        std::vector<scan::DiscoveredFile> discovered;

        if (!onlyDirectSources)
        {
            scan::DiscoveryScanner scanner(rootDir, filters);
            discovered = scanner.Scan();

            if (args.sourceArgs.empty() && discovered.empty())
            {
                log::Logger::Warning(L"Discovery", L"No readable text files were found.");
                return 0;
            }

            log::PrintTree::PrintFoundFilesTree(discovered, rootDir);

            if (discovered.empty() && directPaths.empty())
            {
                log::Logger::Warning(L"Discovery", L"No files matched the current filters or ignore rules.");
                return 0;
            }
        }

        std::vector<std::size_t> selectionIds;
        if (!onlyDirectSources)
        {
            if (!discovered.empty())
            {
                while (true)
                {
                    const std::wstring input = log::ConsolePrompt::AskSelection();
                    if (input.empty())
                    {
                        log::Logger::Warning(L"Selection", L"No valid selection was made.");
                        return 0;
                    }

                    bool hadAnyValid = false;
                    selectionIds = ParseSelectionIds(input, discovered.size(), hadAnyValid);
                    if (hadAnyValid && !selectionIds.empty())
                    {
                        break;
                    }

                    log::Logger::Warning(L"Selection", L"No valid selection was made.");
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
            log::Logger::Warning(L"FileReader", L"Run cancelled by user.");
            return 0;
        }

        if (blocks.empty())
        {
            log::Logger::Warning(L"Gather", L"No file matched the current filters / ignores / source constraints.");
            return 0;
        }

        io::TargetWriter writer;
        std::wstring errorMessage;
        while (true)
        {
            if (writer.Write(outputPath, filePrefix, replace, blocks, errorMessage))
            {
                log::Logger::Info(L"TargetWriter", L"Wrote output to " + cgt::util::RelativeDisplayPath(rootDir, outputPath));
                return 0;
            }

            log::Logger::Warning(L"TargetWriter", errorMessage);
            const auto choice = log::ConsolePrompt::AskWriteFailure();
            if (choice == log::WriteFailureChoice::Cancel)
            {
                return 0;
            }
        }
    }
}
