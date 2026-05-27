#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace cgt::cli
{
    struct ParsedArgs;
}

namespace cgt::io
{
    struct GatheredBlock;
}

namespace cgt::scan
{
    struct DiscoveredFile;
}

namespace cgt::app
{
    namespace fs = std::filesystem;

    struct RuntimeState
    {
        std::wstring outputToken = L"cgt-result.txt";
        std::wstring filePrefix = L"**";
        std::vector<std::wstring> filters;
    };

    std::wstring NormalizeCliFilterRule(std::wstring rule);

    fs::path GetCallerDir();

    void WarnUnknownTokens(const cgt::cli::ParsedArgs& args);
    void ApplyConfigOverrides(const cgt::cli::ParsedArgs& args, RuntimeState& state);
    void ApplyFirstExistingTemplate(const cgt::cli::ParsedArgs& args, RuntimeState& state);
    void CollectCliFilters(const cgt::cli::ParsedArgs& args, RuntimeState& state);
    void SyncGlobalFilters(const std::vector<std::wstring>& filters);

    std::vector<std::size_t> ParseSelectionIds(const std::wstring& input,
                                              std::size_t maxCount,
                                              bool& hadAnyValid);

    std::vector<io::GatheredBlock> BuildBlocks(const fs::path& root,
                                               const std::vector<scan::DiscoveredFile>& discovered,
                                               const std::vector<std::size_t>& selectionIds,
                                               bool& hadAnyReadAttempt);

    fs::path ResolveTarget(const fs::path& root, const std::wstring& token);
}
