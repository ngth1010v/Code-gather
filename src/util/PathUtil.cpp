#include "util/PathUtil.h"

#include "util/StringUtil.h"

#include <algorithm>

namespace cgt::util
{
    static std::wstring NormalizeSlashes(std::wstring text)
    {
        for (wchar_t& ch : text)
        {
            if (ch == L'\\')
            {
                ch = L'/';
            }
        }
        return text;
    }

    static std::wstring CanonicalKey(std::wstring text)
    {
        text = NormalizeSlashes(Trim(text));
        text = ToLower(text);

        while (text.size() > 1 && text.back() == L'/')
        {
            text.pop_back();
        }

        return text;
    }

    fs::path ResolveFromRoot(const fs::path& root, const fs::path& input)
    {
        if (input.is_absolute())
        {
            return input.lexically_normal();
        }

        return (root / input).lexically_normal();
    }

    std::wstring NormalizeForCompare(const fs::path& path)
    {
        return CanonicalKey(path.lexically_normal().generic_wstring());
    }

    std::wstring NormalizeForCompare(const std::wstring& text)
    {
        return CanonicalKey(text);
    }

    std::wstring RelativeDisplayPath(const fs::path& root, const fs::path& absolutePath)
    {
        const std::wstring rootCompare = NormalizeForCompare(root);
        const std::wstring pathCompare = NormalizeForCompare(absolutePath);

        const std::wstring rootDisplay = root.lexically_normal().generic_wstring();
        const std::wstring pathDisplay = absolutePath.lexically_normal().generic_wstring();

        if (pathCompare == rootCompare)
        {
            return L".";
        }

        if (PathKeyStartsWith(pathCompare, rootCompare, true))
        {
            if (pathDisplay.size() > rootDisplay.size() && pathDisplay.compare(0, rootDisplay.size(), rootDisplay) == 0)
            {
                if (pathDisplay[rootDisplay.size()] == L'/')
                {
                    return pathDisplay.substr(rootDisplay.size() + 1);
                }
            }
        }

        return pathDisplay;
    }

    bool IsPathInsideRoot(const fs::path& root, const fs::path& absolutePath)
    {
        const std::wstring rootKey = NormalizeForCompare(root);
        const std::wstring pathKey = NormalizeForCompare(absolutePath);

        if (pathKey == rootKey)
        {
            return true;
        }

        return PathKeyStartsWith(pathKey, rootKey, true);
    }

    bool PathKeyStartsWith(const std::wstring& value, const std::wstring& prefix, bool folderMatch)
    {
        if (prefix.empty() || value.size() < prefix.size())
        {
            return false;
        }

        if (value.compare(0, prefix.size(), prefix) != 0)
        {
            return false;
        }

        if (!folderMatch)
        {
            return true;
        }

        if (value.size() == prefix.size())
        {
            return true;
        }

        return value[prefix.size()] == L'/';
    }

    std::wstring ExtensionKey(const fs::path& path)
    {
        std::wstring ext = NormalizeForCompare(path.extension().wstring());
        if (ext.empty())
        {
            ext = L"<no-ext>";
        }
        return ext;
    }
}
