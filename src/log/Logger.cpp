#include "log/Logger.h"

#include <windows.h>
#include <iostream>

namespace cgt::log
{
    static HANDLE OutHandle()
    {
        return GetStdHandle(STD_OUTPUT_HANDLE);
    }

    static WORD ColorFor(Level level)
    {
        switch (level)
        {
        case Level::Warning:
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case Level::Error:
            return FOREGROUND_RED | FOREGROUND_INTENSITY;
        case Level::Info:
        default:
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
    }

    static void SetColor(Level level)
    {
        SetConsoleTextAttribute(OutHandle(), ColorFor(level));
    }

    static void ResetColor()
    {
        SetConsoleTextAttribute(OutHandle(), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }

    static void Write(Level level, const std::wstring& module, const std::wstring& message)
    {
        SetColor(level);
        const wchar_t* tag = level == Level::Warning ? L"[WARNING]" : level == Level::Error ? L"[ERROR]" : L"[INFO]";
        std::wcout << tag << L"[CODE-GATHER][" << module << L"] " << message << std::endl;
        ResetColor();
    }

    void Logger::Info(const std::wstring& module, const std::wstring& message)
    {
        Write(Level::Info, module, message);
    }

    void Logger::Warning(const std::wstring& module, const std::wstring& message)
    {
        Write(Level::Warning, module, message);
    }

    void Logger::Error(const std::wstring& module, const std::wstring& message)
    {
        Write(Level::Error, module, message);
    }

    void Logger::Plain(const std::wstring& message)
    {
        ResetColor();
        std::wcout << message << std::endl;
    }
}
