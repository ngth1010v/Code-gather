#pragma once

#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace cgt::app::coreRunner
{
    struct SelectionParseState
    {
        std::vector<std::size_t> selectedIds;
        std::set<std::size_t> selectedSet;
        std::optional<std::size_t> activeId;
    };

    SelectionParseState ParseSelectionBuffer(const std::wstring& buffer,
                                             std::size_t cursor,
                                             std::size_t maxCount);

    class SelectionEditor
    {
    public:
        const std::wstring& Buffer() const;
        std::size_t Cursor() const;

        void SetBuffer(std::wstring value);
        void Clear();

        void InsertChar(wchar_t ch);
        void InsertText(std::wstring_view text);

        void Backspace();
        void Delete();
        void MoveLeft();
        void MoveRight();
        void MoveHome();
        void MoveEnd();

        void Step(int delta, std::size_t maxCount);

        SelectionParseState Parse(std::size_t maxCount) const;

    private:
        std::wstring m_buffer;
        std::size_t m_cursor = 0;
    };
}
