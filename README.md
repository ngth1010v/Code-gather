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

# ⚡ Usage

### Gather all readable source files

```bash
cgt
```

### Filter by extension

```bash
cgt .cpp .h
```

### Filter by directory

```bash
cgt include "dir with space"
```

### Gather a specific file

```bash
cgt src/main.cpp
```

### Replace output instead of append

```bash
cgt --replace
```

### Custom output file

```bash
cgt --output=result.txt
```

### Custom file prefix

```bash
cgt --filePrefix=##
```

---

# 🖥️ Console Output

### Example

```text
Found files:
  Code-gather/
  ├── include/
1 │   └── main.h
  └── src/
2     └── main.cpp
```

- Numbered entries = selectable discovered files
- Each extension has its own color
- Colors can be customized in `.cgtconfig`

### File Selection

```text
1 2
```

or

```text
1,2
```

---

# 📄 Result Output

Generated output format with --filePrefix=**:

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

---

# 🧱 Config Format

## 📌 Example

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
```

---

## 🚫 Ignore Rules

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

## 🧩 Ignore Rule Syntax

### Exact Component Match

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

### `*name`

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

### `**name`

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

## 📁 Directory Rules

Directory rules should end with `/`.

### Ignore exact directory level

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

### Ignore directory recursively

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

## 🧪 Ignore Rule Examples

### Ignore root `.git`

```text
.git/
```

---

### Ignore all `.obj` files recursively

```text
**.obj
```

---

### Ignore generated headers inside `include/`

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

### Ignore every `temp/` directory recursively

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

## 📌 Default Ignore Rules

```text
.vscode/
.git/
```