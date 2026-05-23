#pragma once

#include <string>
#include <vector>

namespace ctg::config::detail
{
    std::vector<std::wstring> SplitSegments(const std::wstring& rel);
    bool MatchDirRule(const std::vector<std::wstring>& segs, const std::wstring& rule);
    bool MatchFileRule(const std::vector<std::wstring>& segs, const std::wstring& rule);
    bool MatchRule(const std::wstring& rel, bool isDir, const std::wstring& rule);
}
