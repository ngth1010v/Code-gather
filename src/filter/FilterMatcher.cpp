#include "filter/FilterMatcher.h"

#include "filter/FilterUtils.h"
#include "log/Logger.h"



namespace cgt::filter::detail
{
    bool MatchRule(const std::vector<std::wstring>& srcComponents,
                   const std::vector<std::wstring>& ruleComponents,
                   const bool ignore)
    {
        if (ruleComponents.empty()) return false;
        if (srcComponents.empty()) return false;

        size_t sci = 0;
        size_t rci = 0;

        std::wstring lastSrcComponent = srcComponents[srcComponents.size()-1];
        bool isDir = lastSrcComponent[lastSrcComponent.size()-1] == '/';

        bool unmatch2 = false;
        bool unmatch1 = false;
        bool unmatch = false;

        while (sci < srcComponents.size() && rci < ruleComponents.size())
        {
            const std::wstring& rule = ruleComponents[rci];

            // **xxx -> match any component that ends with xxx
            if (rule.size() >= 2 && rule[0] == L'*' && rule[1] == L'*')
            {

                std::wstring ruleName = rule.substr(2);

                while (sci < srcComponents.size() && !EndWith(srcComponents[sci], ruleName))
                {
                    ++sci;
                }

                if (sci >= srcComponents.size())
                {

                    if (!ignore && isDir)
                    {
                        return true;
                    }

                    unmatch2 = true;
                    break;
                }

                ++sci;
                ++rci;
                continue;
            }

            // *xxx -> match current component that ends with xxx
            if (!rule.empty() && rule[0] == L'*')
            {
                std::wstring ruleName = rule.substr(1);

                if (!EndWith(srcComponents[sci], ruleName))
                {
                    unmatch1 = true;
                    break;
                }

                ++sci;
                ++rci;
                continue;
            }

            // exact match
            if (srcComponents[sci] != rule)
            {
                unmatch = true;
                break;
            }

            ++sci;
            ++rci;
        }



        // DEBUG
        // log::Logger::Warning(L"FilterMatcher", L"");
        // log::Logger::Warning(L"FilterMatcher", isDir ? L"dir" : L"file");
        // std::wstring srcLog = L"Src: [";
        // std::wstring ruleLog = L"Rule: [";
        // for (auto c : srcComponents) srcLog = srcLog + c + L",";
        // for (auto c : ruleComponents) ruleLog = ruleLog + c + L",";
        // srcLog += L"]";
        // ruleLog += L"]";
        // log::Logger::Warning(L"FilterMatcher", srcLog);
        // log::Logger::Warning(L"FilterMatcher", ruleLog);

        // if (unmatch2) log::Logger::Warning(L"FilterMatcher", L"Unmatch 2");
        // else if (unmatch1) log::Logger::Warning(L"FilterMatcher", L"Unmatch 1");
        // else if (unmatch) log::Logger::Warning(L"FilterMatcher", L"Unmatch");
        // else log::Logger::Warning(L"FilterMatcher", L"Match");
        // DEBUG

        if (!ignore && isDir && sci == srcComponents.size() && !unmatch2 && !unmatch1 && !unmatch)
        {
            return true;
        }
        return rci == ruleComponents.size();
    }
}
