# Xeom: Modern High-Performance C++ Framework

Xeom is a high-performance C++ framework configured and optimized for **Clang 20+** and **CMake 4.2+** with **Ninja** on Windows.

---

## Toolchain & Environment Specifications

- **C++ Compiler**: Clang version **20.1.8** (`clang++.exe` / `clang-cl.exe`)
- **Build System**: CMake **4.2.3+** with **Ninja** generator
- **C++ Standard**: **C++20** (with C++23 / C++26 language feature readiness)
- **Target Platform**: Windows x86_64 (`x86_64-pc-windows-msvc`)

---

## Directory Layout

```
C:\Work\Taregna\Area51\Xiom\
├── .clang-format               # LLVM style formatting configuration
├── .clang-tidy                 # Clang static analysis rules
├── .gitignore                  # Git ignore rules for CMake, build, and IDE files
├── README.md                   # Project documentation
├── .vscode/
│   ├── settings.json           # VS Code C/C++ and CMake Tools settings
│   ├── tasks.json              # VS Code build and clean tasks
│   └── launch.json             # VS Code debugger configurations
├── src/                        # ALL first-party headers and code unified here
│   ├── logger.h                # C++20 std::format console logger with colors
│   ├── compute.h               # High-performance C++20 CPU compute & SIMD primitives
│   ├── gpu.h                   # Header-only GPU subsystem with RAII OpenCL handles
│   ├── xeom.h                  # Master framework header with version & compiler detection
│   ├── main.cpp                # Application entry point & CLI benchmark runner
│   └── kernels/
│       └── vector_add.clcpp    # C++ for OpenCL 2021 template kernel
├── tests/
│   └── test_main.cpp           # CPU & GPU test suite
├── thirdpty/
│   ├── CL/                     # Minimal Khronos OpenCL 3.0 C API headers
│   └── lib/
│       └── OpenCL.lib          # OpenCL 64-bit import library
└── tools/
    └── build/                  # Build scripts, presets, and CMake toolchain
        ├── build.bat           # Automated intelligent build script
        ├── run.bat             # 1-click execution runner
        ├── test.bat            # Automated test runner
        ├── CMakeLists.txt      # Root CMake configuration (Header-only library, Clang 20+, SPIR-V)
        ├── CMakePresets.json   # Presets for Ninja (Clang++ / Clang-CL) and VS2026
        └── spirv_embed.cmake   # Offline SPIR-V compilation and C++ embedding tool
```

---

## Quick Start

### 1. Build the Project
Run the automated build script (defaults to Release mode with Clang 20 + Ninja):
```cmd
tools\build\build.bat
```

#### Build Options:
```cmd
tools\build\build.bat --release      # Build Release (optimized)
tools\build\build.bat --debug        # Build Debug with symbols
tools\build\build.bat --clang-cl     # Build with clang-cl (MSVC driver) instead of clang++
tools\build\build.bat --clean        # Clean build directory before building
tools\build\build.bat --test         # Build and immediately run tests
tools\build\build.bat --vs           # Generate Visual Studio 2026 solution (ClangCL toolset)
```

### 2. Run the Application
```cmd
tools\build\run.bat
```

### 3. Run the Automated Tests
```cmd
tools\build\test.bat
```

---

## Using CMake Presets Directly

You can invoke CMake presets directly from any terminal:

```bash
# Configure Release with Clang 20 & Ninja
cmake -S tools/build --preset clang-ninja-release

# Build Release target
cmake --build build/clang-ninja-release

# Run Tests
ctest --test-dir build/clang-ninja-release --output-on-failure
```

Available Presets:
- `clang-ninja-release`: Clang++ 20.1.8 with Ninja in Release mode (`-O3 -march=native`)
- `clang-ninja-debug`: Clang++ 20.1.8 with Ninja in Debug mode (`-O0 -g`)
- `clang-cl-release`: Clang-CL 20.1.8 with Ninja in Release mode (`/O2 /Oi /Ot`)
- `clang-cl-debug`: Clang-CL 20.1.8 with Ninja in Debug mode (`/Od /Zi`)
- `vs2026-clang`: Visual Studio 2026 Solution with `ClangCL` toolset

---

## C++20 / Clang 20 Features Showcased

1. **Compiler Validation**:
   - `xeom::get_compiler_info()` validates at compile-time and runtime that Clang major version >= 20.
2. **Modern Standard Formatting**:
   - `xeom::Logger` uses `<format>` and `<source_location>` for zero-overhead, type-safe console logging with ANSI color coding.
3. **C++20 Concepts**:
   - `xeom::FloatType` concepts enforce strict compile-time type constraints.
4. **Vectorized Compute**:
   - `xeom::ComputeEngine` uses `Arr` and `#pragma clang loop vectorize(enable)` to demonstrate Clang 20 auto-vectorization and high-throughput execution.
