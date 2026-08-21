cmake_minimum_required(VERSION 3.25)
include(FetchContent)
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
set(KERNEL_SOURCE "${XEOM_ROOT}/src/swarm/kernels/vector_add.clcpp")
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
    DEPENDS "${KERNEL_SOURCE}" "${XEOM_ROOT}/src/cove/typeincl.h" "${CMAKE_CURRENT_LIST_DIR}/spirv_embed.cmake"
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
# Jeeves: Per-folder unit tests, linked into `xeom` and run via `xeom --test [filter]`
# -----------------------------------------------------------------------------
option(XEOM_TESTS "Compile Jeeves unit tests found alongside source (src/**/tests/*.cpp) into xeom" ON)

set(XEOM_JEEVES_TEST_SOURCES "")
if (XEOM_TESTS)
    file(GLOB_RECURSE XEOM_JEEVES_TEST_SOURCES CONFIGURE_DEPENDS
        "${XEOM_ROOT}/src/*/tests/*.cpp"
        "${XEOM_ROOT}/src/tests/*.cpp"
    )
    message(STATUS "Jeeves Unit Tests   : ENABLED (${XEOM_JEEVES_TEST_SOURCES})")
else()
    message(STATUS "Jeeves Unit Tests   : DISABLED")
endif()

# -----------------------------------------------------------------------------
# Fetch wxWidgets GUI Framework
# -----------------------------------------------------------------------------
set(wxBUILD_SAMPLES OFF CACHE BOOL "Disable wxWidgets samples" FORCE)
set(wxBUILD_TESTS OFF CACHE BOOL "Disable wxWidgets tests" FORCE)
set(wxBUILD_SHARED OFF CACHE BOOL "Build wxWidgets as static libraries" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 2.8 CACHE STRING "Allow old policies for wxWidgets" FORCE)

FetchContent_Declare(
    wxWidgets
    GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
    GIT_TAG v3.2.5
    PATCH_COMMAND powershell -Command "(Get-Content CMakeLists.txt) -replace 'VERSION 3.0', 'VERSION 3.5...3.25' | Set-Content CMakeLists.txt"
)
FetchContent_MakeAvailable(wxWidgets)

# -----------------------------------------------------------------------------
# Executable: xeom
# -----------------------------------------------------------------------------
set(FENST_SOURCES
    "${XEOM_ROOT}/src/fenst/fenst.cpp"
    "${XEOM_ROOT}/src/fenst/app.cpp"
    "${XEOM_ROOT}/src/fenst/main_frame.cpp"
)

add_executable(xeom "${XEOM_ROOT}/src/main.cpp" ${XEOM_JEEVES_TEST_SOURCES} ${FENST_SOURCES})
target_link_libraries(xeom PRIVATE xeom_core wx::core wx::base)
xeom_set_compile_options(xeom)
if (XEOM_TESTS)
    target_compile_definitions(xeom PRIVATE XEOM_TESTS_ENABLED)
endif()
