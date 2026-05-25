# 📦 CodeGather

Simple and fast CLI tool for collecting source files into one `.txt` output.

Designed for:
- sharing code with AI
- exporting project snippets
- quick code packaging
- readable aggregated output

---

# 🚀 Install

1. Download `CodeGather-Setup` from the GitHub Release page
2. Install
3. Open terminal in your workspace
4. Run:

```bash
cgt
```

---

# ⚡ CLI

## Core CLI

```bash
cgt <config> <flag> <template> <filter>
```

All components can appear in any order.

---

## Config

### Output file

```bash
--output=result.txt
```

### Custom file prefix

```bash
--filePrefix=##
```

---

## Flags

### Replace output instead of append

```bash
--replace
```

---

## Templates

### Apply template

```bash
-cpp
```

### Apply template with space in name

```bash
" -my template "
```

Behavior:
- only the first existing template is used
- if multiple templates are provided:
  - invalid templates are skipped
  - first existing template is applied
  - remaining templates are ignored with warning

CLI config always overrides template config.

CLI filters are appended to template filters without duplicates.

---

## Filters

### Extension filter

```bash
.cpp
.h
```

### Directory filter

```bash
src
include
```

### Direct file

```bash
src/main.cpp
```

---

# 🧩 Template Commands

## Create / Update Template

```bash
cgt setTemplate <templateName> <config> <filter>
```

Example:

```bash
cgt setTemplate cpp .cpp .h include src
```

---

## Remove Template

### Remove one template

```bash
cgt rmTemplate cpp
```

### Remove all templates

```bash
cgt rmTemplate --all
```

---

# 🖥️ Console Output

## Example

```text
Found files:
  Code-gather/
  ├── include/
1 │   └── main.h
  └── src/
2     └── main.cpp
```

Properties:
- numbered entries are selectable
- each extension has its own color
- colors are configurable in `.cgtconfig`

---

# 📌 File Selection

```text
1 2
```

or

```text
1,2
```

---

# 📄 Result Output

Generated output format with:

```bash
--filePrefix=**
```

Result:

```text
*CODE:

**START test1.cpp
<...code...>
**END test1.cpp

**START test2.cpp
<...code...>
**END test2.cpp
```

Properties:
- plain text output
- deterministic format
- easy to parse later
- preserves original formatting

---

# ⚙️ .cgtconfig

CodeGather automatically creates `.cgtconfig` inside the workspace directory.

Used for:
- ignore rules
- extension colors
- output path
- file prefix
- templates

---

# 🧱 Config Format

## Example

```ini
[GENERAL]
output=E:/CODE/Code-gather/cgt-result.txt
filePrefix=**

[IGNORE]
.vscode/
.git/
*.obj
**build/
include/*.gen.h
src/**temp/

[COLOR]
bat=180,180,180
bin=182,201,164
c=200,220,255
cpp=120,210,255
h=255,220,120
md=180,255,180
txt=220,220,220
py=255,230,120
js=255,220,100
jsx=120,220,255
tsx=120,255,220
gitignore=180,180,180
cgtconfig=255,180,255

[TEMPLATE:cpp]
output=cpp-result.txt
filePrefix=**
ext=.cpp
ext=.h
dir=src
dir=include
```

---

# 🚫 Ignore Rules

Ignore rules are component-based path matchers.

Rules are split using `/` and matched component-by-component.

Example:

```text
include/data/file.txt
-> ["include/", "data/", "file.txt"]
```

Rule example:

```text
include/*.txt
-> ["include/", "*.txt"]
```

---

# 🧩 Ignore Rule Syntax

## Exact Component Match

Matches an exact component.

```text
include/data/
```

Matches:

```text
include/data/
```

Does not match:

```text
src/include/data/
```

---

## `*name`

Matches a component in the current level if the component ends with `name`.

```text
*.txt
```

Matches:

```text
hello.txt
test.txt
```

Does not match:

```text
dir/hello.txt
```

---

## `**name`

Matches any component recursively if the component ends with `name`.

```text
**.txt
```

Matches:

```text
a.txt
dir/a.txt
src/test/a.txt
```

---

# 📁 Directory Rules

Directory rules should end with `/`.

## Ignore exact directory level

```text
build/
```

Matches:

```text
build/
```

Does not match:

```text
src/build/
```

---

## Ignore directory recursively

```text
**build/
```

Matches:

```text
build/
src/build/
a/b/c/build/
```

---

# 🧪 Ignore Rule Examples

## Ignore root `.git`

```text
.git/
```

---

## Ignore all `.obj` files recursively

```text
**.obj
```

---

## Ignore generated headers inside `include/`

```text
include/*.gen.h
```

Matches:

```text
include/test.gen.h
```

Does not match:

```text
include/core/test.gen.h
```

---

## Ignore every `temp/` directory recursively

```text
**temp/
```

Matches:

```text
temp/
src/temp/
a/b/temp/
```

---

# 📌 Default Ignore Rules

```text
.vscode/
.git/
```