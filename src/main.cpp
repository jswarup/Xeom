// main.cpp -------------------------------------------------------------------------------------------------------
#include "xeom.h"

#include <cstdlib>

//-----------------------------------------------------------------------------------------------------------------

static void print_diagnostics( const xeom::CompilerInfo &info)
{
    xeom::Logger::info( "Initializing {} v{} runtime...", xeom::PROJECT_NAME, xeom::VERSION_STRING);
    xeom::Logger::info( "Compiler: {}", info.format());
    xeom::Logger::info( "=== Xeom System Diagnostics ===");
    xeom::Logger::info( "  Target Architecture : x86_64");
    xeom::Logger::info( "  Compiler            : {} {}.{}.{}", info.name, info.major, info.minor, info.patch);
    xeom::Logger::info( "  C++ Standard        : {}", info.cpp_standard);
    xeom::Logger::info( "  Clang 20+ Compliance: {}", info.meets_clang20_req ? "PASSED" : "FAILED");
#if defined( __AVX2__)
    xeom::Logger::info( "  SIMD                : AVX2");
#elif defined( __AVX__)
    xeom::Logger::info( "  SIMD                : AVX");
#else
    xeom::Logger::info( "  SIMD                : Standard SSE");
#endif
#if defined( NDEBUG)
    xeom::Logger::info( "  Build               : Release");
#else
    xeom::Logger::info( "  Build               : Debug");
#endif
    xeom::Logger::info(
        "  sizeof(VectorParams): {} bytes (alignof: {})",
        sizeof( xeom::common::VectorParams),
        alignof( xeom::common::VectorParams)
    );
    xeom::Logger::info( "  Workgroup Size      : {}", xeom::common::k_default_workgroup_size);
    xeom::Logger::info( "====================================");
}

static void run_cpu_benchmark( void)
{
    xeom::Logger::info( "Running Vector Add Benchmark (1,000,000 float elements)...");
    const auto r = xeom::ComputeEngine::run_vector_add_benchmark< float>( 1'000'000, 100);
    xeom::Logger::info(
        "  Avg Time  : {:.4f} ms | {:.2f} GFLOPS | {:.2f} GB/s | MaxDiff: {:.6f}",
        r.elapsed_ms,
        r.giga_ops_per_sec,
        r.bandwidth_gb_per_sec,
        r.verification_max_diff
    );
    if ( r.is_verified) {
        xeom::Logger::success( "CPU Verification PASSED!");
    } else {
        xeom::Logger::error( "CPU Verification FAILED!");
    }
}

static void run_gpu_benchmark( size_t vec_sz, size_t iters)
{
    xeom::Logger::info( "\n--- GPU Subsystem (clc++2021 / Intel Iris Xe) ---");
    xeom::gpu::OpenCLContext gpu;
    if ( !gpu.discover_gpu()) {
        xeom::Logger::warn( "GPU unavailable. Skipping.");
        return;
    }

    const auto &di = gpu.info();
    xeom::Logger::success( "GPU discovered: {}", di.name);
    xeom::gpu::GpuVectorAdd vadd( gpu);
    if ( !vadd.initialize()) {
        xeom::Logger::error( "GPU kernel init failed.");
        return;
    }

    xeom::Logger::success( "clc++2021 SPIR-V kernel loaded.");
    xeom::Logger::info(
        "  Device: {} | CUs: {} | Mem: {} MB | SPIR-V: {}",
        di.name,
        di.compute_units,
        di.global_mem_bytes / ( 1024 * 1024),
        di.supports_spirv ? "YES" : "NO"
    );

    const auto r = vadd.run( vec_sz, iters);
    xeom::Logger::info(
        "  Kernel: {:.4f} ms | Wall: {:.4f} ms | {:.2f} GFLOPS | {:.2f} GB/s | MaxDiff: {:.6f}",
        r.kernel_time_ms,
        r.wall_time_ms,
        r.gflops,
        r.bandwidth_gb_per_sec,
        r.max_abs_diff
    );
    if ( r.verified) {
        xeom::Logger::success( "GPU Verification PASSED!");
    } else {
        xeom::Logger::error( "GPU Verification FAILED! MaxDiff: {:.6f}", r.max_abs_diff);
    }
}

//-----------------------------------------------------------------------------------------------------------------

int main( int argc, char *argv[]) noexcept
{
    try {
        xeom::Logger::success( "=================================================");
        xeom::Logger::success( "  Xeom C++ Core Engine (Clang 20+ / CMake)       ");
        xeom::Logger::success( "  clc++2021 GPU Computing on Intel Iris Xe       ");
        xeom::Logger::success( "=================================================");

        size_t vec_sz = 1'000'000;
        size_t iters = 10;
        bool   run_cpu = true;
        bool   run_gpu_flag = true;

        for ( int i = 1; i < argc; ++i) {
            const std::string_view arg = argv[i];
            if ( arg == "--size" && i + 1 < argc) {
                vec_sz = static_cast< size_t>( std::strtoll( argv[++i], nullptr, 10));
            } else if ( arg == "--iter" && i + 1 < argc) {
                iters = static_cast< size_t>( std::strtoll( argv[++i], nullptr, 10));
            } else if ( arg == "--cpu-only") {
                run_gpu_flag = false;
            } else if ( arg == "--gpu-only") {
                run_cpu = false;
            } else if ( arg == "--help" || arg == "-h") {
                xeom::Logger::info( "Usage: xeom [--size <N>] [--iter <N>] [--cpu-only] [--gpu-only]");
                return 0;
            }
        }

        print_diagnostics( xeom::get_compiler_info());
        if ( run_cpu) {
            run_cpu_benchmark();
        }
        if ( run_gpu_flag) {
            run_gpu_benchmark( vec_sz, iters);
        }

        xeom::Logger::success( "Xeom execution completed successfully.");
        return 0;
    } catch ( const std::exception &ex) {
        xeom::Logger::error( "Fatal: {}", ex.what());
        return 1;
    } catch (...) {
        return 1;
    }
}
