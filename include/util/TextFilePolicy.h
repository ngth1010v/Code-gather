#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace cgt::util
{
    namespace fs = std::filesystem;

    bool IsTextCandidate(const fs::path& path);
    bool MatchesFilters(const fs::path& path, const std::vector<std::wstring>& filters);
    bool IsFilterToken(const std::wstring& token);
}
