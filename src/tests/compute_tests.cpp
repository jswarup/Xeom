// compute_tests.cpp -------------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "jeeves/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "ComputeEngine: SIMD vector_add (float)")
{
    constexpr size_t N = 1024;
    std::vector< float> a( N, 2.0f);
    std::vector< float> b( N, 3.5f);
    std::vector< float> c( N, 0.0f);
    xeom::ComputeEngine::vector_add< float>( a, b, c);
    bool ok = true;
    for ( size_t i = 0; i < N && ok; ++i) {
        if ( std::abs( c[i] - 5.5f) > 1e-6f) {
            ok = false;
        }
    }
    JEEVES_CHECK_MSG( ok, "vector_add precision (2.0 + 3.5 == 5.5)");
}

JEEVES_TEST( "ComputeEngine: vector_fma (double)")
{
    constexpr size_t N = 1024;
    std::vector< double> a( N, 4.0);
    std::vector< double> b( N, 2.5);
    std::vector< double> c( N, 0.0);
    xeom::ComputeEngine::vector_fma< double>( a, b, 1.25, c);
    bool ok = true;
    for ( size_t i = 0; i < N && ok; ++i) {
        if ( std::abs( c[i] - 11.25) > 1e-9) {
            ok = false;
        }
    }
    JEEVES_CHECK_MSG( ok, "vector_fma precision (4.0 * 2.5 + 1.25 == 11.25)");
}

JEEVES_TEST( "EquivalenceEngine: traverse equivalence classes")
{
    std::vector< int> data = { 1, 2, 2, 4, 4, 4, 5, 7, 7, 8 };
    uint32_t classes = 0;
    uint32_t elems = 0;
    xeom::EquivalenceEngine::TraverseEquivalenceClasses(
        data,
        []( int a, int b) { return a < b; },
        [&]( const xeom::silo::Seg< uint32_t> &seg, int) {
            ++classes;
            elems += seg.Size();
        }
    );
    JEEVES_CHECK_MSG( classes == 6, "EquivalenceEngine: 6 distinct classes");
    JEEVES_CHECK_MSG( elems == 10, "EquivalenceEngine: 10 total elements");
}
