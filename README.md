# html2tex

**Lightning-fast HTML → LaTeX conversion in pure C & modern C++**<br/>
Static, cross-platform, dependency-light libraries for fast document transformation.

[![C99](https://img.shields.io/badge/C-99-blue.svg)](https://en.wikipedia.org/wiki/C99)
[![C++14](https://img.shields.io/badge/C++-14-blue.svg)](https://en.cppreference.com/w/cpp/14)
[![Platforms](https://img.shields.io/badge/Platforms-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](https://en.wikipedia.org/wiki/Cross-platform)

## 📚 Architecture Overview

The repository provides two independent static libraries:

### 🔹 html2tex_c — Core C Library

Located in `/source` and `/include/html2tex.h`.

Features:
* High-performance *HTML5* subset parsing

* Ensure clean rollback on any allocation failure

* Fast and memory efficient **T**e**X**/**L**a**T**e**X** conversion

* Inline *CSS* 2.1 core support (colors, weight, alignment, spacing, etc.)

* Optional static `libcurl` integration (image downloading or external resources)

* Secure HTML DOM manipulation and LaTeX conversion with guaranteed memory integrity

* Boosts critical conversion & HTML parsing through real-time optimization

* Efficient and strong error system for advanced diagnosis

* No external dependencies besides optional `libcurl`

* Fully cross-platform: Windows OS, Linux OS, Mac OSX, FreeBSD

### 🔹 html2tex_cpp — Modern C++14 Wrapper

Located in `/source` and `/include/html2tex.hpp`.
Features:

* RAII interface over the C library

* Easier integration into C++ applications

* Clean OOP API (`HtmlParser`, `HtmlTeXConverter`)

* Advanced `HtmlDocument` API for efficient HTML DOM traversal

* Very efficient task pooling (`ImageManager`) for fast image downloading

* Exception Bridge for C/C++: Safe runtime error propagation across language boundaries
  
* Asynchronous batch download for queued images (lazy download)

* Statically links against `html2tex_c`<br/>

## 🚀 Quick Start

```bash
# Clone & build
git clone https://github.com/MateiIonutEduard/html2tex.git && cd html2tex
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel --config Release
```

Outputs are generated in:

```bash
/bin/<Debug|Release>/<x64|x86>/
```

## 💻 Usage Examples
### C API (`html2tex_c`)

```c
#include "html2tex.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* read_file(const char* filename) {
    FILE* file = fopen(filename, "r");

    if (!file) {
        fprintf(stderr, "Error: Cannot "
			"open file %s.\n", filename);
        return NULL;
    }

    /* get file size */
    fseek(file, 0, SEEK_END);

    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    /* allocate memory */
    char* content = malloc(file_size + 1);

    if (!content) {
        fprintf(stderr, "Error: Memory "
			"allocation failed.\n");
        fclose(file);
        return NULL;
    }

    /* read file content */
    size_t bytes_read = fread(content,
		1, file_size, file);
    content[bytes_read] = '\0';

    fclose(file);
    return content;
}

int main(int argc, char** argv) {
   /* check command line arguments */
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <output_image_directory>"
			" <html_file_path> <latex_file_path>\n", argv[0]);
        return 1;
    }

    LaTeXConverter* converter = html2tex_create();
    html2tex_set_image_directory(converter, argv[1]);

    html2tex_set_download_images(converter, 1);
    char* html_data = GetHTML(argv[2]);
    char* latex = html2tex_convert(converter, html_data);

    if (latex) {
        FILE* output = fopen(argv[3], "w");
		fwrite(latex, sizeof(char),
			strlen(latex), output);	
        free(latex); 
		fclose(output);
    }

    html2tex_destroy(converter);
    return 0;
}
```

### C++ API (`html2tex_cpp`)

```cpp
#include "html2tex.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    /* check command line arguments */
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <output_image_directory>"
			" <html_file_path> <latex_file_path>" << std::endl;
        return 1;
    }

	HtmlTeXConverter util;
	util.setDirectory(argv[1]);
	
	std::ifstream fin(argv[2]);
	HtmlParser parser; 
	
	fin >> parser;
	fin.close();
    
	std::ofstream fout(argv[3]);
	util.convertToFile(parser, fout);
	
	fout.close();
	return 0;
}
```

## 📁 Repository Layout

```css
html2tex/
├── include/
│   ├── html2tex.h             # C API
│   ├── css_properties.h       # C API
│   ├── dom_tree.h             # C API
│   ├── dom_tree_visitor.h     # C API
│   ├── html2tex_errors.h      # C API
│   ├── html2tex_processor.h   # C API
│   ├── html2tex_queue.h       # C API
│   ├── html2tex_stack.h       # C API
│   ├── image_storage.h        # C API
│   ├── image_utils.h          # C API
│   ├── string_buffer.h        # C API
│   ├── base_exception.hpp     # C++ API wrapper
│   ├── html2tex_defs.hpp      # C++ API wrapper
│   ├── html_exception.hpp     # C++ API wrapper
│   ├── html_parser.hpp        # C++ API wrapper
│   ├── htmltex_converter.hpp  # C++ API wrapper
│   ├── image_exception.hpp    # C++ API wrapper
│   ├── latex_exception.hpp    # C++ API wrapper
│   ├── ext/html_document.hpp   # C++ API wrapper
│   ├── ext/image_manager.hpp   # C++ API wrapper
│   └── html2tex.hpp           # C++ API wrapper
├── source/
│   ├── html2tex.c
│   ├── html2tex_css.c
│   ├── html2tex_dom_tree.c
│   ├── html2tex_dom_tree_visitor.c
│   ├── html2tex_errors.c
│   ├── html2tex_generator.c
│   ├── html2tex_image_storage.c
│   ├── html2tex_image_utils.c
│   ├── html2tex_processor.c
│   ├── html2tex_queue_utils.c
│   ├── html2tex_stack_utils.c
│   ├── html2tex_string_buffer.c
│   ├── html2tex_utils.c
│   ├── html_parser.c
│   ├── html_minify.c
│   ├── html_prettify.c
│   ├── tex_image_utils.c
│   ├── image_manager.cpp
│   ├── image_exception.cpp
│   ├── base_exception.cpp
│   ├── html_document.cpp
│   ├── latex_exception.cpp
│   ├── html_exception.cpp
│   ├── html_converter.cpp
│   └── html_parser.cpp
├── cmake/
│   └── html2texConfig.cmake.in
├── CMakeLists.txt
├── LICENSE README.md
└── README.md
```

## 🔧 Integration Options

### 1. Direct Inclusion (simplest)<br/>
Copy these interfaces (C/C++):<br/>
* `include/html2tex.h`
* `include/html2tex.hpp`<br/>

The built libraries:
* `html2tex_c.lib/.a`
* `html2tex_cpp.lib/.a`<br/>

Example CMake:

```cmake
target_include_directories(your_target PUBLIC third_party/html2tex/include)

target_link_libraries(your_target PUBLIC
    third_party/html2tex/bin/<Debug|Release>/<x64|x86>/html2tex_c
    third_party/html2tex/bin/<Debug|Release>/<x64|x86>/html2tex_cpp
)
```

### 2. CMake Package (recommended)<br/>

After installation:<br/>

```cmake
find_package(html2tex REQUIRED)

target_link_libraries(your_target
    PUBLIC html2tex::c
    PUBLIC html2tex::cpp
)
```

Targets provided:<br/>
* `html2tex::c` → C static library
* `html2tex::cpp` → C++ wrapper<br/>

Installed structure:<br/>

```bash
include/html2tex.h
include/html2tex.hpp
bin/<Debug|Release>/<x64|x86>/libhtml2tex_c.a
bin/<Debug|Release>/<x64|x86>/lib/libhtml2tex_cpp.a
cmake/html2tex/html2texConfig.cmake
```

### 3. Add Subdirectory<br/>

```cmake
add_subdirectory(third_party/html2tex)

target_link_libraries(your_target
    PUBLIC html2tex::c
    PUBLIC html2tex::cpp
)
```

## 🖥️ Platform Compatibility
* Windows: MSVC 2019+, MinGW-w64
* Linux: GCC 9+, Clang 10+
* Mac OSX: Clang (Xcode 12+)<br/>

Auto-detects:<br/>
* OS and Compiler
* libcurl availability
* Architecture (x86/x64)<br/>

Produces architecture-specific output directories.<br/>


## 🛠️ Installation

### Linux / Mac OSX:<br/>

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
sudo cmake --install . --prefix /usr/local
```

### Windows (Visual Studio 2022):<br/>

```cmd
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
cmake --install . --prefix "C:\Libraries\html2tex"
```

## 🎯 Why html2tex?
* 🔄 Lightweight Dependencies - Pure C/C++ core, uses libcurl for image downloads
* ⚡ High Performance - Optimized parsing and conversion with low memory usage
* 🧩 Extensible LaTeX Processor - Modular design for future rich conversion features
* 🚀 Fast DOM Navigation - High-speed traversal via direct HtmlDocument manipulation
* 🛡️ Guarded Error Recovery - Strong error system with automatic rollback for safe state restoration
* 📦 Asynchronous Batch Downloads - Queued lazy image downloading with background processing
* 🎯 Cross-Platform - Consistent behavior everywhere (Windows OS, Linux, Mac OSX, FreeBSD)
* 🧠 Memory Guaranteed - Safe DOM & LaTeX operations with integrity protection
* 🔧 Dual Interface - C99 API + modern C++14 wrapper
* 📦 Static Linking - Single binary deployment<br/>

## 🛡️ Compatibility Notes

- ✅ **Rich HTML5 & CSS 2.1** - Parses a substantial subset of elements and core properties
- 🔒 **Automatic Escaping** - Intelligent LaTeX character handling  
- 🏗️ **Nested Element Support** - Robust scope management
- 🌐 **Cross-Platform Consistency** - Identical behavior everywhere
- 🔧 **Extensible Mappings** - Custom conversions via source code
- 💡 **Graceful Degradation** - Unsupported elements preserved as content
- 🧠 **Memory Guaranteed** - Safe DOM & LaTeX operations with integrity protection
- 🧩 **Extensible LaTeX Processor** - Modular design for future rich conversion features
- 📦 **Asynchronous Batch Downloads** - Queued lazy image downloading with background processing
- 🛡️ **Guarded Error Recovery** - Strong error system with automatic rollback for safe state restoration
<br/>

**Note:** Unsupported **HTML** elements are gracefully ignored while preserving all content.
