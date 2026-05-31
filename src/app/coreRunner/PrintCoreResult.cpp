#include "app/coreRunner/PrintCoreResult.h"

#include "app/coreRunner/TreeBuilder.h"
#include "util/PathUtil.h"

#include <iostream>

namespace cgt::app::coreRunner::printResult
{
    namespace
    {
        void ClearMainBuffer()
        {
            std::wcout << L"\x1b[2J\x1b[H";
            std::wcout.flush();
        }
    }

    int PrintSuccess(const std::vector<cgt::scan::DiscoveredFile>& discovered,
                     const std::vector<std::size_t>& selectedIds,
                     const fs::path& rootDir,
                     const fs::path& outputPath)
    {
        ClearMainBuffer();

        const TreeNode tree =
            BuildSelectedTree(discovered, selectedIds, rootDir);

        const std::vector<std::wstring> lines =
            BuildTreeLines(tree);

        for (const auto& line : lines)
        {
            std::wcout << line << L'\n';
        }

        std::wcout << L"---\n";

        std::wcout
            << L"CodeGather: Successfully merged "
            << selectedIds.size()
            << L" files into ["
            << cgt::util::RelativeDisplayPath(rootDir, outputPath)
            << L"].\n\n";

        std::wcout.flush();

        return 0;
    }

    int PrintNothing()
    {
        ClearMainBuffer();

        std::wcout
            << L"CodeGather: No files were merged.\n\n";

        std::wcout.flush();

        return 0;
    }
}