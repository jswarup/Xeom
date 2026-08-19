---
trigger: always_on
---

# Xeom Coding Guidelines & Format Specifications

This rule file specifies the core architectural paradigms, formatting guidelines, and memory-layout rules for Xeom.

---

## 1. Architectural Paradigms

### A. Structure of Arrays (SoA) Layout
- **Prefer SoA over AoA (Array of Structures)**: Store object properties in parallel, contiguous column vectors rather than arrays of heap-allocated pointers/objects.
- **Cache Locality & SIMD**: SoA enables the CPU L1/L2 cache prefetcher to operate at peak efficiency and allows Clang/LLVM to auto-vectorize loops (AVX2/AVX-512).
- **Index-based References**: Refer to elements by their container index (`uint32_t` / `SzType`) instead of raw C++ pointers.

### B. Sorted Arrays & Equivalence Class Lookups
- **Eliminate Pointer-based Trees & Maps**: Do not use `std::map`, AVL trees, or pointer-chasing node structures in high-performance lookup paths due to pointer overhead and cache misses.
- **Equivalence Class Partitioning**: Append data to flat arrays, sort them using custom comparators, and partition into equivalence classes defined by `!(a < b) && !(b < a)`.
- **`UpperBound` Traversal**: Use binary search (`std::upper_bound` / `UpperBound`) to jump directly to the start of the next equivalence partition.

### C. Zero-Virtual Polymorphism & Type Erasure
- **Avoid `virtual` Functions in Hot Loops**: Virtual method dispatches introduce `vtable` lookups, indirect branch mispredictions, and cache misses.
- **Type Erasure & Tagging**: Manage types using non-virtual structures, `TypeId` enum tagging, and template downcasting (`Conduct` / `Crate` pattern).

### D. Amortized Memory Allocations & Chore Scheduling
- **Pre-allocate Arenas**: Avoid frequent dynamic memory allocations (`malloc`/`free`/`new`/`delete`) inside execution paths. Pre-allocate containers to amortize allocation cost.

### E. Methods in Declared Traits
- **Trait-first Method Placement**: Always place polymorphic, functional, and higher-order operations in declared traits and trait facades (`TRef`, `MTRef`, `IAccess`, `IArr`) rather than bloating concrete class objects. Concrete objects remain zero-virtual, cache-friendly data structs.

---

## 2. Formatting & Code Conventions

### A. Indentation & Braces
- **Indentation**: 4 spaces, UNIX (LF) line endings. No tabs.
- **Brace Placement**:
  - Opening braces `{` are on a **newline** for `struct`, `class`, `enum`, and function/method definitions.
  - For control flow (`if`, `else`, `for`, `while`, `switch`, `do`), keep the opening brace `{` on the **same line**.

### B. Spacing in Parentheses & Brackets
- **Open Parenthesis**: An open parenthesis `(` must always be followed by a space, unless it encloses nothing (e.g., `( value)` or `( void)`, but `()` remains `()`).
- **Open Angular Bracket**: An open angular bracket `<` (used for generic parameters) must always be followed by a space, unless it encloses nothing (e.g., `Buff< T>` or `Result< ()>`, but `<>` remains `<>`). Less-than operators (`<`) are unaffected.
- **Template Declarations**: The `template` keyword must always start on the beginning of its own line above the declaration.

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
- **Self Reference Macro**: Support `SELF` macro for `(*this)` where appropriate.
- **No Verbose Diagnostic Annotations**: Avoid diagnostic clutter and verbose attributes like `[[nodiscard]]`. Rely on `constexpr` and `noexcept` for genuine compiler optimizations without syntactic noise.

---

## 3. Build & Testing
After making changes, always verify them with:
- `tools/build/build.bat --debug` or `tools/build/build.bat --release`
- `tools/build/test.bat`

---

## 4. Git & Version Control Guidelines
- **Explicit Commit Directive Only**: NEVER execute a git commit automatically or proactively. Git commits must ONLY be performed when explicitly directed by the user (e.g., "commit", "git commit", "please commit").
