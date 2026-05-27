#include "app/AppUtils.h"

#include "cli/ParsedArgs.h"
#include "config/Config.h"
#include "filter/Filter.h"
#include "io/FileReader.h"
#include "io/TargetWriter.h"
#include "log/ConsolePrompt.h"
#include "log/Logger.h"
#include "scan/DiscoveryScanner.h"
#include "util/PathUtil.h"
#include "util/StringUtil.h"

#include <algorithm>
#include <cwctype>
#include <set>
#include <stdexcept>
#include <windows.h>

namespace cgt::app
{
    namespace
    {
        static std::wstring Trimmed(std::wstring text)
        {
            return cgt::util::StripQuotes(cgt::util::Trim(text));
        }
    }

    std::wstring NormalizeCliFilterRule(std::wstring rule)
    {
        rule = Trimmed(std::move(rule));
        if (rule.empty())
        {
            return {};
        }

        if (rule.rfind(L"./", 0) == 0 || rule.rfind(L".\\", 0) == 0)
        {
            rule = rule.substr(2);
            return Trimmed(std::move(rule));
        }

        if (rule[0] == L'/' || rule[0] == L'\\')
        {
            rule = rule.substr(1);
            return Trimmed(std::move(rule));
        }

        if (rule.rfind(L"**", 0) == 0 || rule.rfind(L"*", 0) == 0)
        {
            return rule;
        }

        return L"**" + rule;
    }

    fs::path GetCallerDir()
    {
        wchar_t buffer[MAX_PATH * 4]{};
        DWORD len = GetCurrentDirectoryW(static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])), buffer);
        if (len == 0)
        {
            return fs::current_path();
        }
        return fs::path(buffer, buffer + len);
    }

    void WarnUnknownTokens(const cgt::cli::ParsedArgs& args)
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

    void ApplyConfigOverrides(const cgt::cli::ParsedArgs& args, RuntimeState& state)
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

    void ApplyFirstExistingTemplate(const cgt::cli::ParsedArgs& args, RuntimeState& state)
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
                state.filters = tl.filters;
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

    void CollectCliFilters(const cgt::cli::ParsedArgs& args, RuntimeState& state)
    {
        if (!args.GetFilters().empty())
        {
            state.filters.insert(state.filters.end(), args.GetFilters().begin(), args.GetFilters().end());
            return;
        }

        for (const auto& ext : args.GetExtFilters())
        {
            state.filters.push_back(NormalizeCliFilterRule(ext));
        }

        for (const auto& dir : args.GetDirFilters())
        {
            state.filters.push_back(NormalizeCliFilterRule(dir));
        }
    }

    void SyncGlobalFilters(const std::vector<std::wstring>& filters)
    {
        const auto existingFilters = cgt::filter::GetFilters();
        for (const auto& rule : existingFilters)
        {
            cgt::filter::RemoveFilter(rule);
        }

        for (const auto& rule : filters)
        {
            cgt::filter::AddFilter(rule);
        }
    }

    std::vector<std::size_t> ParseSelectionIds(const std::wstring& input,
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

    std::vector<io::GatheredBlock> BuildBlocks(const fs::path& root,
                                               const std::vector<scan::DiscoveredFile>& discovered,
                                               const std::vector<std::size_t>& selectionIds,
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

        return blocks;
    }

    fs::path ResolveTarget(const fs::path& root, const std::wstring& token)
    {
        fs::path input(token);
        return cgt::util::ResolveFromRoot(root, input);
    }
}
