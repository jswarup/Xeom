cmake_minimum_required(VERSION 3.25)

# Resolve Root Project Directory
get_filename_component(XEOM_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

project(Xeom
    VERSION 1.0.0
    DESCRIPTION "High-Performance Modern C++ Framework using Clang 20+ and CMake"
    LANGUAGES C CXX
)

# -----------------------------------------------------------------------------
# C++ Standard Requirements
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------------------------------------------------------
# Compilation Database Mirroring for clangd / IDEs
# -----------------------------------------------------------------------------
if (EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
    file(COPY_FILE
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${XEOM_ROOT}/compile_commands.json"
        ONLY_IF_DIFFERENT
    )
endif()

# -----------------------------------------------------------------------------
# Compiler Detection & Clang 20+ Validation
# -----------------------------------------------------------------------------
message(STATUS "--------------------------------------------------------")
message(STATUS " Configuring ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS " Root Directory    : ${XEOM_ROOT}")
message(STATUS " Target CXX Standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS " Compiler ID       : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS " Compiler Version  : ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "--------------------------------------------------------")

if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING "Recommended compiler for Xeom is Clang 20+. Detected: ${CMAKE_CXX_COMPILER_ID}")
endif()

if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "20.0")
    message(WARNING "Clang version 20+ is recommended. Detected version: ${CMAKE_CXX_COMPILER_VERSION}")
else()
    message(STATUS "Clang 20+ Version Check PASSED (${CMAKE_CXX_COMPILER_VERSION})")
endif()

# -----------------------------------------------------------------------------
# Output Directory Setup
# -----------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# -----------------------------------------------------------------------------
# OpenCL Configuration
# -----------------------------------------------------------------------------
set(OPENCL_LIB "${XEOM_ROOT}/thirdpty/lib/OpenCL.lib")

# -----------------------------------------------------------------------------
# SPIR-V Offline Compilation: vector_add.clcpp -> vector_add_spv.hpp
# -----------------------------------------------------------------------------
set(KERNEL_SOURCE "${XEOM_ROOT}/src/kernels/vector_add.clcpp")
set(SPV_OUTPUT    "${CMAKE_BINARY_DIR}/generated/vector_add.spv")
set(SPV_HEADER    "${CMAKE_BINARY_DIR}/generated/vector_add_spv.hpp")

get_filename_component(CLANG_BIN_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
find_program(CLANG_EXE clang PATHS "${CLANG_BIN_DIR}" NO_DEFAULT_PATH REQUIRED)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")

add_custom_command(
    OUTPUT "${SPV_OUTPUT}" "${SPV_HEADER}"
    COMMAND ${CMAKE_COMMAND}
        -DCLANG_EXE=${CLANG_EXE}
        -DKERNEL_INPUT=${KERNEL_SOURCE}
        -DINCLUDE_DIR=${XEOM_ROOT}/src
        -DSPV_OUTPUT=${SPV_OUTPUT}
        -DHEADER_OUTPUT=${SPV_HEADER}
        -P "${CMAKE_CURRENT_LIST_DIR}/spirv_embed.cmake"
    DEPENDS "${KERNEL_SOURCE}" "${XEOM_ROOT}/src/common_types.h" "${CMAKE_CURRENT_LIST_DIR}/spirv_embed.cmake"
    COMMENT "Compiling C++ for OpenCL 2021 kernel to SPIR-V and embedding bytecode"
    VERBATIM
)

add_custom_target(spirv_kernel DEPENDS "${SPV_HEADER}")

# -----------------------------------------------------------------------------
# Core Header-Only Library: xeom_core
# -----------------------------------------------------------------------------
add_library(xeom_core INTERFACE)
add_dependencies(xeom_core spirv_kernel)

target_include_directories(xeom_core INTERFACE
    "${XEOM_ROOT}/src"
    "${CMAKE_BINARY_DIR}/generated"
)

target_include_directories(xeom_core SYSTEM INTERFACE
    "${XEOM_ROOT}/thirdpty"
)

target_compile_features(xeom_core INTERFACE cxx_std_20)
target_link_libraries(xeom_core INTERFACE "${OPENCL_LIB}")

# Compiler-specific Flags function
function(xeom_set_compile_options target)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if (MSVC)
            target_compile_options(${target} PRIVATE
                /W4 /permissive- /EHsc /Zc:__cplusplus
                -Wno-gnu-anonymous-struct -Wno-nested-anon-types
                $<$<CONFIG:Release>:/O2 /Oi /Ot>
                $<$<CONFIG:Debug>:/Od /Zi>
            )
        else()
            target_compile_options(${target} PRIVATE
                -Wall -Wextra -Wpedantic -fcolor-diagnostics
                -Wno-gnu-anonymous-struct -Wno-nested-anon-types
                $<$<CONFIG:Release>:-O3 -march=native>
                $<$<CONFIG:Debug>:-O0 -g>
            )
        endif()
    elseif (MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /EHsc /Zc:__cplusplus)
    endif()
endfunction()

# -----------------------------------------------------------------------------
# Executable: xeom
# -----------------------------------------------------------------------------
add_executable(xeom "${XEOM_ROOT}/src/main.cpp")
target_link_libraries(xeom PRIVATE xeom_core)
xeom_set_compile_options(xeom)

# -----------------------------------------------------------------------------
# Testing: xeom_tests
# -----------------------------------------------------------------------------
enable_testing()
add_executable(xeom_tests "${XEOM_ROOT}/tests/test_main.cpp")
target_link_libraries(xeom_tests PRIVATE xeom_core)
xeom_set_compile_options(xeom_tests)

add_test(NAME CoreTests COMMAND xeom_tests)
