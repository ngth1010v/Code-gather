cmake_minimum_required(VERSION 3.20)
set(AppName cgt)
project(${AppName} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

add_executable(${AppName}
    src/main.cpp

    src/app/AppRunner.cpp
    src/app/AppUtils.cpp
    src/app/TemplateRunner.cpp

    src/app/CoreRunner.cpp
    src/app/coreRunner/CoreBuildTree.cpp
    src/app/coreRunner/CoreConfirmer.cpp
    src/app/coreRunner/CoreSelector.cpp
    src/app/coreRunner/CoreUtils.cpp

    src/cli/ArgParser.cpp

    src/config/ConfigColor.cpp
    src/config/ConfigParser.cpp
    src/config/ConfigParserHelpers.cpp
    src/config/ConfigPublic.cpp
    src/config/ConfigState.cpp
    src/config/ConfigWriter.cpp

    src/filter/Filter.cpp
    src/filter/FilterMatcher.cpp
    src/filter/FilterUtils.cpp

    src/io/FileReader.cpp
    src/io/TargetWriter.cpp

    src/log/ConsolePrompt.cpp
    src/log/Logger.cpp
    src/log/PrintTree.cpp

    src/scan/DiscoveryScanner.cpp

    src/ui/panel/PanelCommon.cpp
    src/ui/panel/PanelGeometry.cpp
    src/ui/panel/PanelLines.cpp
    src/ui/panel/PanelRender.cpp
    src/ui/panel/PanelScroll.cpp

    src/ui/Terminal.cpp
    src/ui/terminal/Ansi.cpp
    src/ui/terminal/Input.cpp
    src/ui/terminal/Platform.cpp
    src/ui/terminal/State.cpp

    src/util/PathUtil.cpp
    src/util/StringUtil.cpp
    src/util/TextFilePolicy.cpp
    src/util/UnicodeConsole.cpp
    src/util/EncodingUtil.cpp
)

target_include_directories(${AppName} PRIVATE include)

target_compile_definitions(${AppName} PRIVATE UNICODE _UNICODE)

if(WIN32)
    target_link_libraries(${AppName} PRIVATE shell32)
endif()

if(MINGW)
    target_link_options(${AppName} PRIVATE -static-libgcc -static-libstdc++)
endif()
