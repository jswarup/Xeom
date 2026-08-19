# Xeom Project Coding Rules & Standards

This project strictly adheres to high-performance Modern C++20 guidelines and formatting specifications.

## 1. Core Architectural Guidelines
1. **Structure of Arrays (SoA)**: Organize compute data in parallel contiguous arrays indexed by `uint32_t` (`SzType`) rather than pointer-heavy objects.
2. **Equivalence Class Partitioning**: Avoid pointer-based trees/maps (`std::map`, AVL trees). Store data in flat arrays, sort with custom comparators, and traverse contiguous equivalence classes via `UpperBound` binary searches.
3. **Type Erasure & Zero-Virtual Hot Paths**: Do not use `virtual` methods or `vtable`s in performance-critical paths. Use `TypeId` enums and template dispatch (`Crate`/`Conduct` pattern).
4. **Amortized Memory Allocations**: Pre-allocate arenas and contiguous containers to amortize allocation cost and eliminate frequent heap allocations in hot loops.
5. **Methods in Declared Traits**: Always put methods in declared traits and trait facades (`TRef`, `MTRef`, `IAccess`, `IArr`) rather than the concrete class object. Concrete data types remain plain, zero-virtual data representations.

## 2. Formatting & Code Conventions

### A. Indentation & Braces
- **Indentation**: 4 spaces, UNIX (LF) line endings. No tabs.
- **Brace Placement**:
  - Opening braces `{` are on a **newline** for `struct`, `class`, `enum`, and function/method definitions.
  - For control flow (`if`, `else`, `for`, `while`, `switch`, `do`), keep the opening brace `{` on the **same line**.

### B. Spacing in Parentheses & Brackets
- **Open Parenthesis**: An open parenthesis `(` must always be followed by a space, unless it encloses nothing (e.g., `( value)` or `( void)`, but `()` remains `()`).
- **Open Angular Bracket**: An open angular bracket `<` (used for generic parameters) must always be followed by a space, unless it encloses nothing (e.g., `Buff< T>` or `Result< ()>`, but `<>` remains `<>`). Less-than operators (`<`) are unaffected.
- **Template Declarations**: Every `template` keyword must always start on the very beginning of its own line above the declaration.

### C. File Banners & Section Dividers
- **Top Banner**: Begin every header and source file with a top banner comment:
  ```cpp
  // <filename.h> --------------------------------------------------------------------------------------------------
  #pragma once
  ```
- **Separators**: All `//------------------------------------------------------------------------------------------------------------------` separator lines MUST be padded with one blank line before and after.
- **Trailing Comments**: Align trailing comments cleanly (e.g., to column 72 when feasible).

### D. Naming, Statements & Attributes
- **Data Members at Top**: Always keep Data members (`m_...`) at the top of a `class` or `struct`, immediately following type aliases and before any constructors or method definitions.
- **One Statement Per Line**: Never place multiple statements on a single line. Each statement, variable declaration, and control flow action must occupy its own line.
- **Member Variables**: Use `m_` prefix for member variables (e.g., `m_SzPred`, `m_Count`, `m_Ptr`).
- **Local Variables**: Use `camelCase` for local variables.
- **Class & Struct Naming**: Use CamelCase with `Cw_` or `Xeom_` prefix where applicable (e.g., `VectorParams`, `Seg`).
- **Template Parameters**: Use `T` prefix for template type parameters (`typename TStor`, `typename TSzType`, `typename TTrait`).
- **Self Reference Macro**: Use `SELF` macro for `(*this)` where appropriate.
- **No Verbose Diagnostic Annotations**: Avoid diagnostic clutter and verbose attributes like `[[nodiscard]]`. Rely on `constexpr` and `noexcept` for genuine compiler optimizations without syntactic noise.

## 3. Build and Testing
After making changes, always verify them with:
- `tools/build/build.bat --debug` or `tools/build/build.bat --release`
- `tools/build/test.bat`

## 4. Git & Version Control Guidelines
- **Explicit Commit Directive Only**: NEVER execute a git commit automatically or proactively. Git commits must ONLY be performed when explicitly directed by the user (e.g., "commit", "git commit", "please commit").
