# textparser 🚀

[![Language: C](https://img.shields.io/badge/Language-C-blue?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Language: Rust](https://img.shields.io/badge/Language-Rust-orange?logo=rust&logoColor=white)](https://www.rust-lang.org)
[![Language: Python](https://img.shields.io/badge/Language-Python-3776AB?logo=python&logoColor=white)](https://www.python.org)
[![Language: Java](https://img.shields.io/badge/Language-Java-ED8B00?logo=openjdk&logoColor=white)](https://www.java.com)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Arch Linux AUR](https://img.shields.io/badge/Arch_Linux-AUR-1793D1?logo=arch-linux&logoColor=white)](https://aur.archlinux.org/packages/textparser)

A lightning-fast, multi-language **Concrete & Abstract Syntax Tree (CST/AST) generator** and syntax highlighter. Architected with a high-performance **C core engine** powered by **PCRE2** and **JSON-C**, `textparser` provides standalone native ports for **Rust**, **Python**, and **Java**.

It serves as a robust foundation for building linters, static analysis tools, compilers, and terminal utilities like the built-in `ccat` clone.

---

## ✨ Features

- **🌐 Massive Language Support:** Rich declarative and regex-based grammars for modern and classic languages.
- **⚡ High Performance:** Core tokenization, CST/AST construction, and Pratt expression parsing written in highly optimized C.
- **♻️ Incremental, CST/AST-Aware Re-parsing:** Sub-millisecond re-parsing that only re-lexes and splices modified regions, preserving node identity, tree structure, and memoized state across keystrokes.
- **🧬 Multi-Language Ecosystem:** Standalone native ports (Rust, Python, Java) with memory safety and full behavioral parity.
- **🎨 Built-in `ccat` Utility:** High-performance, syntax-highlighted terminal alternative to the standard `cat` command.

---

## 🧰 Tools

`textparser` provides two command-line utilities:

### `textparser`
The primary CLI tool for parsing source files, inspecting token streams, and generating syntax trees (CST/AST).

* **View colored syntax tree:**
  ```bash
  textparser main.c
  ```
* **Generate structured JSON representation:**
  ```bash
  textparser main.c --json
  ```
* **Export token stream with line/column positions:**
  ```bash
  textparser main.c --tokens
  ```
* **Test against a custom language definition:**
  ```bash
  textparser main.c --definition custom_def.json
  ```

### `ccat`
A lightning-fast, colorized replacement for the standard `cat` command that renders syntax-highlighted source code directly in the terminal.

* **View syntax-highlighted source code:**
  ```bash
  ccat main.rs
  ```

---

## 📚 Supported Languages

* **System & General:** C, C++, C#, Java, Go, Rust, Swift, Zig, C3, Jai, Ada, Assembly (x86/ARM), Pascal, Visual Basic, Fortran
* **Web & Data:** HTML, CSS, JavaScript, TypeScript, CFML (ColdFusion), JSON, SQL, Markdown
* **Scripting & Analytics:** Python, PHP, Bash/Shell, Perl, MATLAB, R, Scratch

---

## 🚀 Installation

### 📦 Linux Packages

#### **Arch Linux (AUR)**
```bash
yay -S textparser
# Or build from the latest git commit
yay -S textparser-git
```

#### **Ubuntu / Debian (PPA)**
```bash
sudo add-apt-repository ppa:bbarbulovski-gmail/textparser
sudo apt-get update
sudo apt-get install textparser
```

### 🐳 Docker
Pull and run the pre-configured container instantly:
```bash
docker pull bokic78/textparser
docker run --rm -v \$(pwd):/workspace bokic78/textparser --file /workspace/main.c
```

---

## 🛠️ Building from Source

### Prerequisites
Ensure you have `cmake`, `ninja` (optional but recommended), `libpcre2`, and `libjson-c` installed on your system.

### Compiling the C Core & CLI
```bash
git clone https://github.com/bokic/textparser.git
cd textparser
./build.sh
```
`textparser` is cross-platform and fully supports both little-endian (e.g. x86_64, aarch64) and big-endian architectures (e.g. IBM s390x).

### Windows builds

The Windows scripts use LLVM/Clang and Ninja. Build dependencies and the
project for one architecture, or omit the architecture to build both `x64`
and `arm64`:

```bat
cd windows
build_deps.bat [x64|arm64]
build.bat [x64|arm64]
build_zip.bat [x64|arm64]
```

Architecture-specific binaries and ZIP archives are written separately, for
example to `bin\arm64` and `windows\textparser-<version>-arm64.zip`.

---

## 📖 Documentation

For in-depth architectural details, parser specifications, grammar definitions, and porting contracts, see:
- [ARCHITECTURE.md](ARCHITECTURE.md) — Internal engine design, execution pipelines, schema-v2 grammar specification, and porting guidelines.
- [ROADMAP.md](ROADMAP.md) — Current development roadmap and upcoming features.

---

## 🤝 Contributing

Contributions are what make the open-source community an amazing place to learn, inspire, and create. 

1. **Fork** the project.
2. **Create** your feature branch (`git checkout -b feature/AmazingFeature`).
3. **Commit** your changes (`git commit -m 'Add some AmazingFeature'`).
4. **Push** to the branch (`git push origin feature/AmazingFeature`).
5. **Open a Pull Request**.

## 📄 License

This project is licensed under the **MIT License** - see the [LICENSE](LICENSE) file for details.

Developed with ❤️ by [Boris Barbulovski (bokic)](https://github.com/bokic).
