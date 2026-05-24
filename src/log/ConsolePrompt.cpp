#include "log/ConsolePrompt.h"

#include "util/StringUtil.h"

#include <iostream>

namespace cgt::log
{
    std::wstring ConsolePrompt::AskSelection()
    {
        std::wcout << L"Select files by id list (Press Enter to stop): " << std::flush;
        std::wstring input;
        std::getline(std::wcin, input);
        return input;
    }

    ReadFailureChoice ConsolePrompt::AskReadFailure()
    {
        while (true)
        {
            std::wcout << L"File is blocked or missing. Choose: Retry / Skip / Cancel (default: Skip): " << std::flush;
            std::wstring input;
            std::getline(std::wcin, input);
            input = util::ToLower(util::Trim(input));

            if (input.empty() || input == L"skip" || input == L"s")
            {
                return ReadFailureChoice::Skip;
            }
            if (input == L"retry" || input == L"r")
            {
                return ReadFailureChoice::Retry;
            }
            if (input == L"cancel" || input == L"c")
            {
                return ReadFailureChoice::Cancel;
            }
        }
    }

    WriteFailureChoice ConsolePrompt::AskWriteFailure()
    {
        while (true)
        {
            std::wcout << L"Target file is blocked. Choose: Retry / Cancel (default: Retry): " << std::flush;
            std::wstring input;
            std::getline(std::wcin, input);
            input = util::ToLower(util::Trim(input));

            if (input.empty() || input == L"retry" || input == L"r")
            {
                return WriteFailureChoice::Retry;
            }
            if (input == L"cancel" || input == L"c")
            {
                return WriteFailureChoice::Cancel;
            }
        }
    }

    void ConsolePrompt::PrintHelp()
    {
        std::wcout << L"Usage:\n"
                << L"  <app_name> --output=<file> --filePrefix=<prefix> .<ext> <dir>\n\n"
                << L"Options:\n"
                << L"  --help          Show this help message.\n"
                << L"  --output        Path to the output .txt file.\n"
                << L"  --filePrefix    Prefix text for section markers.\n"
                << L"  .<ext>          Filter files by extension (e.g., .h .cpp).\n"
                << L"  <dir>           Target directory to scan.\n\n"
                << L"Output Format:\n"
                << L"  <filePrefix>START <path>\n"
                << L"  <...code...>\n"
                << L"  <filePrefix>END <path>\n" << std::endl;
    }
}
