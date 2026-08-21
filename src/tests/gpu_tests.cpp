// gpu_tests.cpp -----------------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "jeeves/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "GPU: subsystem discovery and vector_add kernel")
{
    xeom::gpu::OpenCLContext ctx;
    const bool found = ctx.discover_gpu();
    JEEVES_CHECK_MSG( found, "GPU device discovery (Intel Iris Xe)");
    if ( !found) {
        return;
    }

    const auto &di = ctx.info();
    JEEVES_CHECK_MSG( !di.name.empty(), "GPU device name is not empty");
    JEEVES_CHECK_MSG( di.compute_units > 0, "GPU has compute units > 0");
    JEEVES_CHECK_MSG( di.supports_spirv, "GPU supports SPIR-V");
    JEEVES_CHECK_MSG( di.supports_il_programs, "GPU supports IL programs");

    xeom::gpu::GpuVectorAdd vadd( ctx);
    JEEVES_CHECK_MSG( vadd.initialize(), "clc++2021 SPIR-V program compiled on GPU");

    auto check = [&]( size_t n, size_t it, const char *lbl) {
        const auto r = vadd.run( n, it);
        JEEVES_CHECK_MSG( r.verified, lbl);
        JEEVES_CHECK_MSG( r.kernel_time_ms > 0, "GPU kernel time > 0 ms");
        JEEVES_CHECK_MSG( r.gflops > 0, "GPU throughput > 0 GFLOPS");
    };
    check( 1'024, 1, "GPU vector_add 1K elements");
    check( 65'536, 1, "GPU vector_add 64K elements");
    check( 1'000'000, 5, "GPU vector_add 1M elements");
    check( 5'000'000, 3, "GPU vector_add 5M elements");
}
