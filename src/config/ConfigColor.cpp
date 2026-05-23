#include "config/ConfigState.h"

#include "config/Defaults.h"

namespace cgt::config
{
    RGB GetExtColor(std::wstring ext)
    {
        auto& state = detail::State();
        if (!state.initialized)
        {
            detail::LogWarn(L"GetExtColor() called before Init(). Using transient random color.");
            return detail::RandomBrightRGB();
        }

        ext = detail::NormalizeExt(std::move(ext));
        if (ext.empty())
        {
            return detail::RandomBrightRGB();
        }

        auto it = state.extColors.find(ext);
        if (it != state.extColors.end())
        {
            return it->second;
        }

        auto rgb = detail::RandomBrightRGB();
        state.extColors[ext] = rgb;
        if (Write() != kStatusOk)
        {
            detail::LogWarn(L"Color was generated but failed to persist.");
        }
        return rgb;
    }
}
