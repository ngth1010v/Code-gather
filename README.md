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

### Example:

```text
Found files:
  Code-gather/
  ├── include/
1 │   └── main.h
  └── src/
2     └── main.cpp
```

- Numbered entries = selectable discovered files
- Each extension has a different color and can be adjusted in the .cgtconfig file.

### How to select:

```text
1 2
```
or
```text
1,2
```

---

# 📄 Result Output

Generated output format:

```text
*CODE:

**START test1.cpp
<...code...>
**END test1.cpp

**START test2.cpp
<...code...>
**END test2.cpp
```

- output is plain text
- deterministic format
- easy to parse later
- preserves original source formatting

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

Ignore syntax follows Git-style patterns.

### Supported rules:

```text
file.txt
/file.txt
*.txt
**.txt
dir/
**dir/
```

### Default Ignore Rules

```text
.vscode/
.git/
```

---

## 🧪 Ignore Rule Examples

### Ignore exact file

```text
file.txt
```

### Ignore root file

```text
/file.txt
```

### Ignore extension in current level

```text
*.txt
```

### Ignore extension recursively

```text
**.txt
```

### Ignore directory

```text
dir/
```

### Ignore directory recursively

```text
**dir/
```