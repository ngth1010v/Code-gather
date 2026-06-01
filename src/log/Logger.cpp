#include "log/Logger.h"

#include <windows.h>

#include <atomic> // Added for thread-safe flag
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace fs = std::filesystem;

namespace cgt::log
{
    namespace
    {
        std::mutex gMutex;
        bool gInitialized = false;
        fs::path gLogFile;
        std::atomic<bool> gLogEnable{false}; // Disabled by default, change to true if you want it on by default

        fs::path GetExeDirectory()
        {
            wchar_t buffer[MAX_PATH];

            DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
            if (len == 0)
            {
                return fs::current_path();
            }

            return fs::path(buffer).parent_path();
        }

        void Init()
        {
            if (gInitialized)
            {
                return;
            }

            gLogFile = GetExeDirectory() / L"cgt-log.txt";

            // remove old log
            std::error_code ec;
            fs::remove(gLogFile, ec);

            // create empty file
            std::wofstream file(gLogFile, std::ios::out);
            file.close();

            gInitialized = true;
        }

        void AppendLine(const std::wstring& line)
        {
            // Early exit if logging is disabled to avoid unnecessary mutex locking
            if (!gLogEnable.load())
            {
                return;
            }

            std::lock_guard<std::mutex> lock(gMutex);

            Init();

            std::wofstream file(gLogFile, std::ios::app);

            if (!file.is_open())
            {
                return;
            }

            file << line << L"\n";
        }

        const wchar_t* LevelTag(Level level)
        {
            switch (level)
            {
            case Level::Warning:
                return L"[WARNING]";

            case Level::Error:
                return L"[ERROR]";

            case Level::Info:
            default:
                return L"[INFO]";
            }
        }

        void Write(Level level,
                   const std::wstring& module,
                   const std::wstring& message)
        {
            // Early exit before string construction if logging is disabled
            if (!gLogEnable.load())
            {
                return;
            }

            std::wstring line =
                std::wstring(LevelTag(level))
                + L"[CODE-GATHER]["
                + module
                + L"] "
                + message;

            AppendLine(line);
        }
    }

    // Control methods
    void Logger::SetEnabled(bool enable)
    {
        gLogEnable.store(enable);
    }

    bool Logger::IsEnabled()
    {
        return gLogEnable.load();
    }

    void Logger::Info(const std::wstring& module,
                      const std::wstring& message)
    {
        Write(Level::Info, module, message);
    }

    void Logger::Warning(const std::wstring& module,
                         const std::wstring& message)
    {
        Write(Level::Warning, module, message);
    }

    void Logger::Error(const std::wstring& module,
                       const std::wstring& message)
    {
        Write(Level::Error, module, message);
    }

    void Logger::Plain(const std::wstring& message)
    {
        AppendLine(message);
    }
}