#include "app/coreRunner/CoreInput.h"

namespace cgt::app::coreRunner
{
    InputActionResult HandleSelectionInput(SelectionEditor& editor,
                                           const std::vector<wchar_t>& keys,
                                           const std::vector<cgt::ui::teminal::CmdKey>& cmdKeys,
                                           std::size_t maxCount)
    {
        InputActionResult result;

        for (wchar_t ch : keys)
        {
            if ((ch >= L'0' && ch <= L'9') || ch == L' ')
            {
                editor.InsertChar(ch);
                result.changed = true;
            }
        }

        for (const auto key : cmdKeys)
        {
            switch (key)
            {
                case cgt::ui::teminal::CmdKey::Escape:
                    result.cancel = true;
                    return result;

                case cgt::ui::teminal::CmdKey::Enter:
                    result.finish = true;
                    return result;

                case cgt::ui::teminal::CmdKey::Backspace:
                    editor.Backspace();
                    result.changed = true;
                    break;

                case cgt::ui::teminal::CmdKey::Delete:
                    editor.Delete();
                    result.changed = true;
                    break;

                case cgt::ui::teminal::CmdKey::Left:
                    editor.MoveLeft();
                    result.changed = true;
                    break;

                case cgt::ui::teminal::CmdKey::Right:
                    editor.MoveRight();
                    result.changed = true;
                    break;

                case cgt::ui::teminal::CmdKey::Home:
                    editor.MoveHome();
                    result.changed = true;
                    break;

                case cgt::ui::teminal::CmdKey::End:
                    editor.MoveEnd();
                    result.changed = true;
                    break;

                case cgt::ui::teminal::CmdKey::Up:
                    editor.Step(-1, maxCount);
                    result.changed = true;
                    break;

                case cgt::ui::teminal::CmdKey::Down:
                    editor.Step(+1, maxCount);
                    result.changed = true;
                    break;

                default:
                    break;
            }
        }

        return result;
    }
}
