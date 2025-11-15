# html2tex-lib

> **High-Performance HTML to LaTeX Conversion Engine**  
> A battle-tested, zero-dependency C/C++ static library for robust HTML to LaTeX document conversion.

[![CMake](https://img.shields.io/badge/CMake-3.16+-brightgreen.svg)](https://cmake.org/)
[![C Standard](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![C++ Standard](https://img.shields.io/badge/C++-11-blue.svg)](https://en.cppreference.com/w/cpp/11)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](https://en.wikipedia.org/wiki/Cross-platform)

## Overview

`html2tex-lib` is a production-grade library designed for developers who need reliable, high-performance HTML to LaTeX conversion. Built with a pure C core for maximum efficiency and wrapped with a modern C++ interface for developer convenience.

**Perfect for:**
- Document processing pipelines
- Academic publishing tools
- Content management systems
- Automated report generation

## Architecture

```css
html2tex-lib/
├── include/ # Public interfaces
│ ├── html2tex.h # C API (stable ABI)
│ └── HtmlToLatexConverter.hpp # C++ RAII wrapper
├── source/ # Implementation
│ ├── html_parser.c # Recursive descent HTML parser
│ ├── html2tex.c # Core conversion engine
│ ├── tex_gen.c # LaTeX generator
│ └── HtmlToLatexConverter.cpp # C++ binding layer
└── bin/Release/ # Built artifacts
├── html2tex.lib # Windows static library
└── libhtml2tex.a # Unix static library
```

### Design Philosophy

- **C Core**: Zero-overhead parsing and conversion
- **C++ Wrapper**: Modern RAII interface with exception safety
- **Static Linking**: Single binary deployment, no runtime dependencies
- **Cross-Platform**: Consistent behavior across all supported platforms

## Quick Start

### Prerequisites

- **CMake 3.16+** - Build system generator
- **C99 Compiler** - GCC, Clang, or MSVC
- **C++11 Compiler** - (Optional, for C++ wrapper)

### Build & Install

```bash
# Clone and configure
git clone <repository-url> && cd html2tex-lib
mkdir build && cd build

# Configure (auto-detects compiler)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release --parallel

# Install (optional)
cmake --install . --prefix /usr/local
