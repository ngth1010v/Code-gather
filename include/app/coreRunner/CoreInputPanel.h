#pragma once

#include "app/coreRunner/CoreSelectionInput.h"
#include "ui/Panel.h"

#include <cstddef>

namespace cgt::app::coreRunner
{
    class CoreInputPanel
    {
    public:
        int Init();
        int Destroy();

        int SetScroll(bool scrollable);
        int SetSize(cgt::ui::panel::PanelSize size);
        int SetPos(cgt::ui::panel::PanelPos pos);
        int GetSize(cgt::ui::panel::PanelSize& size);

        int OnResize(cgt::ui::teminal::WindowSize& size);
        int OnMouseScroll(cgt::ui::teminal::CursorPos& pos, int& delta);

        std::size_t RequiredHeight(std::size_t width, const SelectionEditor& editor) const;
        int Render(const SelectionEditor& editor, bool forceReprint);

    private:
        cgt::ui::panel::Panel m_panel;
    };
}
