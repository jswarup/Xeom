// traits_tests.cpp --------------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "jeeves/jeeves.h"

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

JEEVES_TEST( "silo::Traits: TRef/MTRef facades, TraitMeta and TraitBundle composition")
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
    JEEVES_CHECK_MSG( refC.IsValid(), "TraitRef Circle is valid");
    JEEVES_CHECK_MSG( refR.IsValid(), "TraitRef Rect is valid");

    // 2. Direct Ergonomic Facade Syntax (ref.Draw(), ref.GetArea())
    Drawable drawCircle = circle;
    Drawable drawRect   = rect;
    std::string facadeOut;
    drawCircle.Draw( facadeOut);
    JEEVES_CHECK_MSG( facadeOut.find( "Circle") != std::string::npos, "Facade Draw on Circle");
    drawRect.Draw( facadeOut);
    JEEVES_CHECK_MSG( facadeOut.find( "Rect") != std::string::npos, "Facade Draw on Rectangle");
    JEEVES_CHECK_MSG( std::abs( drawCircle.GetArea() - 12.56637f) < 1e-3f, "Facade GetArea on Circle");
    JEEVES_CHECK_MSG( std::abs( drawRect.GetArea() - 20.0f) < 1e-5f, "Facade GetArea on Rectangle");

    // 3. Generic Invoke<&VTable::Fn>(args...)
    std::string out;
    refC.Invoke< &DrawableTrait::VTable::Draw>( out);
    JEEVES_CHECK_MSG( out.find( "Circle") != std::string::npos, "Invoke Draw on Circle");

    refR.Invoke< &DrawableTrait::VTable::Draw>( out);
    JEEVES_CHECK_MSG( out.find( "Rect") != std::string::npos, "Invoke Draw on Rectangle");

    float areaC = refC.Invoke< &DrawableTrait::VTable::GetArea>();
    JEEVES_CHECK_MSG( std::abs( areaC - 12.56637f) < 1e-3f, "Invoke GetArea on Circle");

    float areaR = refR.Invoke< &DrawableTrait::VTable::GetArea>();
    JEEVES_CHECK_MSG( std::abs( areaR - 20.0f) < 1e-5f, "Invoke GetArea on Rectangle");

    // 4. Iterate heterogeneous array
    Drawable drawables[] = { drawCircle, drawRect };
    float total = 0.0f;
    for ( const auto &d : drawables) {
        total += d.GetArea();
    }
    JEEVES_CHECK_MSG( std::abs( total - 32.56637f) < 1e-3f, "Batch iteration total area with Facade");

    // 5. Safe downcast
    JEEVES_CHECK_MSG( refC.As< TestCircle>() != nullptr, "Downcast Circle succeeds");
    JEEVES_CHECK_MSG( refC.As< TestRectangle>() == nullptr, "Downcast mismatch returns nullptr");

    // 6. Mutable Facade / MTRef — mutation through fat pointer
    Scalable mutC( circle);
    mutC.Scale( 3.0f);
    JEEVES_CHECK_MSG( circle.m_radius == 6.0f, "Scalable facade mutated Circle radius");

    // 7. TraitMeta & TPtr — owning SBO
    {
        const TraitMeta< DrawableTrait> *metaCircle = TraitMeta< DrawableTrait>::For< TestCircle>();
        JEEVES_CHECK_MSG( metaCircle != nullptr, "TraitMeta singleton for Circle is non-null");
        JEEVES_CHECK_MSG( ( metaCircle == ObjMeta< DrawableTrait, TestCircle>::Get()), "TraitMeta matches ObjMeta singleton");
        JEEVES_CHECK_MSG( metaCircle->m_Size == sizeof( TestCircle), "TraitMeta size matches TestCircle");
        JEEVES_CHECK_MSG( metaCircle->IsInline( 48), "TestCircle fits in 48B SBO");

        TPtr< DrawableTrait> box( TestCircle{.m_radius = 5.0f});
        JEEVES_CHECK_MSG( box.IsValid(), "TPtr constructed with SBO");

        float boxArea = box.Invoke< &DrawableTrait::VTable::GetArea>();
        JEEVES_CHECK_MSG( std::abs( boxArea - 78.5398f) < 1e-2f, "TPtr Invoke GetArea");

        auto box2 = std::move( box);
        JEEVES_CHECK_MSG( !box.IsValid(), "Moved-from TPtr is empty");
        JEEVES_CHECK_MSG( box2.IsValid(), "Moved-to TPtr is valid");
    }

    // 8. TraitMeta — Trait-Scoped TypeId dispatch + Jump tables
    {
        uint32_t circleId = TraitMeta< DrawableTrait>::Id< TestCircle>();
        uint32_t rectId   = TraitMeta< DrawableTrait>::Id< TestRectangle>();
        JEEVES_CHECK_MSG( circleId == 0, "Circle TypeId under DrawableTrait is 0");
        JEEVES_CHECK_MSG( rectId == 1, "Rect TypeId under DrawableTrait is 1");
        JEEVES_CHECK_MSG( TraitMeta< DrawableTrait>::Count() == 2, "DrawableTrait has 2 registered types");

        alignas( std::max_align_t) std::byte store[64]{};
        uint32_t tid = TraitMeta< DrawableTrait>::Emplace< TestCircle>( store, 10.0f);
        JEEVES_CHECK_MSG( tid == 0, "Emplace returns correct TypeId");

        TRef< DrawableTrait> cref = TraitMeta< DrawableTrait>::Ref( store, tid);
        JEEVES_CHECK_MSG( cref.IsValid(), "TraitMeta::Ref produces valid TRef");

        Drawable drawCircle = cref;
        float circleArea = drawCircle.GetArea();
        JEEVES_CHECK_MSG( std::abs( circleArea - 314.159f) < 0.1f, "TraitMeta dispatched GetArea on Circle(r=10)");

        uint32_t scaleTid = TraitMeta< ScalableTrait>::Id< TestCircle>();
        MTRef< ScalableTrait> mutCircle = TraitMeta< ScalableTrait>::MutRef( store, scaleTid);
        mutCircle.Invoke< &ScalableTrait::VTable::Scale>( 2.0f);
        JEEVES_CHECK_MSG( std::abs( drawCircle.GetArea() - 1256.637f) < 0.5f, "Mutated Circle(r=20) via ScalableTrait");

        TraitMeta< DrawableTrait>::Destroy( store, tid);

        tid = TraitMeta< DrawableTrait>::Emplace< TestRectangle>( store, 7.0f, 3.0f);
        cref = TraitMeta< DrawableTrait>::Ref( store, tid);
        float rectArea = cref.Invoke< &DrawableTrait::VTable::GetArea>();
        JEEVES_CHECK_MSG( std::abs( rectArea - 21.0f) < 1e-5f, "TraitMeta dispatched GetArea on Rect(7x3)");
        TraitMeta< DrawableTrait>::Destroy( store, tid);

        JEEVES_CHECK_MSG( TraitMeta< DrawableTrait>::VTable( 99) == nullptr, "TraitMeta returns nullptr for invalid TypeId");
    }

    // 9. Trait Composition & TraitBundle (Rust-like trait composition)
    {
        TestCircle circleObj{.m_radius = 5.0f};

        BundleRef< DrawableTrait, ScalableTrait> bundleRef = circleObj;
        JEEVES_CHECK_MSG( bundleRef.IsValid(), "BundleRef is valid for composite traits");

        std::string desc;
        bundleRef->Draw( bundleRef.m_Ptr, desc);
        JEEVES_CHECK_MSG( desc.find( "Circle") != std::string::npos, "BundleRef dispatches Draw");
        JEEVES_CHECK_MSG( std::abs( bundleRef->GetArea( bundleRef.m_Ptr) - 78.5398f) < 0.01f, "BundleRef dispatches GetArea");

        TRef< DrawableTrait> drawSub = bundleRef.AsSub< DrawableTrait>();
        JEEVES_CHECK_MSG( drawSub.IsValid(), "Upcasted to TRef<DrawableTrait>");

        TRef< DrawableTrait> implicitDraw = bundleRef;
        JEEVES_CHECK_MSG( implicitDraw.IsValid(), "Implicitly converted to TRef<DrawableTrait>");

        MutBundleRef< DrawableTrait, ScalableTrait> mutBundle = circleObj;
        mutBundle->Scale( mutBundle.m_Ptr, 2.0f);
        JEEVES_CHECK_MSG( circleObj.m_radius == 10.0f, "MutBundleRef scaled circle radius to 10");

        BundlePtr< DrawableTrait, ScalableTrait> owningBundle = TestCircle{.m_radius = 3.0f};
        JEEVES_CHECK_MSG( owningBundle.IsValid(), "BundlePtr holding TestCircle is valid");
        TRef< DrawableTrait> owningSub = owningBundle.AsSubRef< DrawableTrait>();
        JEEVES_CHECK_MSG( owningSub.IsValid(), "BundlePtr extracted TRef<DrawableTrait>");
    }
}
