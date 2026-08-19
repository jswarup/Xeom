// compute.h ------------------------------------------------------------------------------------------------------
#pragma once

#include "common_types.h"
#include "silo/seg.h"
#include "silo/arr.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom {

template < typename T>
concept FloatType = std::floating_point< T>;

struct BenchmarkResult
{
    size_t element_count{0};
    double elapsed_ms{0.0};
    double giga_ops_per_sec{0.0};
    double bandwidth_gb_per_sec{0.0};
    double verification_max_diff{0.0};
    bool   is_verified{false};
};

//-----------------------------------------------------------------------------------------------------------------

class EquivalenceEngine
{
public:
template < typename TStor, typename TSzType = uint32_t, typename TComp>
    static TSzType UpperPartitionBound( const std::vector< TStor> &arr, TSzType idx, TComp comp)
    {
        if ( idx >= static_cast< TSzType>( arr.size())) {
            return static_cast< TSzType>( arr.size());
        }
        auto it = std::upper_bound( arr.begin() + idx, arr.end(), arr[idx], comp);
        return static_cast< TSzType>( std::distance( arr.begin(), it));
    }

template < typename TStor, typename TSzType = uint32_t, typename TComp, typename TVisitor>
    static uint32_t TraverseEquivalenceClasses( const std::vector< TStor> &arr, TComp comp, TVisitor visit)
    {
        TSzType cur = 0;
        TSzType classes = 0;
        const auto sz = static_cast< TSzType>( arr.size());
        while ( cur < sz) {
            TSzType next = UpperPartitionBound< TStor, TSzType>( arr, cur, comp);
            visit( silo::Seg< TSzType>{.m_begin = cur, .m_size = next - cur}, arr[cur]);
            cur = next;
            ++classes;
        }
        return classes;
    }
};

//-----------------------------------------------------------------------------------------------------------------

class ComputeEngine
{
public:
template < FloatType T>
    static void vector_add( silo::Arr< const T> a, silo::Arr< const T> b, silo::Arr< T> c)
    {
        const size_t n = std::min( { a.Size(), b.Size(), c.Size() });
#pragma clang loop vectorize(enable) interleave(enable)
        for ( size_t i = 0; i < n; ++i) {
            c[i] = a[i] + b[i];
        }
    }

template < FloatType T>
    static void vector_fma( silo::Arr< const T> a, silo::Arr< const T> b, T scalar, silo::Arr< T> c)
    {
        const size_t n = std::min( { a.Size(), b.Size(), c.Size() });
#pragma clang loop vectorize(enable) interleave(enable)
        for ( size_t i = 0; i < n; ++i) {
            c[i] = std::fma( a[i], b[i], scalar);
        }
    }

template < FloatType T = float>
    static BenchmarkResult run_vector_add_benchmark( size_t n = 1'000'000, size_t iters = 50)
    {
        std::vector< T> a( n, static_cast< T>( 1.5));
        std::vector< T> b( n, static_cast< T>( 2.5));
        std::vector< T> c( n, static_cast< T>( 0));

        vector_add< T>( a, b, c);

        auto t0 = std::chrono::high_resolution_clock::now();
        for ( size_t i = 0; i < iters; ++i) {
            vector_add< T>( a, b, c);
        }
        auto t1 = std::chrono::high_resolution_clock::now();

        const double ms = std::chrono::duration< double, std::milli>( t1 - t0).count() / static_cast< double>( iters);
        double max_diff = 0.0;
        for ( size_t i = 0; i < n; ++i) {
            max_diff = std::max( max_diff, std::abs( static_cast< double>( c[i] - static_cast< T>( 4.0))));
        }

        const double dn = static_cast< double>( n);
        return {
            .element_count         = n,
            .elapsed_ms            = ms,
            .giga_ops_per_sec      = dn / ( ms * 1e-3) / 1e9,
            .bandwidth_gb_per_sec  = dn * sizeof( T) * 3.0 / ( ms * 1e-3) / 1e9,
            .verification_max_diff = max_diff,
            .is_verified           = ( max_diff < 1e-5)
        };
    }
};


} // namespace xeom
