#pragma once

#include <string>

namespace cgt::log
{
    enum class Level
    {
        Info,
        Warning,
        Error
    };

    class Logger
    {
    public:
        static void Info(const std::wstring& module, const std::wstring& message);
        static void Warning(const std::wstring& module, const std::wstring& message);
        static void Error(const std::wstring& module, const std::wstring& message);
        static void Plain(const std::wstring& message);
    };
}
