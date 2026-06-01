# 📦 CodeGather

Simple CLI tool for collecting source files into a single text file.

Useful for:
- sharing code with AI
- exporting project snippets
- creating compact project snapshots

---

# 🚀 Install

1. Download `CodeGather-Setup` from GitHub Releases
2. Install
3. Open a terminal inside your workspace
4. Run:

```bash
cgt
```

---

# ⚡ Usage

```bash
cgt <config> <flag> <template> <rule>
```

Arguments may appear in any order.

Example:

```bash
cgt -cpp src/ include/ !**build/ !**.obj
```

---

# ⚙️ Config / flags

Output file:

```bash
--output=cgt-result.md
```

Replace output instead of append:

```bash
--replace
--r
```

No wrap the code + No header:

```bash
--nowrap
--nw
```

CLI config always overrides template config.

---

# 🧩 Templates

Apply a template:

```bash
-cpp
```

Template names may contain spaces:

```bash
"-my template"
```

Rules:
- only the first valid template is used
- invalid templates are skipped
- CLI rules are appended to template rules
- duplicate rules are removed

Create or update:

```bash
cgt setTemplate cpp src/ include/ .cpp .h
```

Remove:

```bash
cgt rmTemplate cpp
```

Remove all:

```bash
cgt rmTemplate --all
```

---

# 🖥️ File Selection

Example:

```text
Found files:
  Project/
  ├── include/
1 │   └── main.h
  └── src/
2     └── main.cpp
```

Select files:

```text
1 2
```

or

```text
1,2
```

---

# 📄 Output Format

Example (with wrapped):

````md
# CODE
### main.cpp
```cpp
<code main.cpp>
```
### app.h
```h
<code app.h>
```
````

Properties:
- deterministic
- easy to parse
- preserves original formatting

---

# 🔍 Rule Syntax (ignore/filter)


Filters and ignore rules use the same syntax.

Paths are matched component-by-component.

Example:

```text
include/core/test.h
-> ["include/", "core/", "test.h"]
```

Directory components end with `/`.

---

## Logic

NOT:

```text
!condition
```

AND:

```text
condition1,condition2
```

OR:

```text
condition1 condition2
```

---

## Conditions

Exact match:

```text
name
name/
```

Ends with:

```text
*name
**name
```

Starts with:

```text
name*
name**
```

Contains:

```text
*name*
**name**
**name*
*name**
```

Scope:

| Syntax | Scope |
|----------|----------|
| `*` | current level |
| `**` | current level + deeper levels |

Names containing spaces may be quoted:

```text
"My Folder"/
```

---

## Examples

Recursive C++ files:

```text
**.cpp
```

Headers directly inside include:

```text
include/,*.h
```

Directories ending with build under include:

```text
include/**build/
```

Exclude test files:

```text
!**test.cpp
```

Typical source filter:

```text
src/ include/ .cpp .h
```

## 📌 Default Ignore Rules

```text
.vscode/
.git/
```

## 📌 Shortened filter syntax

```text
./file.cpp  -> file.cpp
/file.cpp   -> file.cpp
file.cpp    -> **file.cpp

./dir/      -> dir/
/dir/       -> dir/
dir/        -> **dir/
```

---

# ⚙️ .cgtconfig

CodeGather automatically creates `.cgtconfig` in the workspace directory.

Example:

```ini
[GENERAL]
output=cgt-result.txt
filePrefix=**

[IGNORE]
.vscode/
.git/
**.obj
**build/

[COLOR]
cpp=120,210,255
h=255,220,120

[TEMPLATE:cpp]
output=cpp-result.txt
filePrefix=**
src/
include/
.cpp
.h
```

---

