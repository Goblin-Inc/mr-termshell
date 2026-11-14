# mr-termshell

> 🌐 **Language:** **English** | [Русский (README_Ru.md)](README_Ru.md)

A fast terminal user interface (TUI) file manager written in **C++17** using the **FTXUI** library.

## ✨ Features
- 📁 Rapid filesystem navigation with file-type icons
- 🔍 Instant file filtering and sorting in the current directory
- 📋 Built-in clipboard operations (copy, move, delete, create files and directories)
- 🖼️ In-terminal preview for text files and images
- ⚡ Execute shell commands directly without leaving the file manager

## 🛠️ Building & Running

### Requirements
- CMake >= 3.14
- C++17-compatible compiler (GCC / Clang)
- Git (for fetching FTXUI automatically via CMake `FetchContent`)

### Build
```bash
mkdir -p build && cd build
cmake ..
cmake --build . -j$(nproc)
```

### Run
```bash
./build/mr-termshell
```
