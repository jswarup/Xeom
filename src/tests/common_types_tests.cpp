// common_types_tests.cpp -------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "jeeves/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "Compiler: version requirements")
{
    auto info = xeom::get_compiler_info();
    JEEVES_CHECK_MSG( info.meets_clang20_req, "Compiler must be Clang version >= 20");
    JEEVES_CHECK_MSG( !info.cpp_standard.empty(), "C++ standard must be detected");
}

JEEVES_TEST( "CommonTypes: layout and constants")
{
    using namespace xeom::common;
    JEEVES_CHECK_MSG( sizeof( VectorParams) == 16, "sizeof(VectorParams) == 16 bytes");
    JEEVES_CHECK_MSG( alignof( VectorParams) == 16, "alignof(VectorParams) == 16 bytes");
    JEEVES_CHECK_MSG( sizeof( TransformMatrix2D) == 32, "sizeof(TransformMatrix2D) == 32 bytes");
    JEEVES_CHECK_MSG( sizeof( VectorOpType) == 4, "sizeof(VectorOpType) == 4 bytes");
    JEEVES_CHECK_MSG( sizeof( ComputePrecision) == 4, "sizeof(ComputePrecision) == 4 bytes");
    JEEVES_CHECK_MSG( k_default_workgroup_size == 256, "k_default_workgroup_size == 256");
}
