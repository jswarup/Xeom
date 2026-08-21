// traits.h -------------------------------------------------------------------------------------------------------
#pragma once

#include "cove/typeincl.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::silo {

//-----------------------------------------------------------------------------------------------------------------
// VTable singleton — one static VTable instance per (Trait, Object) pair.

template < typename TTrait, typename TObj>
struct ObjVTable
{
    static inline const typename TTrait::VTable s_VTable = TTrait::template Bind< TObj>();

    static constexpr const typename TTrait::VTable *Get( void) noexcept
    {
        return &s_VTable;
    }
};

//-----------------------------------------------------------------------------------------------------------------
// Forward Declarations & Type Traits

template < typename TTrait>
struct TRef;

template < typename TTrait>
struct MTRef;

namespace detail {

template < typename T>
struct is_tref : std::false_type {};

template < typename TTrait>
struct is_tref< TRef< TTrait>> : std::true_type {};

template < typename T>
struct is_mtref : std::false_type {};

template < typename TTrait>
struct is_mtref< MTRef< TTrait>> : std::true_type {};

} // namespace detail

template < typename T>
inline constexpr bool is_tref_v = detail::is_tref< std::decay_t< T>>::value;

template < typename T>
inline constexpr bool is_mtref_v = detail::is_mtref< std::decay_t< T>>::value;

//-----------------------------------------------------------------------------------------------------------------
// TRef — non-owning immutable fat pointer. 16 bytes, trivially copyable.

template < typename TTrait>
struct TRef
{
    using VT = typename TTrait::VTable;

    const void *m_Ptr{nullptr};
    const VT   *m_Ops{nullptr};

    constexpr TRef( void) noexcept = default;

template < typename T>
        requires ( !is_tref_v< T> && !is_mtref_v< T>)
    constexpr TRef( const T &obj) noexcept
        : m_Ptr( &obj),
          m_Ops( ObjVTable< TTrait, T>::Get())
    {
    }

template < typename TSuperTrait>
        requires ( !std::same_as< TSuperTrait, TTrait> && std::is_base_of_v< VT, typename TSuperTrait::VTable>)
    constexpr TRef( const TRef< TSuperTrait> &other) noexcept
        : m_Ptr( other.m_Ptr),
          m_Ops( static_cast< const VT *>( other.m_Ops))
    {
    }

template < typename TSuperTrait>
        requires std::is_base_of_v< VT, typename TSuperTrait::VTable>
    constexpr TRef( const MTRef< TSuperTrait> &other) noexcept
        : m_Ptr( other.m_Ptr),
          m_Ops( static_cast< const VT *>( other.m_Ops))
    {
    }

    constexpr TRef( const void *ptr, const VT *ops) noexcept
        : m_Ptr( ptr),
          m_Ops( ops)
    {
    }

    constexpr bool IsValid( void) const noexcept
    {
        return m_Ptr && m_Ops;
    }

    constexpr explicit operator bool( void) const noexcept
    {
        return IsValid();
    }

    constexpr const VT *operator->( void) const noexcept
    {
        return m_Ops;
    }

template < auto Fn, typename... TArgs>
    constexpr decltype( auto) Invoke( TArgs &&... args) const
    {
        return ( m_Ops->*Fn)( m_Ptr, std::forward< TArgs>( args)...);
    }

template < typename T>
    const T *As( void) const noexcept
    {
        return ( m_Ops == ObjVTable< TTrait, T>::Get()) ? static_cast< const T *>( m_Ptr) : nullptr;
    }

template < typename TSubTrait>
        requires ( !std::same_as< TSubTrait, TTrait> && std::is_base_of_v< typename TSubTrait::VTable, VT>)
    constexpr TRef< TSubTrait> AsSub( void) const noexcept
    {
        return { m_Ptr, static_cast< const typename TSubTrait::VTable *>( m_Ops) };
    }
};

//-----------------------------------------------------------------------------------------------------------------
// MTRef — non-owning mutable fat pointer.

template < typename TTrait>
struct MTRef
{
    using VT = typename TTrait::VTable;

    void     *m_Ptr{nullptr};
    const VT *m_Ops{nullptr};

    constexpr MTRef( void) noexcept = default;

template < typename T>
        requires ( !is_mtref_v< T> && !is_tref_v< T>)
    constexpr MTRef( T &obj) noexcept
        : m_Ptr( &obj),
          m_Ops( ObjVTable< TTrait, T>::Get())
    {
    }

template < typename TSuperTrait>
        requires ( !std::same_as< TSuperTrait, TTrait> && std::is_base_of_v< VT, typename TSuperTrait::VTable>)
    constexpr MTRef( const MTRef< TSuperTrait> &other) noexcept
        : m_Ptr( other.m_Ptr),
          m_Ops( static_cast< const VT *>( other.m_Ops))
    {
    }

    constexpr MTRef( void *ptr, const VT *ops) noexcept
        : m_Ptr( ptr),
          m_Ops( ops)
    {
    }

    constexpr bool IsValid( void) const noexcept
    {
        return m_Ptr && m_Ops;
    }

    constexpr explicit operator bool( void) const noexcept
    {
        return IsValid();
    }

    constexpr const VT *operator->( void) const noexcept
    {
        return m_Ops;
    }

    constexpr operator TRef< TTrait>( void) const noexcept
    {
        return { m_Ptr, m_Ops };
    }

template < auto Fn, typename... TArgs>
    constexpr decltype( auto) Invoke( TArgs &&... args) const
    {
        return ( m_Ops->*Fn)( m_Ptr, std::forward< TArgs>( args)...);
    }

template < typename T>
    T *As( void) const noexcept
    {
        return ( m_Ops == ObjVTable< TTrait, T>::Get()) ? static_cast< T *>( m_Ptr) : nullptr;
    }

template < typename TSubTrait>
        requires ( !std::same_as< TSubTrait, TTrait> && std::is_base_of_v< typename TSubTrait::VTable, VT>)
    constexpr MTRef< TSubTrait> AsSubMut( void) const noexcept
    {
        return { m_Ptr, static_cast< const typename TSubTrait::VTable *>( m_Ops) };
    }
};



//-----------------------------------------------------------------------------------------------------------------
// TraitMeta — consolidated metadata (VTable, Destructor, Move, Size, Alignment) and Trait-scoped TypeId dispatch.

template < typename TTrait>
struct TraitMeta
{
    using VT = typename TTrait::VTable;

    uint32_t   m_TypeId{0};
    const VT   *m_Ops{nullptr};
    void      ( *m_Destruct)( void *) noexcept{nullptr};
    void      ( *m_Move)( void *src, void *dst) noexcept{nullptr};
    size_t     m_Size{0};
    size_t     m_Align{0};

    static inline std::vector< const TraitMeta *> s_Table{};

    constexpr bool IsInline( size_t cap) const noexcept
    {
        return m_Size <= cap && m_Align <= alignof( std::max_align_t);
    }

template < typename TObj>
    static const TraitMeta *For( void) noexcept;

template < typename TObj>
    static uint32_t Id( void) noexcept;

    static size_t Count( void) noexcept
    {
        return s_Table.size();
    }

    static const TraitMeta *Get( uint32_t typeId) noexcept
    {
        return ( typeId < s_Table.size()) ? s_Table[typeId] : nullptr;
    }

    static const VT *VTable( uint32_t typeId) noexcept
    {
        const TraitMeta *m = Get( typeId);
        return m ? m->m_Ops : nullptr;
    }

    static TRef< TTrait> Ref( const void *ptr, uint32_t typeId) noexcept
    {
        return { ptr, VTable( typeId) };
    }

    static MTRef< TTrait> MutRef( void *ptr, uint32_t typeId) noexcept
    {
        return { ptr, VTable( typeId) };
    }

template < typename TObj, typename... TArgs>
    static uint32_t Emplace( void *dest, TArgs &&... args)
    {
        ::new ( dest) TObj( std::forward< TArgs>( args)...);
        return Id< TObj>();
    }

    static void Destroy( void *ptr, uint32_t typeId) noexcept
    {
        const TraitMeta *m = Get( typeId);
        if ( m && m->m_Destruct) {
            m->m_Destruct( ptr);
        }
    }

    static void Move( void *src, void *dst, uint32_t typeId) noexcept
    {
        const TraitMeta *m = Get( typeId);
        if ( m && m->m_Move) {
            m->m_Move( src, dst);
        }
    }
};

//-----------------------------------------------------------------------------------------------------------------

template < typename TTrait, typename TObj>
struct ObjMeta
{
    static inline const TraitMeta< TTrait> s_Meta = []() noexcept {
        uint32_t tid = static_cast< uint32_t>( TraitMeta< TTrait>::s_Table.size());
        TraitMeta< TTrait> meta {
            .m_TypeId   = tid,
            .m_Ops      = ObjVTable< TTrait, TObj>::Get(),
            .m_Destruct = []( void *p) noexcept {
                static_cast< TObj *>( p)->~TObj();
            },
            .m_Move     = []( void *s, void *d) noexcept {
                ::new ( d) TObj( std::move( *static_cast< TObj *>( s)));
                static_cast< TObj *>( s)->~TObj();
            },
            .m_Size     = sizeof( TObj),
            .m_Align    = alignof( TObj)
        };
        TraitMeta< TTrait>::s_Table.push_back( &s_Meta);
        return meta;
    }();

    static const TraitMeta< TTrait> *Get( void) noexcept
    {
        return &s_Meta;
    }

    static uint32_t Id( void) noexcept
    {
        return s_Meta.m_TypeId;
    }
};

//-----------------------------------------------------------------------------------------------------------------

template < typename TTrait>
template < typename TObj>
const TraitMeta< TTrait> *TraitMeta< TTrait>::For( void) noexcept
{
    return ObjMeta< TTrait, TObj>::Get();
}

//-----------------------------------------------------------------------------------------------------------------

template < typename TTrait>
template < typename TObj>
uint32_t TraitMeta< TTrait>::Id( void) noexcept
{
    return ObjMeta< TTrait, TObj>::Id();
}

//-----------------------------------------------------------------------------------------------------------------
// TPtr — owning fat pointer with inline SBO (Small Buffer Optimization).

template < typename TTrait, size_t TInlineCap = 48>
class TPtr
{
    using VT = typename TTrait::VTable;

    alignas( std::max_align_t) std::byte m_Buf[TInlineCap]{};
    void                     *m_Ptr{nullptr};
    const TraitMeta< TTrait> *m_Meta{nullptr};

    bool IsInline( void) const noexcept
    {
        return m_Meta && m_Meta->IsInline( TInlineCap);
    }

    void Destroy( void) noexcept
    {
        if ( !m_Ptr) {
            return;
        }
        if ( m_Meta && m_Meta->m_Destruct) {
            m_Meta->m_Destruct( m_Ptr);
        }
        if ( m_Meta && !IsInline()) {
            ::operator delete( m_Ptr);
        }
        m_Ptr  = nullptr;
        m_Meta = nullptr;
    }

public:
    TPtr( void) noexcept = default;

template < typename T>
        requires ( !std::same_as< std::decay_t< T>, TPtr>)
    TPtr( T &&val)
    {
        using D = std::decay_t< T>;
        m_Meta  = TraitMeta< TTrait>::template For< D>();
        if ( IsInline()) {
            m_Ptr = m_Buf;
        } else {
            m_Ptr = ::operator new( sizeof( D), std::align_val_t{ alignof( D) });
        }
        ::new ( m_Ptr) D( std::forward< T>( val));
    }

    ~TPtr( void) noexcept
    {
        Destroy();
    }

    TPtr( const TPtr &) = delete;
    TPtr &operator=( const TPtr &) = delete;

    TPtr( TPtr &&o) noexcept
        : m_Meta( o.m_Meta)
    {
        if ( IsInline()) {
            m_Ptr = m_Buf;
            m_Meta->m_Move( o.m_Ptr, m_Ptr);
        } else {
            m_Ptr = o.m_Ptr;
        }
        o.m_Ptr  = nullptr;
        o.m_Meta = nullptr;
    }

    TPtr &operator=( TPtr &&o) noexcept
    {
        if ( this != &o) {
            Destroy();
            m_Meta = o.m_Meta;
            if ( IsInline()) {
                m_Ptr = m_Buf;
                m_Meta->m_Move( o.m_Ptr, m_Ptr);
            } else {
                m_Ptr = o.m_Ptr;
            }
            o.m_Ptr  = nullptr;
            o.m_Meta = nullptr;
        }
        return SELF;
    }

    bool IsValid( void) const noexcept
    {
        return m_Ptr && m_Meta && m_Meta->m_Ops;
    }

    explicit operator bool( void) const noexcept
    {
        return IsValid();
    }

    const VT *operator->( void) const noexcept
    {
        return m_Meta ? m_Meta->m_Ops : nullptr;
    }

template < auto Fn, typename... TArgs>
    decltype( auto) Invoke( TArgs &&... args) const
    {
        return ( m_Meta->m_Ops->*Fn)( m_Ptr, std::forward< TArgs>( args)...);
    }

    TRef< TTrait> AsRef( void) const noexcept
    {
        return { m_Ptr, m_Meta ? m_Meta->m_Ops : nullptr };
    }

    MTRef< TTrait> AsMutRef( void) noexcept
    {
        return { m_Ptr, m_Meta ? m_Meta->m_Ops : nullptr };
    }

template < typename T>
    const T *As( void) const noexcept
    {
        return ( m_Meta && m_Meta->m_Ops == ObjVTable< TTrait, T>::Get()) ? static_cast< const T *>( m_Ptr) : nullptr;
    }

template < typename TSubTrait>
        requires ( !std::same_as< TSubTrait, TTrait> && std::is_base_of_v< typename TSubTrait::VTable, typename TTrait::VTable>)
    TRef< TSubTrait> AsSubRef( void) const noexcept
    {
        return { m_Ptr, m_Meta ? static_cast< const typename TSubTrait::VTable *>( m_Meta->m_Ops) : nullptr };
    }

template < typename TSubTrait>
        requires ( !std::same_as< TSubTrait, TTrait> && std::is_base_of_v< typename TSubTrait::VTable, typename TTrait::VTable>)
    MTRef< TSubTrait> AsSubMutRef( void) noexcept
    {
        return { m_Ptr, m_Meta ? static_cast< const typename TSubTrait::VTable *>( m_Meta->m_Ops) : nullptr };
    }
};

//-----------------------------------------------------------------------------------------------------------------
// TraitBundle — variadic zero-overhead composition of multiple independent traits.

template < typename... TTraits>
struct TraitBundle
{
    struct VTable : TTraits::VTable... {};

template < typename T>
        requires ( ( requires { TTraits::template Bind< T>(); } ) && ... )
    static constexpr VTable Bind( void) noexcept
    {
        return VTable{ { TTraits::template Bind< T>() }... };
    }
};

//-----------------------------------------------------------------------------------------------------------------
// Trait Convenience Type Aliases

template < typename... TTraits>
using BundleRef = TRef< TraitBundle< TTraits...>>;

template < typename... TTraits>
using MutBundleRef = MTRef< TraitBundle< TTraits...>>;

template < typename... TTraits>
using BundlePtr = TPtr< TraitBundle< TTraits...>>;

} // namespace xeom::silo

