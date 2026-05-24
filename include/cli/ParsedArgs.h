#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

namespace cgt::cli
{
    struct ParsedArgs
    {
        std::vector<std::wstring> sourceArgs;
        std::vector<std::wstring> extFilters;  // Lưu trữ các định dạng file (.txt, .cpp...)
        std::vector<std::wstring> dirFilters;  // Lưu trữ đường dẫn thư mục
        std::vector<std::wstring> unknownTokens;
        std::set<std::wstring> flags;
        std::map<std::wstring, std::vector<std::wstring>> configValues;

        bool HasFlag(const std::wstring& flag) const;
        std::wstring GetFirstConfigValue(const std::wstring& key, const std::wstring& defaultValue = L"") const;
        std::vector<std::wstring> GetConfigValues(const std::wstring& key) const;
        
        // Hai hàm mới theo yêu cầu
        std::vector<std::wstring> GetExtFilters() const { return extFilters; }
        std::vector<std::wstring> GetDirFilters() const { return dirFilters; }
    };
}