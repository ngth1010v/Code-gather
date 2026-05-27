#pragma once

namespace cgt::cli
{
    struct ParsedArgs;
}

namespace cgt::app
{
    class TemplateRunner
    {
    public:
        int RunSetTemplate(const cgt::cli::ParsedArgs& args);
        int RunRemoveTemplate(const cgt::cli::ParsedArgs& args);
    };
}
