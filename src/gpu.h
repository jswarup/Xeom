// gpu.h ----------------------------------------------------------------------------------------------------------
#pragma once

#define CL_TARGET_OPENCL_VERSION 300
#include "CL/cl.h"
#include "common_types.h"
#include "logger.h"
#include "vector_add_spv.hpp"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::gpu {

//-----------------------------------------------------------------------------------------------------------------
// RAII wrapper for OpenCL handles

template < typename T, cl_int ( *ReleaseFn)( T)>
class ClHandle
{
private:
    T m_Handle{nullptr};

public:
    ClHandle( void) = default;

    explicit ClHandle( T h)
        : m_Handle( h)
    {
    }

    ~ClHandle( void)
    {
        reset();
    }

    ClHandle( const ClHandle &) = delete;
    ClHandle &operator=( const ClHandle &) = delete;

    ClHandle( ClHandle &&o) noexcept
        : m_Handle( std::exchange( o.m_Handle, nullptr))
    {
    }

    ClHandle &operator=( ClHandle &&o) noexcept
    {
        if ( this != &o) {
            reset();
            m_Handle = std::exchange( o.m_Handle, nullptr);
        }
        return SELF;
    }

    T get( void) const noexcept
    {
        return m_Handle;
    }

    operator T( void) const noexcept
    {
        return m_Handle;
    }

    const T *address( void) const noexcept
    {
        return &m_Handle;
    }

    T *put( void) noexcept
    {
        reset();
        return &m_Handle;
    }

    void reset( T h = nullptr) noexcept
    {
        if ( m_Handle) {
            ReleaseFn( m_Handle);
        }
        m_Handle = h;
    }
};

using ScopedMem   = ClHandle< cl_mem, clReleaseMemObject>;
using ScopedKern  = ClHandle< cl_kernel, clReleaseKernel>;
using ScopedProg  = ClHandle< cl_program, clReleaseProgram>;
using ScopedQueue = ClHandle< cl_command_queue, clReleaseCommandQueue>;
using ScopedCtx   = ClHandle< cl_context, clReleaseContext>;
using ScopedEvent = ClHandle< cl_event, clReleaseEvent>;

//-----------------------------------------------------------------------------------------------------------------

struct DeviceInfo
{
    std::string    name;
    std::string    driver_version;
    std::string    opencl_c_version;
    std::string    il_version;
    cl_device_type device_type{0};
    cl_uint        compute_units{0};
    cl_uint        max_clock_mhz{0};
    cl_ulong       global_mem_bytes{0};
    cl_ulong       local_mem_bytes{0};
    size_t         max_workgroup_size{0};
    bool           supports_il_programs{false};
    bool           supports_spirv{false};
};

struct GpuBenchmarkResult
{
    size_t element_count{0};
    double kernel_time_ms{0.0};
    double wall_time_ms{0.0};
    double gflops{0.0};
    double bandwidth_gb_per_sec{0.0};
    float  max_abs_diff{0.0f};
    bool   verified{false};
};

//-----------------------------------------------------------------------------------------------------------------

class OpenCLContext
{
private:
    cl_platform_id m_Platform{nullptr};
    cl_device_id   m_Device{nullptr};
    ScopedCtx      m_Context;
    ScopedQueue    m_Queue;
    DeviceInfo     m_Info;
    bool           m_Ready{false};

public:
    bool discover_gpu( void)
    {
        cl_uint n_plat = 0;
        if ( clGetPlatformIDs( 0, nullptr, &n_plat) != CL_SUCCESS || n_plat == 0) {
            return false;
        }
        std::vector< cl_platform_id> plats( n_plat);
        clGetPlatformIDs( n_plat, plats.data(), nullptr);

        for ( auto p : plats) {
            cl_uint n_dev = 0;
            if ( clGetDeviceIDs( p, CL_DEVICE_TYPE_GPU, 0, nullptr, &n_dev) != CL_SUCCESS || n_dev == 0) {
                continue;
            }
            std::vector< cl_device_id> devs( n_dev);
            clGetDeviceIDs( p, CL_DEVICE_TYPE_GPU, n_dev, devs.data(), nullptr);
            for ( auto d : devs) {
                char buf[256]{};
                clGetDeviceInfo( d, CL_DEVICE_NAME, sizeof( buf), buf, nullptr);
                std::string sname( buf);
                if ( sname.find( "Iris") != std::string::npos || !m_Device) {
                    m_Platform = p;
                    m_Device   = d;
                    if ( sname.find( "Iris") != std::string::npos) {
                        break;
                    }
                }
            }
            if ( m_Device) {
                break;
            }
        }
        if ( !m_Device) {
            return false;
        }

        auto get_str = [&]( cl_device_info id) {
            size_t sz = 0;
            clGetDeviceInfo( m_Device, id, 0, nullptr, &sz);
            std::string s( sz, '\0');
            if ( sz > 0) {
                clGetDeviceInfo( m_Device, id, sz, s.data(), nullptr);
            }
            while ( !s.empty() && ( s.back() == ' ' || s.back() == '\0')) {
                s.pop_back();
            }
            return s;
        };

        auto get_val = [&]< typename V>( cl_device_info id, V &out) {
            clGetDeviceInfo( m_Device, id, sizeof( V), &out, nullptr);
        };

        m_Info.name             = get_str( CL_DEVICE_NAME);
        m_Info.driver_version   = get_str( CL_DRIVER_VERSION);
        m_Info.opencl_c_version = get_str( CL_DEVICE_OPENCL_C_VERSION);
        get_val( CL_DEVICE_TYPE, m_Info.device_type);
        get_val( CL_DEVICE_MAX_COMPUTE_UNITS, m_Info.compute_units);
        get_val( CL_DEVICE_MAX_CLOCK_FREQUENCY, m_Info.max_clock_mhz);
        get_val( CL_DEVICE_GLOBAL_MEM_SIZE, m_Info.global_mem_bytes);
        get_val( CL_DEVICE_LOCAL_MEM_SIZE, m_Info.local_mem_bytes);
        get_val( CL_DEVICE_MAX_WORK_GROUP_SIZE, m_Info.max_workgroup_size);

        char il_ver[256]{};
        if ( clGetDeviceInfo( m_Device, CL_DEVICE_IL_VERSION, sizeof( il_ver), il_ver, nullptr) == CL_SUCCESS) {
            m_Info.il_version = std::string( il_ver);
            while ( !m_Info.il_version.empty() && ( m_Info.il_version.back() == ' ' || m_Info.il_version.back() == '\0')) {
                m_Info.il_version.pop_back();
            }
            m_Info.supports_il_programs = true;
        }
        m_Info.supports_spirv = get_str( CL_DEVICE_EXTENSIONS).find( "cl_khr_spirv") != std::string::npos || m_Info.supports_il_programs;

        cl_int err = CL_SUCCESS;
        cl_context_properties ctx_props[] = {
            CL_CONTEXT_PLATFORM,
            reinterpret_cast< cl_context_properties>( m_Platform),
            0
        };
        m_Context.reset( clCreateContext( ctx_props, 1, &m_Device, nullptr, nullptr, &err));
        if ( err != CL_SUCCESS) {
            return false;
        }

        cl_queue_properties q_props[] = {
            CL_QUEUE_PROPERTIES,
            CL_QUEUE_PROFILING_ENABLE,
            0
        };
        m_Queue.reset( clCreateCommandQueueWithProperties( m_Context.get(), m_Device, q_props, &err));
        m_Ready = ( err == CL_SUCCESS);
        return m_Ready;
    }

    const DeviceInfo &info( void) const noexcept
    {
        return m_Info;
    }

    bool is_ready( void) const noexcept
    {
        return m_Ready;
    }

    cl_context context( void) const noexcept
    {
        return m_Context.get();
    }

    cl_command_queue queue( void) const noexcept
    {
        return m_Queue.get();
    }

    cl_device_id device( void) const noexcept
    {
        return m_Device;
    }
};

//-----------------------------------------------------------------------------------------------------------------

class GpuVectorAdd
{
private:
    const OpenCLContext &m_Ctx;
    ScopedProg           m_Program;
    ScopedKern           m_Kernel;
    bool                 m_Ready{false};

public:
    explicit GpuVectorAdd( const OpenCLContext &ctx)
        : m_Ctx( ctx)
    {
    }

    bool initialize( void)
    {
        if ( !m_Ctx.is_ready()) {
            return false;
        }
        cl_int err = CL_SUCCESS;
        m_Program.reset( clCreateProgramWithIL( m_Ctx.context(), k_vector_add_spv, k_vector_add_spv_size, &err));
        if ( err != CL_SUCCESS) {
            return false;
        }

        cl_device_id dev = m_Ctx.device();
        if ( clBuildProgram( m_Program.get(), 1, &dev, nullptr, nullptr, nullptr) != CL_SUCCESS) {
            return false;
        }

        m_Kernel.reset( clCreateKernel( m_Program.get(), "vector_add_clcxx", &err));
        m_Ready = ( err == CL_SUCCESS);
        return m_Ready;
    }

    GpuBenchmarkResult run( size_t count, size_t iterations = 10)
    {
        GpuBenchmarkResult res{.element_count = count};
        if ( !m_Ready) {
            return res;
        }

        std::vector< float> a( count);
        std::vector< float> b( count);
        std::vector< float> c( count, 0.0f);
        std::vector< float> ref( count);
        std::mt19937 rng( 42);
        std::uniform_real_distribution< float> dist( -100.0f, 100.0f);
        for ( size_t i = 0; i < count; ++i) {
            a[i]   = dist( rng);
            b[i]   = dist( rng);
            ref[i] = a[i] + b[i];
        }

        const size_t bytes = count * sizeof( float);
        cl_int err = CL_SUCCESS;
        ScopedMem d_a( clCreateBuffer( m_Ctx.context(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bytes, a.data(), &err));
        ScopedMem d_b( clCreateBuffer( m_Ctx.context(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bytes, b.data(), &err));
        ScopedMem d_c( clCreateBuffer( m_Ctx.context(), CL_MEM_WRITE_ONLY, bytes, nullptr, &err));
        if ( !d_a.get() || !d_b.get() || !d_c.get()) {
            return res;
        }

        cl_mem ma = d_a.get();
        cl_mem mb = d_b.get();
        cl_mem mc = d_c.get();
        common::VectorParams params {
            .count         = static_cast< uint32_t>( count),
            .op_type       = common::VectorOpType::Add,
            .scalar_factor = 1.0f,
            .flags         = 0
        };
        clSetKernelArg( m_Kernel.get(), 0, sizeof( cl_mem), &ma);
        clSetKernelArg( m_Kernel.get(), 1, sizeof( cl_mem), &mb);
        clSetKernelArg( m_Kernel.get(), 2, sizeof( cl_mem), &mc);
        clSetKernelArg( m_Kernel.get(), 3, sizeof( common::VectorParams), &params);

        const size_t lws = 256;
        const size_t gws = ( ( count + lws - 1) / lws) * lws;

        // warmup
        {
            ScopedEvent ev;
            clEnqueueNDRangeKernel( m_Ctx.queue(), m_Kernel.get(), 1, nullptr, &gws, &lws, 0, nullptr, ev.put());
            clWaitForEvents( 1, ev.address());
        }

        double total_ns = 0.0;
        auto tw0 = std::chrono::high_resolution_clock::now();
        for ( size_t it = 0; it < iterations; ++it) {
            ScopedEvent ev;
            if ( clEnqueueNDRangeKernel( m_Ctx.queue(), m_Kernel.get(), 1, nullptr, &gws, &lws, 0, nullptr, ev.put()) != CL_SUCCESS) {
                break;
            }
            clWaitForEvents( 1, ev.address());
            cl_ulong t0 = 0;
            cl_ulong t1 = 0;
            clGetEventProfilingInfo( ev.get(), CL_PROFILING_COMMAND_START, sizeof( t0), &t0, nullptr);
            clGetEventProfilingInfo( ev.get(), CL_PROFILING_COMMAND_END, sizeof( t1), &t1, nullptr);
            total_ns += static_cast< double>( t1 - t0);
        }
        auto tw1 = std::chrono::high_resolution_clock::now();

        clEnqueueReadBuffer( m_Ctx.queue(), d_c.get(), CL_TRUE, 0, bytes, c.data(), 0, nullptr, nullptr);

        const double di = static_cast< double>( iterations);
        const double avg_ms = ( total_ns / di) / 1e6;
        const double wall = std::chrono::duration< double, std::milli>( tw1 - tw0).count() / di;
        const double dn = static_cast< double>( count);

        float max_diff = 0.0f;
        for ( size_t i = 0; i < count; ++i) {
            max_diff = std::max( max_diff, std::abs( c[i] - ref[i]));
        }

        return {
            .element_count        = count,
            .kernel_time_ms       = avg_ms,
            .wall_time_ms         = wall,
            .gflops               = dn / ( avg_ms * 1e-3) / 1e9,
            .bandwidth_gb_per_sec = dn * sizeof( float) * 3.0 / ( avg_ms * 1e-3) / 1e9,
            .max_abs_diff         = max_diff,
            .verified             = ( max_diff < 1e-5f)
        };
    }

    bool is_ready( void) const noexcept
    {
        return m_Ready;
    }
};

} // namespace xeom::gpu
