#include "util/TextFilePolicy.h"

#include "util/PathUtil.h"
#include "util/StringUtil.h"

#include <fstream>
#include <set>
#include <vector>

namespace cgt::util
{

    static bool AllowedTextName(const fs::path& path)
    {
        static const std::set<std::wstring> allowedNames = {
            L"cmakelists.txt",
            L"makefile",
            L"readme",
            L"readme.md",
            L"readme.txt",
            L".gitignore",
            L".gitattributes",
            L".clang-format",
            L".clang-tidy",
            L".editorconfig",
            L".cgtignore"
        };

        const std::wstring filename = ToLower(path.filename().wstring());
        if (allowedNames.contains(filename))
        {
            return true;
        }

        const std::wstring extension = ExtensionKey(path);
        static const std::set<std::wstring> allowedExtensions = {
            L".c", L".cc", L".cpp", L".cxx", L".h", L".hh", L".hpp", L".hxx",
            L".inl", L".ixx", L".m", L".mm", L".txt", L".md", L".cmake",
            L".ini", L".cfg", L".json", L".xml", L".yaml", L".yml", L".toml",
            L".bat", L".cmd", L".ps1", L".sh", L".py", L".js", L".ts", L".tsx",
            L".css", L".html", L".htm", L".sql", L".log"
        };

        return allowedExtensions.contains(extension);
    }

    bool IsTextCandidate(const fs::path& path)
    {
        if (!fs::is_regular_file(path))
        {
            return false;
        }

        if (!AllowedTextName(path))
        {
            return false;
        }

        return true;
    }

    bool IsFilterToken(const std::wstring& token)
    {
        if (token.size() < 2 || token[0] != L'.')
        {
            return false;
        }

        if (token.find(L'\\') != std::wstring::npos || token.find(L'/') != std::wstring::npos)
        {
            return false;
        }

        return true;
    }

    bool MatchesFilters(const fs::path& path, const std::vector<std::wstring>& filters)
    {
        if (filters.empty())
        {
            return true;
        }

        const std::wstring ext = ToLower(path.extension().wstring());
        for (const std::wstring& filter : filters)
        {
            if (ToLower(filter) == ext)
            {
                return true;
            }
        }

        return false;
    }
}
