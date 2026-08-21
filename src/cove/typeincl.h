// typeincl.h ------------------------------------------------------------------------------------------------------
#pragma once

#define SELF (*this)

#if defined( __OPENCL_CPP_VERSION__) || defined( __OPENCL_VERSION__)
#define XEOM_DEVICE_CODE 1
#define XEOM_HOST_CODE   0
#define XEOM_GLOBAL      global
#define XEOM_CONSTANT    constant
#define XEOM_LOCAL       local
typedef uint  uint32_t;
typedef int   int32_t;
typedef ulong uint64_t;
typedef long  int64_t;
#else
#define XEOM_DEVICE_CODE 0
#define XEOM_HOST_CODE   1
#define XEOM_GLOBAL
#define XEOM_CONSTANT
#define XEOM_LOCAL
#include "includes.h"
#endif

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::common {

inline constexpr uint32_t k_default_workgroup_size = 256;
inline constexpr uint32_t k_max_elements           = 1'000'000;
inline constexpr float    k_float_epsilon           = 1e-5f;

enum class VectorOpType : uint32_t
{
    Add      = 0,
    Subtract = 1,
    Multiply = 2,
    FMA      = 3
};

enum class ComputePrecision : uint32_t
{
    FP32 = 0,
    FP64 = 1,
    FP16 = 2
};

enum class TypeId : uint32_t
{
    Unknown = 0,
    Float32 = 1,
    Float64 = 2,
    Int32   = 3
};

struct alignas( 16) VectorParams
{
    uint32_t     count;
    VectorOpType op_type;
    float        scalar_factor;
    uint32_t     flags;
};

struct alignas( 16) TransformMatrix2D
{
    float m[4];
    float translate[2];
    float padding[2];
};

} // namespace xeom::common
