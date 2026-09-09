# textparser 🚀

[![Language: C](https://img.shields.io/badge/Language-C-blue?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Language: Rust](https://img.shields.io/badge/Language-Rust-orange?logo=rust&logoColor=white)](https://www.rust-lang.org)
[![Language: Python](https://img.shields.io/badge/Language-Python-3776AB?logo=python&logoColor=white)](https://www.python.org)
[![Language: Java](https://img.shields.io/badge/Language-Java-ED8B00?logo=openjdk&logoColor=white)](https://www.java.com)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Arch Linux AUR](https://img.shields.io/badge/Arch_Linux-AUR-1793D1?logo=arch-linux&logoColor=white)](https://aur.archlinux.org/packages/textparser)

A lightning-fast, multi-language **Abstract Syntax Tree (AST) generator** and syntax highlighter. Architected with a high-performance **C core engine** powered by **PCRE2** and **JSON-C**, `textparser` provides native, zero-overhead bindings and ports for **Rust**, **Python**, **Java**, and **WebAssembly (WASM)**.

It serves as a robust foundation for building linters, static analysis tools, compilers, and terminal utilities like the built-in `ccat` clone.

---

## ✨ Features

- **🌐 Massive Language Support:** Built-in regex-based grammars for modern and classic languages.
- **⚡ High Performance:** Core tokenization and AST construction written in highly optimized C.
- **🧬 Multi-Language Ecosystem:** Native language ports (Rust, Python, Java) manage underlying C memory safely.
- **🌐 WebAssembly Ready:** Compile to WASM for client-side code analysis directly in the browser.
- **🎨 Built-in `ccat` Utility:** A colorized alternative to the standard `cat` command for terminal code viewing.

---

## 📚 Supported Languages

`textparser` features rich tokenization and syntax parsing rules for:
* **System & General:** C, C++, C#, Java, Go, Rust, Swift, Zig, C3, V, Ada, Assembly (x86/ARM)
* **Web & Data:** HTML, CSS, JavaScript, TypeScript, JSON, XML, SQL, Markdown
* **Scripting:** Python, PHP, Bash/Shell

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
git clone https://github.com
cd textparser
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
sudo ninja install
```

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

## 💡 Quick Usage Examples

### 💻 CLI Usage (AST Generation)
Generate a clean, structured JSON representation of a source file's AST:
```bash
textparser main.c --json
```

### 🎨 Colorized Cat (`ccat`)
View your code with automatic, high-performance syntax highlighting in the terminal:
```bash
ccat main.rs
```

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

Developed with ❤️ by [Boris Barbulovski (bokic)](https://github.com).
