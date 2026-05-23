#pragma once

#include <string>

namespace cgt::log
{
    enum class ReadFailureChoice
    {
        Retry,
        Skip,
        Cancel
    };

    enum class WriteFailureChoice
    {
        Retry,
        Cancel
    };

    class ConsolePrompt
    {
    public:
        static std::wstring AskSelection();
        static ReadFailureChoice AskReadFailure();
        static WriteFailureChoice AskWriteFailure();
    };
}
