# html2tex-lib

**Lightning-fast HTML to LaTeX conversion in C/C++**  
Zero-dependency static library for robust document conversion across all platforms.

[![C99](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![C++11](https://img.shields.io/badge/C++-11-blue.svg)](https://en.cppreference.com/w/cpp/11)
[![Platforms](https://img.shields.io/badge/Platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](https://en.wikipedia.org/wiki/Cross-platform)

## 🚀 Quick Start

```bash
# Clone & build
git clone https://github.com/MateiIonutEduard/html2tex-lib.git && cd html2tex-lib
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release --parallel
```

## 💻 Usage
### C API

```c
#include "html2tex.h"
int main() {
  LaTeXConverter* converter = html2tex_create();
  char* latex = html2tex_convert(converter, "<p>This is <b>bold</b> text</p>");
  
  if (latex) {
      printf("%s\n", latex);
      free(latex);
  }

  html2tex_destroy(converter);
  return 0;
}
```

### C++ Wrapper

```cpp
#include "HtmlToLatexConverter.hpp"
using namespace std;

int main() {
  HtmlToLatexConverter converter;
  string latex = converter.convert("<p>This is <b>bold</b> text</p>");

  cout << latex << endl;
  return 0;
}
```

## 🔧 Integration

### Method 1: Direct File Inclusion
Copy these files to your project:
* `include/HtmlToLatexConverter.h`
* `source/HtmlToLatexConverter.cpp`
* Built static library from `bin/Release/`

### CMakeLists.txt:

```cmake
# Add C++ wrapper source
target_sources(your_target PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/HtmlToLatexConverter.cpp
)

# Include headers
target_include_directories(your_target PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/include
)

# Link against C library
target_link_libraries(your_target PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lib/html2tex
)
```

### Method 2: CMake Package

```cmake
find_package(html2tex REQUIRED)
target_link_libraries(your_target PUBLIC html2tex::html2tex)
```

### Method 3: Subdirectory

```cmake
add_subdirectory(third_party/html2tex-lib)
target_link_libraries(your_target PUBLIC html2tex)
target_include_directories(your_target PUBLIC 
    third_party/html2tex-lib/include
)
```

## 🖥️ Platform Support
* Windows: MSVC 2019+, MinGW-w64 (`.lib`/`.a`)
* Linux: GCC 9+, Clang 10+ (`.a`)
* Mac OSX: Clang (Xcode 12+) (`.a`)

## 🛠️ Advanced Build

```bash
# Linux/Mac OSX
cmake .. -DCMAKE_BUILD_TYPE=Release

# Windows (VS2022)
cmake .. -G "Visual Studio 17 2022" -A x64

# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Install system-wide
cmake --install . --prefix /usr/local
```

## 🎯 Why html2tex-lib?
* 🔄 Zero Dependencies - Pure C/C++, no external libs
* ⚡ High Performance - Optimized parsing and conversion
* 🎯 Cross-Platform - Consistent behavior everywhere
* 🔧 Dual Interface - C API + modern C++ wrapper
* 📦 Static Linking - Single binary deployment
