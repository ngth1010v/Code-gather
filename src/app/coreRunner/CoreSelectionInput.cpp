#include "app/coreRunner/CoreSelectionInput.h"

#include <algorithm>
#include <cwctype>
#include <stdexcept>

namespace cgt::app::coreRunner
{
    namespace
    {
        struct TokenInfo
        {
            std::size_t start = 0;
            std::size_t end = 0;
            std::size_t value = 0;
            bool valid = false;
        };

        bool IsSpace(wchar_t ch)
        {
            return std::iswspace(ch) != 0;
        }

        std::vector<TokenInfo> ParseTokens(const std::wstring& buffer, std::size_t maxCount)
        {
            std::vector<TokenInfo> tokens;
            std::size_t i = 0;

            while (i < buffer.size())
            {
                while (i < buffer.size() && IsSpace(buffer[i]))
                {
                    ++i;
                }

                const std::size_t begin = i;
                while (i < buffer.size() && !IsSpace(buffer[i]))
                {
                    ++i;
                }

                const std::size_t end = i;
                if (begin >= end)
                {
                    continue;
                }

                TokenInfo token;
                token.start = begin;
                token.end = end;

                bool numeric = true;
                for (std::size_t j = begin; j < end; ++j)
                {
                    if (!std::iswdigit(buffer[j]))
                    {
                        numeric = false;
                        break;
                    }
                }

                if (numeric)
                {
                    try
                    {
                        token.value = static_cast<std::size_t>(std::stoull(buffer.substr(begin, end - begin)));
                        token.valid = token.value >= 1 && token.value <= maxCount;
                    }
                    catch (...)
                    {
                        token.valid = false;
                    }
                }

                tokens.push_back(token);
            }

            return tokens;
        }

        SelectionParseState BuildParseState(const std::wstring& buffer,
                                           std::size_t cursor,
                                           std::size_t maxCount)
        {
            SelectionParseState state;
            cursor = std::min(cursor, buffer.size());

            const auto tokens = ParseTokens(buffer, maxCount);
            for (const auto& token : tokens)
            {
                if (cursor >= token.start && cursor <= token.end && token.valid)
                {
                    state.activeId = token.value;
                    break;
                }
            }

            std::set<std::size_t> seen;
            for (const auto& token : tokens)
            {
                if (!token.valid)
                {
                    continue;
                }

                if (seen.insert(token.value).second)
                {
                    state.selectedIds.push_back(token.value);
                    state.selectedSet.insert(token.value);
                }
            }

            return state;
        }

        std::optional<TokenInfo> FindActiveToken(const std::wstring& buffer,
                                                 std::size_t cursor,
                                                 std::size_t maxCount)
        {
            const auto tokens = ParseTokens(buffer, maxCount);
            cursor = std::min(cursor, buffer.size());

            for (const auto& token : tokens)
            {
                if (cursor >= token.start && cursor <= token.end && token.valid)
                {
                    return token;
                }
            }

            return std::nullopt;
        }

        std::optional<TokenInfo> FindLastValidToken(const std::wstring& buffer, std::size_t maxCount)
        {
            const auto tokens = ParseTokens(buffer, maxCount);
            for (auto it = tokens.rbegin(); it != tokens.rend(); ++it)
            {
                if (it->valid)
                {
                    return *it;
                }
            }
            return std::nullopt;
        }

        void ReplaceToken(std::wstring& buffer,
                          std::size_t& cursor,
                          const TokenInfo& token,
                          std::size_t value)
        {
            const std::wstring replacement = std::to_wstring(value);
            buffer.replace(token.start, token.end - token.start, replacement);
            cursor = token.start + replacement.size();
        }

        void InsertTextImpl(std::wstring& buffer, std::size_t& cursor, std::wstring_view text)
        {
            cursor = std::min(cursor, buffer.size());
            buffer.insert(cursor, text);
            cursor += text.size();
        }

        void EraseBeforeCursorImpl(std::wstring& buffer, std::size_t& cursor)
        {
            if (cursor == 0 || buffer.empty())
            {
                return;
            }

            --cursor;
            buffer.erase(cursor, 1);
        }

        void EraseAtCursorImpl(std::wstring& buffer, std::size_t& cursor)
        {
            if (cursor >= buffer.size())
            {
                return;
            }

            buffer.erase(cursor, 1);
        }
    }

    SelectionParseState ParseSelectionBuffer(const std::wstring& buffer,
                                             std::size_t cursor,
                                             std::size_t maxCount)
    {
        return BuildParseState(buffer, cursor, maxCount);
    }

    const std::wstring& SelectionEditor::Buffer() const
    {
        return m_buffer;
    }

    std::size_t SelectionEditor::Cursor() const
    {
        return m_cursor;
    }

    void SelectionEditor::SetBuffer(std::wstring value)
    {
        m_buffer = std::move(value);
        if (m_cursor > m_buffer.size())
        {
            m_cursor = m_buffer.size();
        }
    }

    void SelectionEditor::Clear()
    {
        m_buffer.clear();
        m_cursor = 0;
    }

    void SelectionEditor::InsertChar(wchar_t ch)
    {
        if (ch >= L'0' && ch <= L'9')
        {
            InsertTextImpl(m_buffer, m_cursor, std::wstring_view(&ch, 1));
            return;
        }

        if (ch == L' ')
        {
            InsertTextImpl(m_buffer, m_cursor, std::wstring_view(L" ", 1));
        }
    }

    void SelectionEditor::InsertText(std::wstring_view text)
    {
        InsertTextImpl(m_buffer, m_cursor, text);
    }

    void SelectionEditor::Backspace()
    {
        EraseBeforeCursorImpl(m_buffer, m_cursor);
    }

    void SelectionEditor::Delete()
    {
        EraseAtCursorImpl(m_buffer, m_cursor);
    }

    void SelectionEditor::MoveLeft()
    {
        if (m_cursor > 0)
        {
            --m_cursor;
        }
    }

    void SelectionEditor::MoveRight()
    {
        if (m_cursor < m_buffer.size())
        {
            ++m_cursor;
        }
    }

    void SelectionEditor::MoveHome()
    {
        m_cursor = 0;
    }

    void SelectionEditor::MoveEnd()
    {
        m_cursor = m_buffer.size();
    }

    void SelectionEditor::Step(int delta, std::size_t maxCount)
    {
        if (maxCount == 0)
        {
            return;
        }

        std::size_t target = 1;
        const auto active = FindActiveToken(m_buffer, m_cursor, maxCount);
        if (active.has_value())
        {
            target = active->value;
        }
        else
        {
            const auto lastValid = FindLastValidToken(m_buffer, maxCount);
            if (lastValid.has_value())
            {
                target = lastValid->value;
            }
        }

        if (delta < 0)
        {
            target = target > 1 ? target - 1 : 1;
        }
        else if (delta > 0)
        {
            target = target < maxCount ? target + 1 : maxCount;
        }

        if (active.has_value())
        {
            ReplaceToken(m_buffer, m_cursor, *active, target);
            return;
        }

        const auto lastValid = FindLastValidToken(m_buffer, maxCount);
        if (lastValid.has_value())
        {
            ReplaceToken(m_buffer, m_cursor, *lastValid, target);
            return;
        }

        InsertTextImpl(m_buffer, m_cursor, std::to_wstring(target));
    }

    SelectionParseState SelectionEditor::Parse(std::size_t maxCount) const
    {
        return BuildParseState(m_buffer, m_cursor, maxCount);
    }
}
