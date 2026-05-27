#include "app/TemplateRunner.h"

#include "app/AppUtils.h"
#include "cli/ParsedArgs.h"
#include "config/Config.h"
#include "log/Logger.h"

namespace cgt::app
{
    int TemplateRunner::RunSetTemplate(const cgt::cli::ParsedArgs& args)
    {
        WarnUnknownTokens(args);

        const auto& names = args.GetTemplateNames();
        if (names.empty() || names.front().empty())
        {
            cgt::log::Logger::Warning(L"Template", L"Missing template name for setTemplate.");
            return 0;
        }

        RuntimeState state;
        state.outputToken = cgt::config::GetOutputFilePath().wstring();
        state.filePrefix = cgt::config::GetFilePrefix();

        ApplyConfigOverrides(args, state);
        CollectCliFilters(args, state);

        cgt::config::CgtTemplate tl;
        tl.output = state.outputToken;
        tl.filePrefix = state.filePrefix;
        tl.filters = state.filters;

        cgt::config::SetTemplate(names.front(), tl);
        cgt::config::Write();
        return 0;
    }

    int TemplateRunner::RunRemoveTemplate(const cgt::cli::ParsedArgs& args)
    {
        WarnUnknownTokens(args);

        if (args.HasFlag(L"all"))
        {
            const std::vector<std::wstring> names = cgt::config::GetTemplateNames();
            for (const auto& name : names)
            {
                if (!name.empty())
                {
                    cgt::config::RemoveTemplate(name);
                }
            }

            cgt::config::Write();
            return 0;
        }

        const auto& names = args.GetTemplateNames();
        if (names.empty() || names.front().empty())
        {
            cgt::log::Logger::Warning(L"Template", L"Missing template name for rmTemplate.");
            return 0;
        }

        cgt::config::RemoveTemplate(names.front());
        cgt::config::Write();
        return 0;
    }
}
