#pragma once

#include <filesystem>
#include <string>

namespace cgt::util
{
    namespace fs = std::filesystem;

    fs::path ResolveFromRoot(const fs::path& root, const fs::path& input);
    std::wstring NormalizeForCompare(const fs::path& path);
    std::wstring NormalizeForCompare(const std::wstring& text);
    std::wstring RelativeDisplayPath(const fs::path& root, const fs::path& absolutePath);
    bool IsPathInsideRoot(const fs::path& root, const fs::path& absolutePath);
    bool PathKeyStartsWith(const std::wstring& value, const std::wstring& prefix, bool folderMatch);
    std::wstring ExtensionKey(const fs::path& path);
}
