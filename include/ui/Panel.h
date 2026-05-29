#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "ui/Terminal.h"

namespace cgt::ui::panel
{
    using WindowSize = cgt::ui::teminal::WindowSize;
    using CursorPos  = cgt::ui::teminal::CursorPos;

    struct PanelSize
    {
        bool absolute = true;

        // Absolute size
        int w{0};
        int h{0};

        // Relative size
        float rw{0.0f};
        float rh{0.0f};
    };

    struct PanelPos
    {
        bool absolute = true;

        // Absolute pos
        int x{0};
        int y{0};

        // Relative pos
        float rx{0.0f};
        float ry{0.0f};
    };

    struct PanelLine
    {
        std::wstring text;
        RGB color{255, 255, 255};
        RGB bgColor{0, 0, 0};
    };

    class Panel
    {
    public:
        int Init();
        int Destroy();
        int Clean();

        int SetScroll(bool scrollable);

        int SetSize(PanelSize size);
        int SetPos(PanelPos pos);

        int GetSize(PanelSize& size);
        int GetPos(PanelPos& pos);
        int GetOffset(int& offset);

        int SetLine(int y, PanelLine line);
        int RemoveLine(int y);

        int GetLine(int y, PanelLine& line);
        int GetLines(std::vector<PanelLine>& lines);

        int MoveUp(int step);
        int MoveDown(int step);

        int OnResize(WindowSize& size);
        int OnMouseScroll(CursorPos& pos, int& delta);

    private:
        int CaptureTerminalSize(WindowSize& size) const;
        int RefreshRelativeFromAbsolute();
        int RefreshAbsoluteFromRelative();
        int ClampOffset();
        int ClearRegion(const PanelPos& pos, const PanelSize& size) const;
        int RenderViewport(bool forceAll);
        int RenderRow(int rowIndex, const PanelLine& newLine, const PanelLine* oldLine, bool forceFullWidth);
        bool IsInside(const CursorPos& pos) const;
        std::size_t GetVisibleWidth() const;
        std::size_t GetVisibleHeight() const;
        bool HasValidGeometry() const;

    private:
        WindowSize m_terminalSize{};
        bool m_initialized{false};
        bool m_scrollable{true};

        PanelSize m_size{};
        PanelPos m_pos{};

        int m_offset{0};

        std::vector<PanelLine> m_lines;

        std::vector<PanelLine> m_visibleCache;
        std::vector<char> m_visibleValid;

        bool m_hasDrawn{false};
        PanelPos m_lastDrawPos{};
        PanelSize m_lastDrawSize{};
    };
}
