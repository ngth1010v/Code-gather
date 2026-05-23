#include "app/AppRunner.h"

#include "cli/ArgParser.h"
#include "cli/ParsedArgs.h"
#include "io/FileReader.h"
#include "io/TargetWriter.h"
#include "log/ConsolePrompt.h"
#include "log/Logger.h"
#include "scan/DiscoveryScanner.h"
#include "scan/IgnoreStore.h"
#include "util/PathUtil.h"
#include "util/StringUtil.h"
#include "util/TextFilePolicy.h"

#include <algorithm>
#include <filesystem>
#include <set>
#include <unordered_set>
#include <stdexcept>
#include <cwctype>
#include <iostream>
#include <windows.h>
#include <string>
#include <cmath>
#include <algorithm>

namespace cgt::app
{
    namespace fs = std::filesystem;

    static fs::path GetCallerDir()
    {
        wchar_t buffer[MAX_PATH * 4]{};
        DWORD len = GetCurrentDirectoryW(static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0])), buffer);
        if (len == 0) return fs::current_path(); // Dự phòng nếu lỗi
        return fs::path(buffer, buffer + len);
    }

    static bool IsDirectPathToken(const std::wstring& token)
    {
        return !cgt::util::IsFilterToken(token);
    }

    static std::vector<std::wstring> ExtractFilters(const std::vector<std::wstring>& sourceArgs)
    {
        std::vector<std::wstring> filters;
        for (const auto& token : sourceArgs)
        {
            if (cgt::util::IsFilterToken(token))
            {
                filters.push_back(cgt::util::ToLower(token));
            }
        }
        return filters;
    }

    static std::vector<std::wstring> ExtractDirects(const std::vector<std::wstring>& sourceArgs)
    {
        std::vector<std::wstring> directs;
        for (const auto& token : sourceArgs)
        {
            if (!cgt::util::IsFilterToken(token))
            {
                directs.push_back(token);
            }
        }
        return directs;
    }

    // Hàm kích hoạt ANSI Escape Codes cho Console (Chỉ cần gọi 1 lần duy nhất ở đầu hàm main)
    void EnableVirtualTerminalProcessing()
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;

        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode))
        {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }

    struct RGB { int r, g, b; };

    // Hàm chuyển đổi HSL sang RGB (H: 0-360, S: 0.0-1.0, L: 0.0-1.0)
    RGB HSLtoRGB(double h, double s, double l) 
    {
        double c = (1.0 - std::abs(2.0 * l - 1.0)) * s;
        double x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2.0) - 1.0));
        double m = l - c / 2.0;

        double r_prime = 0, g_prime = 0, b_prime = 0;

        if (0 <= h && h < 60)       { r_prime = c; g_prime = x; b_prime = 0; }
        else if (60 <= h && h < 120)  { r_prime = x; g_prime = c; b_prime = 0; }
        else if (120 <= h && h < 180) { r_prime = 0; g_prime = c; b_prime = x; }
        else if (180 <= h && h < 240) { r_prime = 0; g_prime = x; b_prime = c; }
        else if (240 <= h && h < 300) { r_prime = x; g_prime = 0; b_prime = c; }
        else if (300 <= h && h <= 360){ r_prime = c; g_prime = 0; b_prime = x; }

        return {
            static_cast<int>((r_prime + m) * 255),
            static_cast<int>((g_prime + m) * 255),
            static_cast<int>((b_prime + m) * 255)
        };
    }

    static void PrintColoredPath(const std::wstring& text, const std::wstring& extension)
    {
        // 1. Tính hash từ extension (giữ nguyên gốc)
        const std::wstring key = cgt::util::ToLower(extension);
        std::size_t hash = 1469598103934665603ULL;
        for (wchar_t ch : key)
        {
            hash ^= static_cast<std::size_t>(ch);
            hash *= 1099511628211ULL;
        }

        // 2. Tính toán HSL động dựa trên hash
        // Hue (Sắc độ): lấy từ 0 đến 359 độ
        double h = static_cast<double>(hash % 360); 
        
        // Saturation (Độ bão hòa): cố định 0.85 (85%) để màu sắc tươi tắn, không bị xám
        double s = 0.85; 
        
        // Lightness (Độ sáng): Map phần dư còn lại của hash vào dải 0.72 đến 0.85 (> 70%)
        // Việc đổi nhẹ độ sáng giúp các phần mở rộng có Hue gần nhau vẫn phân biệt được
        double l = 0.72 + (static_cast<double>(hash % 100) / 100.0) * 0.13;

        // 3. Chuyển sang RGB
        RGB color = HSLtoRGB(h, s, l);

        // 4. In ra Console bằng mã ANSI TrueColor
        std::wcout << L"\x1b[38;2;" << color.r << L";" << color.g << L";" << color.b << L"m" 
                << text 
                << L"\x1b[0m";
    }

    static void PrintFoundFilesBlock(const std::vector<scan::DiscoveredFile>& files,
                                    const std::vector<fs::path>& directPaths,
                                    const fs::path& root)
    {
        if (files.empty() && directPaths.empty())
        {
            return;
        }

        cgt::log::Logger::Plain(L"Found files:");
        for (std::size_t i = 0; i < files.size(); ++i)
        {
            const auto& item = files[i];
            std::wcout << L"(" << (i + 1) << L") ";
            for (int count = 0; count < std::to_string(files.size()+1).size() - std::to_string(i+1).size(); count++)
                std::wcout << L" ";

            PrintColoredPath(item.relativePath, cgt::util::ExtensionKey(item.absolutePath));
            std::wcout << std::endl;
        }

        for (const auto& direct : directPaths)
        {
            const std::wstring rel = cgt::util::RelativeDisplayPath(root, direct);
            std::wcout << L"(+) ";
            PrintColoredPath(rel, cgt::util::ExtensionKey(direct));
            std::wcout << std::endl;
        }
    }

    static std::vector<std::size_t> ParseSelectionIds(const std::wstring& input,
                                                     std::size_t maxCount,
                                                     bool& hadAnyValid)
    {
        std::vector<std::size_t> selected;
        hadAnyValid = false;

        const auto tokens = cgt::util::SplitTokens(input);
        std::set<std::size_t> unique;
        for (const auto& token : tokens)
        {
            if (token.empty())
            {
                continue;
            }

            bool numeric = true;
            for (wchar_t ch : token)
            {
                if (!std::iswdigit(ch))
                {
                    numeric = false;
                    break;
                }
            }

            if (!numeric)
            {
                cgt::log::Logger::Warning(L"Selection", L"Rejected invalid selection token: " + token);
                continue;
            }

            std::size_t id = 0;
            try
            {
                id = static_cast<std::size_t>(std::stoull(token));
            }
            catch (...)
            {
                cgt::log::Logger::Warning(L"Selection", L"Rejected invalid selection token: " + token);
                continue;
            }
            if (id == 0 || id > maxCount)
            {
                cgt::log::Logger::Warning(L"Selection", L"Ignored out-of-range id: " + token);
                continue;
            }

            unique.insert(id);
            hadAnyValid = true;
        }

        selected.assign(unique.begin(), unique.end());
        return selected;
    }

    static std::vector<io::GatheredBlock> BuildBlocks(const fs::path& root,
                                                     const std::vector<scan::DiscoveredFile>& discovered,
                                                     const std::vector<std::size_t>& selectionIds,
                                                     const std::vector<fs::path>& directPaths,
                                                     const scan::IgnoreStore& ignoreStore,
                                                     bool& hadAnyReadAttempt)
    {
        std::vector<io::GatheredBlock> blocks;
        io::FileReader reader;
        hadAnyReadAttempt = false;

        std::set<std::wstring> used;

        auto addPath = [&](const fs::path& path)
        {
            const std::wstring key = cgt::util::NormalizeForCompare(path);
            if (used.contains(key))
            {
                return;
            }

            if (ignoreStore.IsIgnored(path))
            {
                cgt::log::Logger::Warning(L"IgnoreStore", L"Ignored path: " + cgt::util::RelativeDisplayPath(root, path));
                return;
            }

            hadAnyReadAttempt = true;

            while (true)
            {
                const io::ReadResult result = reader.Read(path);
                if (result.success)
                {
                    io::GatheredBlock block;
                    block.relativePath = cgt::util::RelativeDisplayPath(root, path);
                    block.content = result.content;
                    blocks.push_back(std::move(block));
                    used.insert(key);
                    return;
                }

                cgt::log::Logger::Warning(L"FileReader", L"Failed to read: " + cgt::util::RelativeDisplayPath(root, path));
                const auto choice = cgt::log::ConsolePrompt::AskReadFailure();
                if (choice == cgt::log::ReadFailureChoice::Retry)
                {
                    continue;
                }
                if (choice == cgt::log::ReadFailureChoice::Skip)
                {
                    return;
                }
                throw std::runtime_error("cancelled");
            }
        };

        try
        {
            for (std::size_t idx : selectionIds)
            {
                if (idx == 0 || idx > discovered.size())
                {
                    continue;
                }
                addPath(discovered[idx - 1].absolutePath);
            }

            for (const auto& direct : directPaths)
            {
                addPath(direct);
            }
        }
        catch (...)
        {
            throw;
        }

        return blocks;
    }

    static fs::path ResolveTarget(const fs::path& root, const std::wstring& token)
    {
        fs::path input(token);
        return cgt::util::ResolveFromRoot(root, input);
    }

    int AppRunner::Run(int argc, wchar_t* argv[])
    {
        using namespace cgt;

        EnableVirtualTerminalProcessing();

        const fs::path rootDir = GetCallerDir();

        // START-DEBUG
        log::Logger::Info(L"AppRunner", rootDir.wstring());
        // END-DEBUG

        cli::ArgParser parser;
        const cli::ParsedArgs args = parser.Parse(argc, argv);

        if (!args.unknownTokens.empty())
        {
            for (const auto& token : args.unknownTokens)
            {
                log::Logger::Warning(L"ArgParser", L"Unknown token ignored: " + token);
            }
        }

        const std::wstring targetToken = args.GetFirstConfigValue(L"target", L"cgt-result.txt");
        const std::wstring filePrefix = args.GetFirstConfigValue(L"fileprefix", L"**");
        const bool replace = args.HasFlag(L"replace");
        const std::vector<std::wstring> ignoreTokens = args.GetConfigValues(L"ignore");
        const std::vector<std::wstring> filters = ExtractFilters(args.sourceArgs);
        const std::vector<std::wstring> directTokens = ExtractDirects(args.sourceArgs);

        scan::IgnoreStore ignoreStore(rootDir);
        if (!ignoreStore.Load())
        {
            log::Logger::Warning(L"IgnoreStore", L"Could not load .cgtignore. Continuing without stored ignore entries.");
        }

        ignoreStore.MergeCliEntries(ignoreTokens);
        if (!ignoreStore.SaveIfNeeded())
        {
            log::Logger::Warning(L"IgnoreStore", L"Could not write .cgtignore.");
        }

        const fs::path targetPath = ResolveTarget(rootDir, targetToken);
        std::vector<fs::path> directPaths;
        for (const auto& token : directTokens)
        {
            const fs::path resolved = cgt::util::ResolveFromRoot(rootDir, fs::path(token));
            directPaths.push_back(resolved.lexically_normal());
        }

        const bool onlyDirectSources = !directPaths.empty() && filters.empty();
        std::vector<scan::DiscoveredFile> discovered;

        if (!onlyDirectSources)
        {
            scan::DiscoveryScanner scanner(rootDir, filters, ignoreStore.Entries());
            discovered = scanner.Scan();

            if (args.sourceArgs.empty() && discovered.empty())
            {
                log::Logger::Warning(L"Discovery", L"No readable text files were found.");
                return 0;
            }

            PrintFoundFilesBlock(discovered, filters.empty() ? std::vector<fs::path>{} : directPaths, rootDir);

            if (discovered.empty() && directPaths.empty())
            {
                log::Logger::Warning(L"Discovery", L"No files matched the current filters or ignore rules.");
                return 0;
            }
        }
        else
        {
            for (const auto& p : directPaths)
            {
                if (ignoreStore.IsIgnored(p))
                {
                    log::Logger::Warning(L"IgnoreStore", L"Ignored direct path: " + cgt::util::RelativeDisplayPath(rootDir, p));
                }
            }
        }

        std::vector<std::size_t> selectionIds;
        if (!onlyDirectSources)
        {
            if (!discovered.empty())
            {
                while (true)
                {
                    const std::wstring input = log::ConsolePrompt::AskSelection();
                    if (input.empty())
                    {
                        log::Logger::Warning(L"Selection", L"No valid selection was made.");
                        return 0;
                    }

                    bool hadAnyValid = false;
                    selectionIds = ParseSelectionIds(input, discovered.size(), hadAnyValid);
                    if (hadAnyValid && !selectionIds.empty())
                    {
                        break;
                    }

                    log::Logger::Warning(L"Selection", L"No valid selection was made.");
                }
            }
        }

        std::vector<io::GatheredBlock> blocks;
        bool hadAnyReadAttempt = false;
        try
        {
            blocks = BuildBlocks(rootDir, discovered, selectionIds, directPaths, ignoreStore, hadAnyReadAttempt);
        }
        catch (...)
        {
            log::Logger::Warning(L"FileReader", L"Run cancelled by user.");
            return 0;
        }

        if (blocks.empty())
        {
            log::Logger::Warning(L"Gather", L"No file matched the current filters / ignores / source constraints.");
            return 0;
        }

        io::TargetWriter writer;
        std::wstring errorMessage;
        while (true)
        {
            if (writer.Write(targetPath, filePrefix, replace, blocks, errorMessage))
            {
                log::Logger::Info(L"TargetWriter", L"Wrote output to " + cgt::util::RelativeDisplayPath(rootDir, targetPath));
                return 0;
            }

            log::Logger::Warning(L"TargetWriter", errorMessage);
            const auto choice = log::ConsolePrompt::AskWriteFailure();
            if (choice == log::WriteFailureChoice::Cancel)
            {
                return 0;
            }
        }
    }
}
