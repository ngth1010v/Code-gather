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
        std::wcout
            << L"CodeGather\n"
            << L"Simple and fast CLI tool for collecting source files into one .txt output.\n\n"

            << L"USAGE\n"
            << L"  cgt <config> <flag> <template> <filter>\n"
            << L"  cgt setTemplate <templateName> <config> <filter>\n"
            << L"  cgt rmTemplate <templateName> <flag>\n"
            << L"  cgt --help\n\n"

            << L"CONFIG\n"
            << L"  --output=<file>\n"
            << L"      Output file path.\n\n"

            << L"  --filePrefix=<prefix>\n"
            << L"      Prefix for generated section markers.\n\n"

            << L"FLAGS\n"
            << L"  --replace\n"
            << L"      Replace output file instead of append.\n\n"

            << L"  --all\n"
            << L"      Remove all templates (rmTemplate only).\n\n"

            << L"TEMPLATES\n"
            << L"  -<templateName>\n"
            << L"      Apply template.\n"
            << L"      Only the first existing template is used.\n\n"

            << L"FILTERS\n"
            << L"  .<ext>\n"
            << L"      Filter by extension.\n\n"

            << L"  <dir>\n"
            << L"      Filter by directory or file.\n\n"

            << L"EXAMPLES\n"
            << L"  cgt\n"
            << L"  cgt .cpp .h\n"
            << L"  cgt src include\n"
            << L"  cgt src/main.cpp\n"
            << L"  cgt -cpp\n"
            << L"  cgt -cpp --output=result.txt\n"
            << L"  cgt --replace .cpp src\n"
            << L"  cgt setTemplate cpp .cpp include src\n"
            << L"  cgt rmTemplate cpp\n"
            << L"  cgt rmTemplate --all\n\n"

            << L"OUTPUT FORMAT\n"
            << L"  <filePrefix>START <path>\n"
            << L"  <...code...>\n"
            << L"  <filePrefix>END <path>\n"

            << std::endl;
    }
}
