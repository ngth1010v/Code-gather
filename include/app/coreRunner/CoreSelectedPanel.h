#pragma once

#include "app/coreRunner/TreeBuilder.h"
#include "ui/Panel.h"

namespace cgt::app::coreRunner
{
    class CoreSelectedPanel
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

        int Render(const TreeNode& root, bool forceReprint);

    private:
        cgt::ui::panel::Panel m_panel;
    };
}
