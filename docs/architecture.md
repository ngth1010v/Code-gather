# I. CodeGather Overview
```text
Project name: CodeGather
CLI name: cgt
Target OS: Windows 11
Build target: ucrt64 / console app
Core job: collect selected text-readable source files into one target .txt file

The app must be simple to run from console, but split into small modules so that each responsibility is isolated:
- command parsing
- file discovery
- ignore handling
- file reading
- target writing
- logging and prompt handling
- color output
- app orchestration

The design goal is not only to work for the current options, but to make future flags and configs easy to add without rewriting the parser or main flow.
```

## 1. High-level behavior
```text
The user runs:

cgt <args>

The app must:
1) parse config / flags / source args
2) resolve working root from the running EXE context
3) load .cgtignore if present
4) discover readable text files recursively from the root
5) print found files with stable numbering and color by extension
6) let the user choose which files to gather
7) read selected files and write them into one target .txt file
8) handle blocked / missing files with interactive retry logic
9) exit cleanly with warning or error logs when needed

Important rule:
- all discovered paths and all printed paths are relative to the running EXE directory context
- output path should also be resolved in the same context unless user passes an absolute path
```

### A. Scope and terminology
```text
Definitions:
- RootDir: the directory used as the base for scanning and for relative display paths
- RunningExeDir: directory of the .exe file, used as the default context for paths
- RelativePath: path printed to console and written in target format
- SourceArg: a non-flag, non-config CLI argument that is either:
  - a filter token like .h or .cpp
  - a direct file path like main.cpp
- TargetFile: the final output file such as cgt-result.txt
- IgnoreEntry: a file path or folder path stored in .cgtignore
```

## 2. Default assumptions
```text
Default target file:
- cgt-result.txt

Default file prefix:
- **

Default write mode:
- append

Default file selection behavior:
- if no source args are given, include all readable text-like files
- if only one source arg exists and it is a file path, skip the found-files menu and gather that file directly

The app must be able to read Unicode paths and Unicode content correctly.
```

# II. CLI Rules and Parsing
```text
The CLI must support three groups of args:
- flags
- configs
- source / filter args

The parser must be extensible:
- new flags should be addable without changing the main workflow
- new config keys should be addable without changing selection logic
- unknown tokens should not crash the app
```

## 1. Flag format
```text
Flag format:
--<flag>

Current flag list:
- --replace

Meaning:
- --replace forces full overwrite of target file instead of append
- if absent, append mode is used

Future-safe parser rule:
- flags are stored in a dedicated set-like structure
- parser should accept new flags later without changing token shape
```

### A. Flag precedence
```text
When multiple modes may conflict, use this order:
1) invalid / unreadable target path handling
2) explicit --replace
3) default append

No other flag should silently override --replace unless a future spec explicitly says so.
```

## 2. Config format
```text
Config format:
--<configKey>=<value>

Current config keys:
- --target=<.txt path>
- --filePrefix=<text>
- --ignore=<filePath/folderPath>

Notes:
- value may be quoted or unquoted
- quotes should be stripped before use
- whitespace around '=' should not be required
- parser should not assume only these three configs exist in the future
```

### A. Current config meanings
```text
--target=<path>
- sets the final output path
- default is cgt-result.txt
- create the file if it does not exist
- if path is relative, resolve from the running EXE context

--filePrefix=<text>
- prefix inserted before START and END lines for each gathered file
- default is **
- this prefix is used only for the result format, not for log lines

--ignore=<filePath/folderPath>
- marks a file or directory for skipping during discovery
- this is a non-recursive ignore entry at the input level
- the actual skip behavior is recursive by effect: if a folder is ignored, its subtree is skipped during traversal
```

## 3. Source / filter args
```text
Source args are all tokens that are neither flags nor configs.
There are two kinds of source args:
- extension filter tokens like .h, .cpp
- direct file paths like main.cpp

Rules:
- if no source args exist, scan all readable text-like files
- if one or more filter tokens exist, show the found-files menu normally
- if a file path is explicitly given, include it directly as a chosen source
- if the only source arg is a file path, skip the found-files menu and gather it directly
```

### A. Direct file path treatment
```text
A direct file path means the user intentionally points to a specific file.
Examples:
- cgt main.cpp
- cgt "src/main.cpp"

Behavior:
- resolve the path from the running EXE context when relative
- do not require the file path to match extension filters
- if a file path also matches a filter, it must still be treated as an explicit selection
```

### B. Filter treatment
```text
A filter token is a token that starts with a dot and looks like an extension.
Examples:
- .h
- .cpp
- .cmake

Behavior:
- only files with matching extension are added to the discovered list
- if no filter tokens exist, all readable text-like files are eligible
```

# III. Ignore System and Discovery Scope
```text
The app must support both CLI ignore entries and persistent ignore storage.
The ignore system is important because the app is meant to run repeatedly in the same directory without re-choosing known excluded paths.
```

## 1. .cgtignore behavior
```text
.cgtignore rules:
- on startup, if a .cgtignore file exists in the current directory context, load it
- entries in .cgtignore are automatically skipped during discovery
- .cgtignore stores previously ignored files / folders
- .cgtignore should be respected before any file is printed as found

Creation rule:
- create .cgtignore only when both conditions are true:
  1) there is no .cgtignore in the current directory
  2) the CLI includes at least one --ignore= entry
```

### A. Ignore matching
```text
Ignore matching should be path-based, not name-only.
Recommended behavior:
- normalize separators before comparison
- compare relative paths in the running EXE context
- support exact file match and folder prefix match

Folder ignore example:
- if the ignore entry is src/generated/
- any file under src/generated/ should be skipped

File ignore example:
- if the ignore entry is include/a.h
- only that file should be skipped
```

### B. Ignore precedence
```text
Ignore order:
1) .cgtignore entries
2) CLI --ignore entries
3) runtime discovery candidates

If the same path appears in multiple ignore sources, it is still ignored once.
```

## 2. Discovery root
```text
Discovery root is the running EXE directory context.
This means:
- all discovered relative paths are printed from that root
- target path resolution should also use this root when relative
- source file path arguments should be resolved from this root when relative

The app should not depend on the user's shell current directory if that would make path output inconsistent with the executable context.
```

### A. Discovery algorithm overview
```text
Discovery should be recursive for normal scanning.
Pseudo flow:
1) start from RootDir
2) walk the tree recursively
3) skip ignored folders early
4) skip ignored files
5) keep only readable text-like candidates
6) apply extension filter if present
7) de-duplicate final candidates
8) sort candidates for display
```

# IV. File Classification and Sorting
```text
The app must decide which files are eligible to be read as text, and must display them in a stable order that resembles VSCode explorer sorting.
```

## 1. Text-readable file policy
```text
The app should collect files that can reasonably be converted to text.
Expected examples:
- .cpp
- .h
- .cmake
- CMakeLists.txt
- .txt
- .md
- other plain-text project files

The implementation should not treat binary files as text candidates.
If a file cannot be read as text, it must be handled as an error or skipped according to the runtime prompt rule.
```

### A. File type detection
```text
Recommended approach:
- extension-based allowlist for common text project files
- special filename allowlist for files like CMakeLists.txt
- optional quick content check to avoid binary data

The architecture should keep file classification separate from discovery traversal so that the allowlist can be extended later.
```

## 2. Sort order
```text
Sort order must be similar to VSCode explorer behavior.
A practical implementation should:
- group folders before files when relevant
- sort alphabetically in a stable, case-aware way
- keep the order deterministic across runs

The display order and gather order should use the same final sorted candidate list unless later rules force a manual selection order.
```

### A. Display numbering
```text
Numbering rules:
- discovered filter-based files are numbered from 1
- explicit direct filePath args are not counted in the numbered found list if a filter list exists
- if the user provided only a direct filePath and no filter, skip the found list entirely

The final chosen files must preserve a stable internal order even if the user selects ids in a different order.
```

## 3. Extension colors
```text
Files with different extensions should be printed in different colors.
Color rules:
- color choice must be random-looking but stable per extension
- brightness must remain above 70%
- the same extension should keep the same color across multiple runs of cgt

Recommended implementation:
- hash the extension string
- derive HSL or another bright color space from the hash
- clamp brightness/value to a high range
```

### A. Color handling scope
```text
Coloring applies to console output only.
It does not affect file content written into the target file.
Warnings and errors use fixed semantic colors:
- warning = yellow
- error = red
```

# V. User Interaction Flow
```text
The app is interactive after discovery.
Its goal is to let the user choose which files are gathered, then guide the write step safely.
```

## 1. Normal flow
```text
Standard runtime flow:
1) parse args
2) load ignore data
3) discover files
4) print found files if required
5) ask user to choose ids or confirm source path selection
6) read selected files
7) write target file
8) exit cleanly
```

### A. Found files prompt
```text
Found files output must follow this style:
Found files:
(1) include/a.h
(2) include/b.h
(3) src/main.cpp
(4) CMakeLists.txt

If filters exist:
- print the normal numbered list for filtered discoveries
- print direct filePath args as:
  (+) <filePath>
- direct filePath args are not part of the numbered index

If no filter exists and only direct source path exists:
- skip found-files printing entirely
```

### B. Selection input
```text
The user selects files by id list such as:
1,3,4

Selection parser should support:
- commas
- spaces
- repeated separators
- invalid tokens being rejected with a warning

Recommended selection rules:
- invalid ids do not crash the app
- out-of-range ids are ignored or reprompted depending on the chosen UX implementation
- the architecture should prefer explicit confirmation over silent guessing
```

## 2. Missing selection behavior
```text
If no valid selection is made:
- the app should not attempt to gather files
- it should warn and stop cleanly

The architecture should keep selection validation separate from file reading so that the prompt logic can be adjusted later.
```

# VI. Result File Format
```text
The target file is a plain text aggregation file.
Its content format must be deterministic and easy to parse later by both humans and tools.
```

## 1. Result envelope
```text
The first line of the target output must be:
*CODE:

After that, each gathered file is appended in the format below.
The prefix marker is controlled by --filePrefix.
```

### A. Per-file format
```text
Per-file format:
<filePrefix>START <filePath>
<code in file>
<filePrefix>END <filePath>

Example:
**START src/main.cpp
#include <iostream>

int main() {
    std::cout << "Hello" << std::endl;
    return 0;
}
**END src/main.cpp
```

### B. Append and replace rules
```text
Append mode:
- keep existing file content and append new gathered blocks
- if target file does not exist, create it

Replace mode:
- clear the previous content and write the new content from scratch
- replace behavior is controlled only by --replace
```

## 2. Write order
```text
Write order should follow the final selected file order.
Recommended order:
1) selected numbered items in the order the app stores them
2) direct source filePath items if they are included in the current run

The architecture should keep selection order explicit so that future UX changes do not change file order accidentally.
```

# VII. Error, Warning, and Retry Rules
```text
The app must distinguish warnings from errors and print them with consistent formatting.
Console logs should remain human-readable and easy to grep.
```

## 1. Log format
```text
Warning format:
[WARNING][CODE-GATHER][<file call warning>] <content>

Error format:
[ERROR][CODE-GATHER][<file call error>] <content>

Notes:
- <file call warning> and <file call error> are the cpp file names without extension
- warning uses yellow
- error uses red
- ordinary logs may use normal console color or a dedicated neutral color
```

### A. Log ownership
```text
Each log call should carry its source module name.
This makes it easier to know whether the message came from:
- argument parser
- discovery
- reader
- writer
- ignore loader
- selection handler
```

## 2. File read failure prompt
```text
If a file is blocked or missing while reading, the app must show:
- Retry / Skip / Cancel
- default choice: Skip

English wording should be used in the prompt.
Recommended prompt text:
File is blocked or missing. Choose: Retry / Skip / Cancel (default: Skip)
```

### A. Read failure outcomes
```text
Retry:
- attempt to read the same file again

Skip:
- ignore this file and continue with the next selected file

Cancel:
- stop the whole run immediately
```

## 3. Target file write failure prompt
```text
If target.txt is blocked while writing, the app must show:
- Retry / Cancel
- default choice: Retry

Recommended prompt text:
Target file is blocked. Choose: Retry / Cancel (default: Retry)
```

### A. Write failure outcomes
```text
Retry:
- re-attempt the write using the same gathered content

Cancel:
- stop immediately and keep the current state unchanged as much as possible
```

## 4. No result case
```text
If no file matches the current filters / ignores / source constraints:
- print a warning
- shut down cleanly
- do not create a misleading empty result unless the user explicitly selected a direct path that resolves to an empty read for a recoverable reason
```

# VIII. Module Layout and File Split
```text
The code should be split into small cpp/h pairs to keep debugging easy.
The architecture should avoid one giant main.cpp with all logic mixed together.
```

## 1. Recommended project structure
```text
CodeGather/
├── CMakeLists.txt
├── build.bat
├── .cgtignore
├── include/
│   ├── app/
│   ├── cli/
│   ├── io/
│   ├── log/
│   ├── scan/
│   └── util/
└── src/
    ├── main.cpp
    ├── app/
    ├── cli/
    ├── io/
    ├── log/
    ├── scan/
    └── util/
```

### A. Core files and responsibility split
```text
main.cpp
- entry point only
- sets Unicode-safe console mode
- delegates to AppRunner

AppRunner
- orchestrates the full workflow
- receives parsed args and coordinates scan / select / read / write

ArgParser
- parses flags, configs, and source args
- stores unknown tokens safely

DiscoveryScanner
- walks filesystem
- applies filters and ignore rules
- returns candidate file list

IgnoreStore
- loads and saves .cgtignore
- normalizes entries

FileReader
- reads selected files as text
- handles blocked / missing / encoding issues

TargetWriter
- writes append or replace mode
- formats output blocks

Logger
- handles warning / error / normal logging and colors

ConsolePrompt
- asks retry / skip / cancel / retry / cancel questions
- parses user answers safely

PathUtil
- normalize, join, split, relative-path handling
```

## 2. Main.cpp Unicode rule
```text
main.cpp must be written so that CLI still receives full Unicode characters.
Recommended entry approach on Windows:
- use wmain or wWinMain based on build mode
- avoid lossy narrow conversion at the boundary
- convert only after capturing the full Unicode command line

If the build uses console mode, the app must still preserve Unicode arguments and file paths correctly.
```

### A. Console boundary rule
```text
At process startup:
- read argv in Unicode-safe form
- convert to internal wide string or UTF-8 consistently
- do not parse command arguments from a narrow lossy buffer
- keep output encoding consistent with the chosen console strategy
```

# IX. CMake and Build Assumptions
```text
The architecture should match the provided CMakeLists.txt and build.bat style.
The app is expected to be built on Windows with ucrt64 and run from console.
```

## 1. CMake goals
```text
CMake should support:
- C++20
- UNICODE and _UNICODE definitions
- optional USE_WWINMAIN definition
- optional HIDE_CONSOLE option
- Windows link settings
- shell32 linking
- MinGW static runtime options when applicable

The source list should be easy to extend as files are split into modules.
```

### A. Suggested source handling
```text
The current CMake example only lists src/main.cpp.
For the final architecture, the source list should be expanded to include all module cpp files.
Recommended approach:
- explicit file list for stable builds
- or a controlled GLOB only if the project policy accepts it

The architecture document should not force a single source enumeration style, but it should require that all module cpp files are linked into the executable.
```

## 2. build.bat goals
```text
build.bat should remain simple:
- configure Release build
- build the project
- copy generated executable to bin

The architecture should not require extra runtime steps beyond build and run.
```

### A. Packaging rule
```text
The final executable should be runnable directly from the console after build.
Any required runtime config file such as .cgtignore should be created or loaded relative to the chosen root context without extra manual installation.
```

# X. Edge Cases and Safety Rules
```text
The architecture must explicitly define behavior for ambiguous or failure scenarios so that implementation is predictable.
```

## 1. Duplicate and overlapping paths
```text
If the same file appears from multiple inputs:
- it should not be gathered twice in one run unless a future spec explicitly requires duplicates
- explicit source paths and discovered paths should be de-duplicated by normalized absolute or normalized relative key

If a folder is ignored and a file inside that folder is explicitly requested:
- the architecture should treat ignore rules as higher priority unless a later spec says explicit paths override ignore
```

### A. Duplicate rule rationale
```text
This prevents duplicated content in the target file and keeps the final output deterministic.
```

## 2. Empty input scenarios
```text
Possible empty-input cases:
- no args at all
- only ignore configs were provided
- filters exclude all files
- ignore rules remove all candidate files
- direct path does not exist

The app should warn and stop when there is nothing useful to gather.
```

## 3. Encoding and newline handling
```text
The app must preserve file content as text and avoid unintended rewrites.
Recommended rules:
- keep original line breaks when reading and writing text blocks
- normalize only where the target format requires it
- handle Unicode filenames and Unicode content

The architecture should not assume ASCII-only project text.
```

### A. Text reading rule
```text
When reading source files:
- do not strip meaningful whitespace from code blocks
- do not alter indentation
- do not auto-format source text

The collector should behave like a copier / packer, not a formatter.
```

# XI. Implementation Priority Order
```text
When coding this architecture, implement in this order:
1) arg parser
2) path and unicode handling
3) ignore loading and saving
4) discovery and sort
5) selection prompt
6) file reader with retry logic
7) target writer
8) logging and color output
9) CMake / build integration

This order reduces debugging risk because each layer can be tested in isolation.
```

## 1. Minimum viable first version
```text
The first working version should support:
- --target
- --filePrefix
- --replace
- .cgtignore load
- filter by extension
- direct file path source
- found files print
- interactive selection
- append / replace write
- warning / error logs

After that, extend with extra ignore persistence and richer UX only if needed.
```

## 2. Final behavior summary
```text
At runtime, CodeGather should feel like this:
- simple CLI
- predictable discovery
- readable colored output
- safe handling of blocked files
- stable aggregated text output
- easy future extension for more flags and configs
```
