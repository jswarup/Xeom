// xeom.h ---------------------------------------------------------------------------------------------------------
#pragma once

#include "cove/typeincl.h"
#include "compute.h"
#include "swarm/gpu.h"
#include "cove/logger.h"
#include "silo/seg.h"
#include "silo/traits.h"
#include "silo/access.h"
#include "silo/arr.h"
#include "silo/buff.h"
#include "silo/stk.h"
#include "silo/stash.h"
#include "stalks/atm.h"
#include "stalks/node.h"
#include "stalks/work.h"
#include "heist/heist.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom {

inline constexpr std::string_view VERSION_STRING = "1.0.0";
inline constexpr std::string_view PROJECT_NAME   = "Xeom";

#if defined( __clang__)
inline constexpr std::string_view COMPILER_NAME = "Clang";
inline constexpr int  CLANG_MAJOR = __clang_major__;
inline constexpr int  CLANG_MINOR = __clang_minor__;
inline constexpr int  CLANG_PATCH = __clang_patchlevel__;
inline constexpr bool IS_CLANG    = true;
#else
inline constexpr std::string_view COMPILER_NAME = "Unknown";
inline constexpr int  CLANG_MAJOR = 0;
inline constexpr int  CLANG_MINOR = 0;
inline constexpr int  CLANG_PATCH = 0;
inline constexpr bool IS_CLANG    = false;
#endif

#if defined( _MSVC_LANG)
#define XEOM_CPLUSPLUS _MSVC_LANG
#else
#define XEOM_CPLUSPLUS __cplusplus
#endif

inline constexpr std::string_view get_cpp_standard_name( void)
{
#if XEOM_CPLUSPLUS > 202302L
    return "C++26 (Preview)";
#elif XEOM_CPLUSPLUS >= 202302L
    return "C++23";
#elif XEOM_CPLUSPLUS >= 202002L
    return "C++20";
#elif XEOM_CPLUSPLUS >= 201703L
    return "C++17";
#else
    return "C++14 or older";
#endif
}

struct CompilerInfo
{
    std::string_view name;
    std::string_view cpp_standard;
    int              major{0};
    int              minor{0};
    int              patch{0};
    bool             meets_clang20_req{false};

    std::string format( void) const
    {
        return std::format(
            "{} {}.{}.{} [{}] (Clang 20+: {})",
            name,
            major,
            minor,
            patch,
            cpp_standard,
            meets_clang20_req ? "YES" : "NO"
        );
    }
};

inline CompilerInfo get_compiler_info( void)
{
    return {
        COMPILER_NAME,
        get_cpp_standard_name(),
        CLANG_MAJOR,
        CLANG_MINOR,
        CLANG_PATCH,
        IS_CLANG && CLANG_MAJOR >= 20
    };
}

} // namespace xeom
