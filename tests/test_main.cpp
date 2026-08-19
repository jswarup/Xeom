// test_main.cpp --------------------------------------------------------------------------------------------------
#include "xeom.h"

static int g_failed = 0;

#define TEST_ASSERT( cond, msg)                                                                     \
    do {                                                                                            \
        if ( !( cond)) {                                                                            \
            xeom::Logger::error( "TEST FAILED: {} ({}:{})", msg, __FILE__, __LINE__);               \
            g_failed++;                                                                             \
        } else {                                                                                    \
            xeom::Logger::success( "TEST PASSED: {}", msg);                                         \
        }                                                                                           \
    } while ( 0)

//-----------------------------------------------------------------------------------------------------------------

void test_compiler_version( void)
{
    auto info = xeom::get_compiler_info();
    TEST_ASSERT( info.meets_clang20_req, "Compiler must be Clang version >= 20");
    TEST_ASSERT( !info.cpp_standard.empty(), "C++ standard must be detected");
}

void test_common_types( void)
{
    using namespace xeom::common;
    TEST_ASSERT( sizeof( VectorParams) == 16, "sizeof(VectorParams) == 16 bytes");
    TEST_ASSERT( alignof( VectorParams) == 16, "alignof(VectorParams) == 16 bytes");
    TEST_ASSERT( sizeof( TransformMatrix2D) == 32, "sizeof(TransformMatrix2D) == 32 bytes");
    TEST_ASSERT( sizeof( VectorOpType) == 4, "sizeof(VectorOpType) == 4 bytes");
    TEST_ASSERT( sizeof( ComputePrecision) == 4, "sizeof(ComputePrecision) == 4 bytes");
    TEST_ASSERT( k_default_workgroup_size == 256, "k_default_workgroup_size == 256");
}

void test_xeom_paradigms( void)
{
    constexpr uint32_t N = 1024;
    std::vector< float> a( N, 10.0f);
    std::vector< float> b( N, 20.0f);
    std::vector< float> c( N, 0.0f);
    xeom::ComputeEngine::vector_add< float>( a, b, c);
    bool addOk = true;
    for ( uint32_t i = 0; i < N && addOk; ++i) {
        if ( std::abs( c[i] - 30.0f) > 1e-6f) {
            addOk = false;
        }
    }
    TEST_ASSERT( addOk, "SIMD vector addition (10.0 + 20.0 == 30.0)");

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
    TEST_ASSERT( classes == 6, "EquivalenceEngine: 6 distinct classes");
    TEST_ASSERT( elems == 10, "EquivalenceEngine: 10 total elements");
}

void test_vector_add_cpu( void)
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
    TEST_ASSERT( ok, "vector_add precision (2.0 + 3.5 == 5.5)");
}

void test_vector_fma_cpu( void)
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
    TEST_ASSERT( ok, "vector_fma precision (4.0 * 2.5 + 1.25 == 11.25)");
}

void test_gpu_subsystem( void)
{
    xeom::gpu::OpenCLContext ctx;
    const bool found = ctx.discover_gpu();
    TEST_ASSERT( found, "GPU device discovery (Intel Iris Xe)");
    if ( !found) {
        return;
    }

    const auto &di = ctx.info();
    TEST_ASSERT( !di.name.empty(), "GPU device name is not empty");
    TEST_ASSERT( di.compute_units > 0, "GPU has compute units > 0");
    TEST_ASSERT( di.supports_spirv, "GPU supports SPIR-V");
    TEST_ASSERT( di.supports_il_programs, "GPU supports IL programs");

    xeom::gpu::GpuVectorAdd vadd( ctx);
    TEST_ASSERT( vadd.initialize(), "clc++2021 SPIR-V program compiled on GPU");

    auto check = [&]( size_t n, size_t it, const char *lbl) {
        const auto r = vadd.run( n, it);
        TEST_ASSERT( r.verified, lbl);
        TEST_ASSERT( r.kernel_time_ms > 0, "GPU kernel time > 0 ms");
        TEST_ASSERT( r.gflops > 0, "GPU throughput > 0 GFLOPS");
    };
    check( 1'024, 1, "GPU vector_add 1K elements");
    check( 65'536, 1, "GPU vector_add 64K elements");
    check( 1'000'000, 5, "GPU vector_add 1M elements");
    check( 5'000'000, 3, "GPU vector_add 5M elements");
}

//-----------------------------------------------------------------------------------------------------------------
// C++20 Concepts — precise compile-time diagnostics for Trait compliance

template < typename T>
concept CDrawable = requires( const T &obj, std::string &out) {
    { obj.Draw( out) } -> std::same_as< void>;
    { obj.GetArea() }  -> std::convertible_to< float>;
};

template < typename T>
concept CScalable = requires( T &obj, float factor) {
    { obj.Scale( factor) } -> std::same_as< void>;
};

// Trait definitions — VTable + Concept-constrained Bind<T>

struct DrawableTrait
{
    struct VTable
    {
        void  ( *Draw)( const void *self, std::string &out);
        float ( *GetArea)( const void *self);
    };

template < typename T>
        requires CDrawable< T>
    static constexpr VTable Bind( void) noexcept
    {
        return {
            .Draw    = []( const void *s, std::string &o) { static_cast< const T *>( s)->Draw( o); },
            .GetArea = []( const void *s) -> float        { return static_cast< const T *>( s)->GetArea(); }
        };
    }
};

struct ScalableTrait
{
    struct VTable
    {
        void ( *Scale)( void *self, float factor);
    };

template < typename T>
        requires CScalable< T>
    static constexpr VTable Bind( void) noexcept
    {
        return {
            .Scale = []( void *s, float f) { static_cast< T *>( s)->Scale( f); }
        };
    }
};

// Ergonomic Trait Facades — 16-byte zero-overhead wrappers providing direct OOP syntax

struct Drawable : public xeom::silo::TRef< DrawableTrait>
{
    using Base = xeom::silo::TRef< DrawableTrait>;
    using Base::Base;

    constexpr Drawable( const Base &ref) noexcept
        : Base( ref)
    {
    }

    void Draw( std::string &out) const
    {
        m_Ops->Draw( m_Ptr, out);
    }

    float GetArea( void) const
    {
        return m_Ops->GetArea( m_Ptr);
    }
};

struct Scalable : public xeom::silo::MTRef< ScalableTrait>
{
    using Base = xeom::silo::MTRef< ScalableTrait>;
    using Base::Base;

    constexpr Scalable( const Base &ref) noexcept
        : Base( ref)
    {
    }

    void Scale( float factor) const
    {
        m_Ops->Scale( m_Ptr, factor);
    }
};

// Concrete types — no base class, no virtual, plain data.

struct TestCircle
{
    float m_radius{1.0f};

    void Draw( std::string &out) const
    {
        out = "Circle(r=" + std::to_string( m_radius) + ")";
    }

    float GetArea( void) const
    {
        return 3.14159265f * m_radius * m_radius;
    }

    void Scale( float f)
    {
        m_radius *= f;
    }
};

struct TestRectangle
{
    float m_width{2.0f};
    float m_height{3.0f};

    void Draw( std::string &out) const
    {
        out = "Rect(" + std::to_string( m_width) + "x" + std::to_string( m_height) + ")";
    }

    float GetArea( void) const
    {
        return m_width * m_height;
    }

    void Scale( float f)
    {
        m_width  *= f;
        m_height *= f;
    }
};

void test_silo_traits( void)
{
    using namespace xeom::silo;
    using DRef = TRef< DrawableTrait>;
    using DBox = TPtr< DrawableTrait, 48>;

    // Static size & efficiency guarantees
    static_assert( sizeof( DRef) == 2 * sizeof( void *), "TRef must be exactly 2 pointers (16 bytes)");
    static_assert( sizeof( Drawable) == 2 * sizeof( void *), "Facade must have zero size overhead (16 bytes)");
    static_assert( sizeof( DBox) == 64, "TPtr with 48B SBO must fit exactly in one 64-byte cache line");
    static_assert( std::is_trivially_copyable_v< DRef>, "TRef must be trivially copyable for register passing");
    static_assert( CDrawable< TestCircle>, "TestCircle satisfies CDrawable concept");
    static_assert( CDrawable< TestRectangle>, "TestRectangle satisfies CDrawable concept");

    TestCircle    circle{.m_radius = 2.0f};
    TestRectangle rect{.m_width = 4.0f, .m_height = 5.0f};

    // 1. Construct fat pointers (implicit conversion)
    DRef refC = circle;
    DRef refR = rect;
    TEST_ASSERT( refC.IsValid(), "TraitRef Circle is valid");
    TEST_ASSERT( refR.IsValid(), "TraitRef Rect is valid");

    // 2. Direct Ergonomic Facade Syntax (ref.Draw(), ref.GetArea())
    Drawable drawCircle = circle;
    Drawable drawRect   = rect;
    std::string facadeOut;
    drawCircle.Draw( facadeOut);
    TEST_ASSERT( facadeOut.find( "Circle") != std::string::npos, "Facade Draw on Circle");
    drawRect.Draw( facadeOut);
    TEST_ASSERT( facadeOut.find( "Rect") != std::string::npos, "Facade Draw on Rectangle");
    TEST_ASSERT( std::abs( drawCircle.GetArea() - 12.56637f) < 1e-3f, "Facade GetArea on Circle");
    TEST_ASSERT( std::abs( drawRect.GetArea() - 20.0f) < 1e-5f, "Facade GetArea on Rectangle");

    // 3. Generic Invoke<&VTable::Fn>(args...)
    std::string out;
    refC.Invoke< &DrawableTrait::VTable::Draw>( out);
    TEST_ASSERT( out.find( "Circle") != std::string::npos, "Invoke Draw on Circle");

    refR.Invoke< &DrawableTrait::VTable::Draw>( out);
    TEST_ASSERT( out.find( "Rect") != std::string::npos, "Invoke Draw on Rectangle");

    float areaC = refC.Invoke< &DrawableTrait::VTable::GetArea>();
    TEST_ASSERT( std::abs( areaC - 12.56637f) < 1e-3f, "Invoke GetArea on Circle");

    float areaR = refR.Invoke< &DrawableTrait::VTable::GetArea>();
    TEST_ASSERT( std::abs( areaR - 20.0f) < 1e-5f, "Invoke GetArea on Rectangle");

    // 4. Iterate heterogeneous array
    Drawable drawables[] = { drawCircle, drawRect };
    float total = 0.0f;
    for ( const auto &d : drawables) {
        total += d.GetArea();
    }
    TEST_ASSERT( std::abs( total - 32.56637f) < 1e-3f, "Batch iteration total area with Facade");

    // 5. Safe downcast
    TEST_ASSERT( refC.As< TestCircle>() != nullptr, "Downcast Circle succeeds");
    TEST_ASSERT( refC.As< TestRectangle>() == nullptr, "Downcast mismatch returns nullptr");

    // 6. Mutable Facade / MTRef — mutation through fat pointer
    Scalable mutC( circle);
    mutC.Scale( 3.0f);
    TEST_ASSERT( circle.m_radius == 6.0f, "Scalable facade mutated Circle radius");

    // 7. TraitMeta & TPtr — owning SBO
    {
        const TraitMeta< DrawableTrait> *metaCircle = TraitMeta< DrawableTrait>::For< TestCircle>();
        TEST_ASSERT( metaCircle != nullptr, "TraitMeta singleton for Circle is non-null");
        TEST_ASSERT( ( metaCircle == ObjMeta< DrawableTrait, TestCircle>::Get()), "TraitMeta matches ObjMeta singleton");
        TEST_ASSERT( metaCircle->m_Size == sizeof( TestCircle), "TraitMeta size matches TestCircle");
        TEST_ASSERT( metaCircle->IsInline( 48), "TestCircle fits in 48B SBO");

        TPtr< DrawableTrait> box( TestCircle{.m_radius = 5.0f});
        TEST_ASSERT( box.IsValid(), "TPtr constructed with SBO");

        float boxArea = box.Invoke< &DrawableTrait::VTable::GetArea>();
        TEST_ASSERT( std::abs( boxArea - 78.5398f) < 1e-2f, "TPtr Invoke GetArea");

        auto box2 = std::move( box);
        TEST_ASSERT( !box.IsValid(), "Moved-from TPtr is empty");
        TEST_ASSERT( box2.IsValid(), "Moved-to TPtr is valid");
    }

    // 8. TraitMeta — Trait-Scoped TypeId dispatch + Jump tables
    {
        // Trait-scoped TypeId lookup
        uint32_t circleId = TraitMeta< DrawableTrait>::Id< TestCircle>();
        uint32_t rectId   = TraitMeta< DrawableTrait>::Id< TestRectangle>();
        TEST_ASSERT( circleId == 0, "Circle TypeId under DrawableTrait is 0");
        TEST_ASSERT( rectId == 1, "Rect TypeId under DrawableTrait is 1");
        TEST_ASSERT( TraitMeta< DrawableTrait>::Count() == 2, "DrawableTrait has 2 registered types");

        // Aligned store buffer
        alignas( std::max_align_t) std::byte store[64]{};
        uint32_t tid = TraitMeta< DrawableTrait>::Emplace< TestCircle>( store, 10.0f);
        TEST_ASSERT( tid == 0, "Emplace returns correct TypeId");

        // O(1) Trait lookup directly from TraitMeta: TraitMeta<DrawableTrait>::Ref
        TRef< DrawableTrait> cref = TraitMeta< DrawableTrait>::Ref( store, tid);
        TEST_ASSERT( cref.IsValid(), "TraitMeta::Ref produces valid TRef");

        // Can convert directly to Facade for clean syntax
        Drawable drawCircle = cref;
        float circleArea = drawCircle.GetArea();
        TEST_ASSERT( std::abs( circleArea - 314.159f) < 0.1f, "TraitMeta dispatched GetArea on Circle(r=10)");

        // Demonstrate multi-trait view via ScalableTrait
        uint32_t scaleTid = TraitMeta< ScalableTrait>::Id< TestCircle>();
        MTRef< ScalableTrait> mutCircle = TraitMeta< ScalableTrait>::MutRef( store, scaleTid);
        mutCircle.Invoke< &ScalableTrait::VTable::Scale>( 2.0f);
        TEST_ASSERT( std::abs( drawCircle.GetArea() - 1256.637f) < 0.5f, "Mutated Circle(r=20) via ScalableTrait");

        // Type-safe destruction via TraitMeta TypeId jump table
        TraitMeta< DrawableTrait>::Destroy( store, tid);

        tid = TraitMeta< DrawableTrait>::Emplace< TestRectangle>( store, 7.0f, 3.0f);
        cref = TraitMeta< DrawableTrait>::Ref( store, tid);
        float rectArea = cref.Invoke< &DrawableTrait::VTable::GetArea>();
        TEST_ASSERT( std::abs( rectArea - 21.0f) < 1e-5f, "TraitMeta dispatched GetArea on Rect(7x3)");
        TraitMeta< DrawableTrait>::Destroy( store, tid);

        // Out-of-bounds TypeId returns null vtable
        TEST_ASSERT( TraitMeta< DrawableTrait>::VTable( 99) == nullptr, "TraitMeta returns nullptr for invalid TypeId");
    }

    // 7. Trait Composition & TraitBundle (Rust-like trait composition)
    {
        TestCircle circleObj{.m_radius = 5.0f};

        // Construct composite fat pointer (BundleRef)
        BundleRef< DrawableTrait, ScalableTrait> bundleRef = circleObj;
        TEST_ASSERT( bundleRef.IsValid(), "BundleRef is valid for composite traits");

        // Direct call via inherited VTable fields
        std::string desc;
        bundleRef->Draw( bundleRef.m_Ptr, desc);
        TEST_ASSERT( desc.find( "Circle") != std::string::npos, "BundleRef dispatches Draw");
        TEST_ASSERT( std::abs( bundleRef->GetArea( bundleRef.m_Ptr) - 78.5398f) < 0.01f, "BundleRef dispatches GetArea");

        // Zero-cost sub-trait upcasting to individual TRef<DrawableTrait>
        TRef< DrawableTrait> drawSub = bundleRef.AsSub< DrawableTrait>();
        TEST_ASSERT( drawSub.IsValid(), "Upcasted to TRef<DrawableTrait>");

        // Implicit conversion to supertrait TRef
        TRef< DrawableTrait> implicitDraw = bundleRef;
        TEST_ASSERT( implicitDraw.IsValid(), "Implicitly converted to TRef<DrawableTrait>");

        // Mutable bundle ref & scaling
        MutBundleRef< DrawableTrait, ScalableTrait> mutBundle = circleObj;
        mutBundle->Scale( mutBundle.m_Ptr, 2.0f);
        TEST_ASSERT( circleObj.m_radius == 10.0f, "MutBundleRef scaled circle radius to 10");

        // Owning polymorphic box with TraitBundle (BundlePtr)
        BundlePtr< DrawableTrait, ScalableTrait> owningBundle = TestCircle{.m_radius = 3.0f};
        TEST_ASSERT( owningBundle.IsValid(), "BundlePtr holding TestCircle is valid");
        TRef< DrawableTrait> owningSub = owningBundle.AsSubRef< DrawableTrait>();
        TEST_ASSERT( owningSub.IsValid(), "BundlePtr extracted TRef<DrawableTrait>");
    }
}


void test_silo_seg( void)
{
    using namespace xeom::silo;

    USeg seg = USeg::New( 2, 8);
    TEST_ASSERT( seg.Begin() == 2, "Seg Begin == 2");
    TEST_ASSERT( seg.Size() == 8, "Seg Size == 8");
    TEST_ASSERT( seg.End() == 10, "Seg End == 10");
    TEST_ASSERT( !seg.IsEmpty(), "Seg IsEmpty is false");

    USeg emptySeg = USeg::New( 0, 0);
    TEST_ASSERT( emptySeg.IsEmpty(), "Seg IsEmpty is true for size 0");

    USeg rsnip = seg.RSnip( 3);
    TEST_ASSERT( rsnip.Begin() == 2 && rsnip.Size() == 5, "Seg RSnip(3)");

    USeg lsnip = seg.LSnip( 3);
    TEST_ASSERT( lsnip.Begin() == 5 && lsnip.Size() == 5, "Seg LSnip(3)");

    uint32_t count = 0;
    seg.Traverse( [&]( uint32_t) {
        ++count;
    });
    TEST_ASSERT( count == 8, "Seg Traverse visited 8 elements");

    bool spanOk = seg.Span( []( uint32_t idx) {
        return idx >= 2 && idx < 10;
    });
    TEST_ASSERT( spanOk, "Seg Span verified interval [2, 10)");
}

void test_silo_access( void)
{
    using namespace xeom::silo;

    std::vector< int> vec = { 10, 20, 30, 40, 50 };
    IAccess< int> accessVec = vec;

    TEST_ASSERT( accessVec.IsValid(), "IAccess bound to std::vector");
    TEST_ASSERT( accessVec.Size() == 5, "IAccess Size == 5");
    TEST_ASSERT( !accessVec.IsEmpty(), "IAccess IsEmpty is false");
    TEST_ASSERT( accessVec.First() == 10, "IAccess First == 10");
    TEST_ASSERT( accessVec.Last() == 50, "IAccess Last == 50");
    TEST_ASSERT( accessVec[2] == 30, "IAccess operator[] == 30");
    TEST_ASSERT( accessVec.At( 3) == 40, "IAccess At(3) == 40");

    // USeg and functional methods
    auto seg = accessVec.USeg();
    TEST_ASSERT( seg.Begin() == 0 && seg.Size() == 5, "IAccess USeg matches [0, 5)");

    int sumTraverse = 0;
    accessVec.Traverse( [&]( int val) {
        sumTraverse += val;
    });
    TEST_ASSERT( sumTraverse == 150, "IAccess Traverse accumulated 150");

    bool allPositive = accessVec.Span( []( int val) {
        return val > 0;
    });
    TEST_ASSERT( allPositive, "IAccess Span verified all positive");

    bool sorted = accessVec.SortSanity( []( int a, int b) {
        return a < b;
    });
    TEST_ASSERT( sorted, "IAccess SortSanity verified ascending order");

    // Range-based for loop & Iterator
    int sumIter = 0;
    for ( int x : accessVec) {
        sumIter += x;
    }
    TEST_ASSERT( sumIter == 150, "IAccess range-based for loop accumulated 150");

    // Display format
    std::string formatted = accessVec.Format();
    TEST_ASSERT( formatted == "[10, 20, 30, 40, 50]", "IAccess Format matches [10, 20, 30, 40, 50]");

    // std::array binding
    std::array< float, 3> arr = { 1.5f, 2.5f, 3.5f };
    IAccess< float> accessArr = arr;
    TEST_ASSERT( accessArr.Size() == 3, "IAccess bound to std::array with Size 3");
    TEST_ASSERT( accessArr.First() == 1.5f && accessArr.Last() == 3.5f, "IAccess First/Last on std::array");

    // Arr binding
    int rawArr[3] = { 20, 30, 40 };
    Arr< int> xeomArr = rawArr;
    IAccess< int> accessArrDirect = xeomArr;
    TEST_ASSERT( accessArrDirect.Size() == 3, "IAccess bound to Arr with Size 3");
    TEST_ASSERT( accessArrDirect.First() == 20 && accessArrDirect.Last() == 40, "IAccess First/Last on Arr");
}

void test_silo_arr( void)
{
    using namespace xeom::silo;

    // 1. Concrete Arr<int> basic operations
    int buffer[5] = { 10, 20, 30, 40, 50 };
    Arr< int> arr( buffer, 5);

    TEST_ASSERT( arr.Size() == 5, "Arr Size == 5");
    TEST_ASSERT( !arr.IsEmpty(), "Arr IsEmpty is false");
    TEST_ASSERT( arr.First() == 10 && arr.Last() == 50, "Arr First/Last access");
    TEST_ASSERT( arr[2] == 30, "Arr operator[] == 30");

    // 2. Mutating operations
    arr.SetAt( 0, 99);
    TEST_ASSERT( arr.First() == 99, "Arr SetAt(0, 99)");
    arr.SetAt( 0, 10);

    int swapVal = 77;
    arr.SwapAt( 1, swapVal);
    TEST_ASSERT( arr[1] == 77 && swapVal == 20, "Arr SwapAt swapped values");
    arr.SetAt( 1, 20);

    arr.Swap( 0, 4);
    TEST_ASSERT( arr[0] == 50 && arr[4] == 10, "Arr Swap(0, 4)");
    arr.Swap( 0, 4); // restore

    // 3. Slicing: LSnip, RSnip, Subset
    auto lsnip = arr.LSnip( 2);
    TEST_ASSERT( lsnip.Size() == 3 && lsnip[0] == 30 && lsnip[2] == 50, "Arr LSnip(2) matches [30, 40, 50]");

    auto rsnip = arr.RSnip( 2);
    TEST_ASSERT( rsnip.Size() == 3 && rsnip[0] == 10 && rsnip[2] == 30, "Arr RSnip(2) matches [10, 20, 30]");

    auto subset = arr.Subset( 1, 3);
    TEST_ASSERT( subset.Size() == 3 && subset[0] == 20 && subset[2] == 40, "Arr Subset(1, 3) matches [20, 30, 40]");

    // 4. Block swap: SwapFrom
    int srcBuf[3] = { 100, 200, 300 };
    Arr< int> srcArr( srcBuf, 3);
    arr.SwapFrom( 1, srcArr, 0, 3);
    TEST_ASSERT( arr[1] == 100 && arr[2] == 200 && arr[3] == 300, "Arr SwapFrom transferred elements");
    TEST_ASSERT( srcArr[0] == 20 && srcArr[1] == 30 && srcArr[2] == 40, "Arr SwapFrom source received original elements");
    arr.SwapFrom( 1, srcArr, 0, 3); // restore

    // 5. DoIndexSetup & SortSanity
    int idxBuf[6] = { 0 };
    Arr< int> idxArr( idxBuf, 6);
    idxArr.DoIndexSetup();
    TEST_ASSERT( idxArr[0] == 0 && idxArr[5] == 5, "Arr DoIndexSetup initialized indices 0..5");
    TEST_ASSERT( idxArr.SortSanity( []( int a, int b) { return a < b; }), "Arr SortSanity on indexed array");

    // 6. Formatting
    TEST_ASSERT( arr.Format() == "[10, 20, 30, 40, 50]", "Arr Format matches [10, 20, 30, 40, 50]");

    // 7. String / Byte view integration
    std::string_view hello = "Xeom";
    Arr< char> charArr( hello);
    TEST_ASSERT( charArr.Size() == 4 && charArr.AsStringView() == "Xeom", "Arr<char> from string_view");

    // 8. IArr Trait Facade (fat pointer over mutable container)
    std::vector< int> mutVec = { 5, 4, 3, 2, 1 };
    IArr< int> iarr = mutVec;
    TEST_ASSERT( iarr.IsValid(), "IArr bound to std::vector");
    TEST_ASSERT( iarr.Size() == 5, "IArr Size == 5");
    TEST_ASSERT( iarr[0] == 5 && iarr.Last() == 1, "IArr indexing");

    iarr.Swap( 0, 4);
    TEST_ASSERT( mutVec[0] == 1 && mutVec[4] == 5, "IArr Swap mutated underlying std::vector");

    // Conversion to IAccess
    IAccess< int> accessFromArr = arr.AsAccess();
    TEST_ASSERT( accessFromArr.Size() == 5 && accessFromArr[2] == 30, "Arr converted to IAccess");
}

void test_silo_buff( void)
{
    using namespace xeom::silo;

    // 1. NewEmpty & default constructor
    Buff< int> emptyBuff = Buff< int>::NewEmpty();
    TEST_ASSERT( emptyBuff.IsEmpty(), "Buff NewEmpty is empty");
    TEST_ASSERT( emptyBuff.Size() == 0, "Buff NewEmpty Size == 0");

    // 2. Factory New and Create
    Buff< int> fillBuff = Buff< int>::New( 4, 99);
    TEST_ASSERT( fillBuff.Size() == 4, "Buff New size == 4");
    TEST_ASSERT( fillBuff.First() == 99 && fillBuff.Last() == 99, "Buff New values initialized to 99");

    Buff< int> genBuff = Buff< int>::Create( 5, []( uint32_t i) {
        return static_cast< int>( i) * 10;
    });
    TEST_ASSERT( genBuff.Size() == 5, "Buff Create size == 5");
    TEST_ASSERT( genBuff[0] == 0 && genBuff[4] == 40, "Buff Create values 0..40");

    // 3. Initializer list constructor
    Buff< int> initBuff = { 1, 2, 3, 4, 5 };
    TEST_ASSERT( initBuff.Size() == 5, "Buff initializer list size == 5");
    TEST_ASSERT( initBuff[2] == 3, "Buff initializer list element [2] == 3");

    // 4. Resize with dispenser
    initBuff.Resize( 8, []( uint32_t i) {
        return static_cast< int>( i) * 100;
    });
    TEST_ASSERT( initBuff.Size() == 8, "Buff Resize to 8");
    TEST_ASSERT( initBuff[5] == 500 && initBuff[7] == 700, "Buff Resize dispenser values");

    // 6. ExtendFromSlice & ExtendFromArr
    int extra[2] = { 800, 900 };
    initBuff.ExtendFromSlice( extra);
    TEST_ASSERT( initBuff.Size() == 10 && initBuff.Last() == 900, "Buff ExtendFromSlice");

    // 7. Concat
    int bufA[2] = { 10, 20 };
    int bufB[3] = { 30, 40, 50 };
    Buff< int> concatBuff = Buff< int>::Concat( Arr< int>( bufA, 2), Arr< int>( bufB, 3));
    TEST_ASSERT( concatBuff.Size() == 5, "Buff Concat size == 5");
    TEST_ASSERT( concatBuff[0] == 10 && concatBuff[4] == 50, "Buff Concat contents [10..50]");

    // 8. Conversion to Arr, IAccess, and IArr
    Arr< int> buffArr = concatBuff;
    TEST_ASSERT( buffArr.Size() == 5, "Buff implicitly converts to Arr with size == 5");

    IAccess< int> accessBuff = concatBuff.AsAccess();
    TEST_ASSERT( accessBuff.Size() == 5 && accessBuff[1] == 20, "Buff AsAccess");

    IArr< int> iarrBuff = concatBuff.AsIArr();
    iarrBuff.Swap( 0, 4);
    TEST_ASSERT( concatBuff[0] == 50 && concatBuff[4] == 10, "Buff mutated via IArr Swap");

    // 9. Slicing on Buff: LSnip, RSnip, Subset
    auto lsnip = concatBuff.LSnip( 2);
    TEST_ASSERT( lsnip.Size() == 3 && lsnip[0] == 30, "Buff LSnip(2)");

    // 10. Copy and Move semantics
    Buff< int> copyBuff = concatBuff;
    TEST_ASSERT( copyBuff.Size() == concatBuff.Size(), "Buff copy constructor size match");
    TEST_ASSERT( copyBuff[0] == concatBuff[0], "Buff copy constructor elements match");

    Buff< int> moveBuff = std::move( copyBuff);
    TEST_ASSERT( moveBuff.Size() == 5, "Buff move constructor target has size 5");
    TEST_ASSERT( copyBuff.IsEmpty(), "Buff moved-from is empty");

    // 11. Format
    TEST_ASSERT( concatBuff.Format() == "[50, 20, 30, 40, 10]", "Buff Format");
}

//-----------------------------------------------------------------------------------------------------------------

void test_stalks_primitives( void)
{
    using namespace xeom::stalks;

    // 1. Atm
    Atm< uint32_t> atm( 10);
    TEST_ASSERT( atm.Get() == 10, "Atm Get initial value");
    atm.Set( 20);
    TEST_ASSERT( atm.Get() == 20, "Atm Set value");
    uint32_t prev = atm.FetchAdd( 5);
    TEST_ASSERT( prev == 20 && atm.Get() == 25, "Atm FetchAdd");

    // 2. Spinlock
    Spinlock spinlock;
    {
        auto guard = spinlock.Lock();
    }
    TEST_ASSERT( true, "Spinlock RAII acquire & release");

    // 3. WorkPtr & Worker
    int executed = 0;
    Worker worker = Worker::New();
    worker.Post( [&]( IWorker * ) {
        executed += 1;
    });
    TEST_ASSERT( executed == 1, "Worker executed posted lambda");

    // 4. BinNode composition
    BinNode< int, int> node( 10, 20, BinOp::Sum);
    TEST_ASSERT( node.m_Left == 10 && node.m_Right == 20 && node.m_Op == BinOp::Sum, "BinNode structure");
}

//-----------------------------------------------------------------------------------------------------------------

void test_silo_stk_stash( void)
{
    using namespace xeom::silo;

    // 1. Stash basic ops
    Stash< int> stash = Stash< int>::New( 16, 0, 0);
    TEST_ASSERT( stash.Size() == 0, "Stash initial size 0");
    stash.Push( 100);
    stash.Push( 200);
    TEST_ASSERT( stash.Size() == 2, "Stash size after 2 pushes");

    int val = 0;
    TEST_ASSERT( stash.Pop( val) && val == 200, "Stash Pop top element");
    TEST_ASSERT( stash.Pop( val) && val == 100, "Stash Pop second element");
    TEST_ASSERT( stash.Size() == 0, "Stash empty after popping all");
}

//-----------------------------------------------------------------------------------------------------------------

void test_heist_maestro( void)
{
    using namespace xeom::heist;
    using namespace xeom::silo;

    Atelier atelier = Atelier::New( 4);
    {
        auto maestros = atelier.Maestros();
        Maestro &m2 = maestros[2];
        m2.SetAtelier( &atelier);
        m2.SetCurSuccId( 42);
    }
    auto maestros = atelier.Maestros();
    TEST_ASSERT( maestros[2].MaestroIndex() == 2, "MaestroIndex == 2");
    TEST_ASSERT( maestros[2].CurSuccId() == 42, "CurSuccId == 42");
}

//-----------------------------------------------------------------------------------------------------------------

void test_heist_atelier_launch( void)
{
    using namespace xeom::heist;
    using namespace xeom::stalks;
    using namespace xeom::silo;

    static std::atomic< int> executedCount{0};
    executedCount = 0;

    Atelier atelier = Atelier::New( 4);
    Maestro *mainMaestro = atelier.MainMaestro();

    uint16_t jobId = mainMaestro->ConstructJob(
        0,
        WorkPtr::FromLambda( []( IWorker *w ) {
            Maestro *m = Maestro::FromWorker( w);
            executedCount += 1;
            uint16_t child1 = m->ConstructJob(
                m->CurSuccId(),
                WorkPtr::FromLambda( []( IWorker * ) {
                    executedCount += 10;
                }),
                "Child1"
            );
            m->EnqueueJob( child1);
        }),
        "TrialJob"
    );
    mainMaestro->EnqueueJob( jobId);
    atelier.DoLaunch();

    TEST_ASSERT( executedCount == 11, "Atelier DoLaunch executed master & child jobs");
}

//-----------------------------------------------------------------------------------------------------------------

void test_heist_choretree_dag( void)
{
    using namespace xeom::heist;
    using namespace xeom::stalks;
    using namespace xeom::silo;

    static std::atomic< int> traceIdx{0};
    traceIdx = 0;

    auto a = Chore::NewDoc( "A", []( IWorker * ) { traceIdx += 1; });
    auto b = Chore::NewDoc( "B", []( IWorker * ) { traceIdx += 2; });
    auto c = Chore::NewDoc( "C", []( IWorker * ) { traceIdx += 4; });
    auto d = Chore::NewDoc( "D", []( IWorker * ) { traceIdx += 8; });

    // ChoreTree DAG: (a < b) | (c < d)
    auto choreTree = ( a < b) | ( c < d);

    Atelier atelier = Atelier::New( 4);
    Maestro *mainMaestro = atelier.MainMaestro();
    mainMaestro->PostChoreTree( choreTree);
    atelier.DoLaunch();

    TEST_ASSERT( traceIdx == 15, "ChoreTree executed all 4 jobs in DAG");
}

//-----------------------------------------------------------------------------------------------------------------

int main( void) noexcept
{
    try {
        xeom::Logger::info( "========================================================");
        xeom::Logger::info( "  Xeom Test Suite (Clang 20+ / C++20 / clc++2021)       ");
        xeom::Logger::info( "========================================================");
        xeom::Logger::info( "--- CPU Tests ---");
        test_compiler_version();
        test_common_types();
        test_xeom_paradigms();
        test_silo_traits();
        test_silo_seg();
        test_silo_access();
        test_silo_arr();
        test_silo_buff();
        test_stalks_primitives();
        test_silo_stk_stash();
        test_heist_maestro();
        test_heist_atelier_launch();
        test_heist_choretree_dag();
        test_vector_add_cpu();
        test_vector_fma_cpu();

        xeom::Logger::info( "\n--- GPU Tests (clc++2021 on Intel Iris Xe) ---");
        test_gpu_subsystem();

        xeom::Logger::info( "");
        if ( g_failed == 0) {
            xeom::Logger::success( "All tests passed!");
            return 0;
        }
        xeom::Logger::error( "{} test(s) failed!", g_failed);
        return 1;
    } catch (...) {
        return 1;
    }
}
