# =============================================================================
# Compile C++ for OpenCL 2021 kernel to SPIR-V and generate embedded C++ header
# =============================================================================
# Usage: cmake -P spirv_embed.cmake
#   Required vars (passed via -D):
#     CLANG_EXE     - Path to clang compiler
#     KERNEL_INPUT  - Path to .clcpp source
#     SPV_OUTPUT    - Path for .spv binary output
#     HEADER_OUTPUT - Path for generated .hpp header

# 1. Compile .clcpp -> .spv
set(CLANG_ARGS -cl-std=clc++2021 -target spirv64)
if(INCLUDE_DIR)
    list(APPEND CLANG_ARGS -I "${INCLUDE_DIR}")
endif()
list(APPEND CLANG_ARGS -c "${KERNEL_INPUT}" -o "${SPV_OUTPUT}")

execute_process(
    COMMAND "${CLANG_EXE}" ${CLANG_ARGS}
    RESULT_VARIABLE COMPILE_RESULT
    ERROR_VARIABLE COMPILE_ERROR
)

if(NOT COMPILE_RESULT EQUAL 0)
    message(FATAL_ERROR "SPIR-V compilation failed:\n${COMPILE_ERROR}")
endif()

message(STATUS "Compiled SPIR-V: ${SPV_OUTPUT}")

# 2. Read binary and generate C++ header
file(READ "${SPV_OUTPUT}" SPV_DATA HEX)
file(SIZE "${SPV_OUTPUT}" SPV_SIZE)

# Convert hex string to comma-separated 0xNN bytes
string(LENGTH "${SPV_DATA}" HEX_LEN)
set(BYTE_ARRAY "")
set(BYTES_PER_LINE 0)
math(EXPR LAST_IDX "${HEX_LEN} - 2")

foreach(IDX RANGE 0 ${LAST_IDX} 2)
    string(SUBSTRING "${SPV_DATA}" ${IDX} 2 BYTE)
    if(BYTE_ARRAY)
        string(APPEND BYTE_ARRAY ", ")
    endif()
    if(BYTES_PER_LINE EQUAL 16)
        string(APPEND BYTE_ARRAY "\n    ")
        set(BYTES_PER_LINE 0)
    endif()
    string(APPEND BYTE_ARRAY "0x${BYTE}")
    math(EXPR BYTES_PER_LINE "${BYTES_PER_LINE} + 1")
endforeach()

# Write the header
file(WRITE "${HEADER_OUTPUT}"
"#pragma once
// =============================================================================
// AUTO-GENERATED — DO NOT EDIT
// Embedded SPIR-V bytecode for C++ for OpenCL 2021 vector_add kernel
// Source: ${KERNEL_INPUT}
// Size: ${SPV_SIZE} bytes
// =============================================================================

#include <cstddef>
#include <cstdint>

inline constexpr unsigned char k_vector_add_spv[] = {
    ${BYTE_ARRAY}
};

inline constexpr size_t k_vector_add_spv_size = sizeof(k_vector_add_spv);
")

message(STATUS "Generated embedded header: ${HEADER_OUTPUT} (${SPV_SIZE} bytes)")
