// buff.h ----------------------------------------------------------------------------------------------------------
#pragma once

#include "cove/typeincl.h"
#include "silo/seg.h"
#include "silo/access.h"
#include "silo/arr.h"
#include "silo/traits.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::silo {

template < typename TElem>
class Buff : public Arr< TElem>
{
public:
    using Base = Arr< TElem>;
    using typename Base::value_type;
    using typename Base::size_type;
    using typename Base::difference_type;
    using typename Base::reference;
    using typename Base::const_reference;
    using typename Base::pointer;
    using typename Base::const_pointer;

    // Constructors
    constexpr Buff( void) noexcept = default;

    Buff( uint32_t size, const TElem &initialValue)
        requires std::is_copy_constructible_v< TElem>
    {
        InitWith( size, [&]( TElem *p) {
            for ( uint32_t i = 0; i < size; ++i) {
                ::new ( static_cast< void *>( p + i)) TElem( initialValue);
            }
        });
    }

template < typename Dispenser>
        requires std::is_invocable_r_v< TElem, Dispenser, uint32_t>
    Buff( uint32_t size, Dispenser &&dispenser)
    {
        InitWith( size, [&]( TElem *p) {
            for ( uint32_t i = 0; i < size; ++i) {
                ::new ( static_cast< void *>( p + i)) TElem( dispenser( i));
            }
        });
    }

    Buff( std::initializer_list< TElem> init)
        requires std::is_copy_constructible_v< TElem>
        : Buff( Arr< const TElem>( init.begin(), static_cast< uint32_t>( init.size())))
    {
    }

    Buff( Arr< const TElem> arr)
        requires std::is_copy_constructible_v< TElem>
    {
        InitWith( arr.Size(), [&]( TElem *p) {
            for ( uint32_t i = 0; i < arr.Size(); ++i) {
                ::new ( static_cast< void *>( p + i)) TElem( arr[i]);
            }
        });
    }

    ~Buff( void) noexcept
    {
        Destroy();
    }

    Buff( const Buff &o)
        requires std::is_copy_constructible_v< TElem>
        : Buff( Arr< const TElem>( o.m_Ptr, o.m_Size))
    {
    }

    Buff &operator=( const Buff &o)
        requires std::is_copy_constructible_v< TElem>
    {
        if ( this != &o) {
            Buff tmp( o);
            SwapBuff( tmp);
        }
        return SELF;
    }

    Buff( Buff &&o) noexcept
        : Base( std::exchange( o.m_Ptr, nullptr), std::exchange( o.m_Size, 0))
    {
    }

    Buff &operator=( Buff &&o) noexcept
    {
        if ( this != &o) {
            Destroy();
            this->m_Ptr  = std::exchange( o.m_Ptr, nullptr);
            this->m_Size = std::exchange( o.m_Size, 0);
        }
        return SELF;
    }

    // Factory methods
    static constexpr Buff NewEmpty( void) noexcept
    {
        return {};
    }

    static Buff New( uint32_t size, const TElem &initialValue)
        requires std::is_copy_constructible_v< TElem>
    {
        return Buff( size, initialValue);
    }

template < typename Dispenser>
        requires std::is_invocable_r_v< TElem, Dispenser, uint32_t>
    static Buff Create( uint32_t size, Dispenser &&dispenser)
    {
        return Buff( size, std::forward< Dispenser>( dispenser));
    }

    static Buff Concat( Arr< const TElem> a, Arr< const TElem> b)
        requires std::is_copy_constructible_v< TElem>
    {
        Buff result;
        const uint32_t aSz = a.Size();
        const uint32_t bSz = b.Size();
        result.InitWith( aSz + bSz, [&]( TElem *p) {
            for ( uint32_t i = 0; i < aSz; ++i) {
                ::new ( static_cast< void *>( p + i)) TElem( a[i]);
            }
            for ( uint32_t i = 0; i < bSz; ++i) {
                ::new ( static_cast< void *>( p + aSz + i)) TElem( b[i]);
            }
        });
        return result;
    }

    // Dynamic operations
template < typename Dispenser>
        requires std::is_invocable_r_v< TElem, Dispenser, uint32_t>
    void Resize( uint32_t newSize, Dispenser &&dispenser)
    {
        if ( newSize <= this->m_Size) {
            return;
        }
        const uint32_t oldSize = this->m_Size;
        GrowAndInit( newSize - oldSize, [&]( TElem *dst) {
            for ( uint32_t i = oldSize; i < newSize; ++i) {
                ::new ( static_cast< void *>( dst + ( i - oldSize))) TElem( dispenser( i));
            }
        });
    }

    void ExtendFromSlice( Arr< const TElem> slice)
        requires std::is_copy_constructible_v< TElem>
    {
        const uint32_t addSize = slice.Size();
        if ( addSize == 0) {
            return;
        }
        GrowAndInit( addSize, [&]( TElem *dst) {
            for ( uint32_t i = 0; i < addSize; ++i) {
                ::new ( static_cast< void *>( dst + i)) TElem( slice[i]);
            }
        });
    }

    void SwapBuff( Buff &other) noexcept
    {
        std::swap( this->m_Ptr, other.m_Ptr);
        std::swap( this->m_Size, other.m_Size);
    }

    IArr< TElem> AsIArr( void) noexcept
    {
        return IArr< TElem>( *this);
    }

private:
    static TElem *Allocate( uint32_t count)
    {
        return static_cast< TElem *>( ::operator new( count * sizeof( TElem), std::align_val_t{ alignof( TElem) }));
    }

    static void Deallocate( TElem *ptr) noexcept
    {
        ::operator delete( static_cast< void *>( ptr), std::align_val_t{ alignof( TElem) });
    }

template < typename FInit>
    void InitWith( uint32_t sz, FInit &&finit)
    {
        if ( sz > 0) {
            this->m_Ptr  = Allocate( sz);
            this->m_Size = sz;
            finit( this->m_Ptr);
        }
    }

template < typename FInit>
    void GrowAndInit( uint32_t addCount, FInit &&finit)
    {
        const uint32_t oldSize = this->m_Size;
        const uint32_t newSize = oldSize + addCount;
        TElem *newPtr = Allocate( newSize);

        for ( uint32_t i = 0; i < oldSize; ++i) {
            ::new ( static_cast< void *>( newPtr + i)) TElem( std::move( this->m_Ptr[i]));
            this->m_Ptr[i].~TElem();
        }
        finit( newPtr + oldSize);

        if ( this->m_Ptr) {
            Deallocate( this->m_Ptr);
        }
        this->m_Ptr  = newPtr;
        this->m_Size = newSize;
    }

    void Destroy( void) noexcept
    {
        if ( this->m_Ptr) {
            for ( uint32_t i = 0; i < this->m_Size; ++i) {
                this->m_Ptr[i].~TElem();
            }
            Deallocate( this->m_Ptr);
            this->m_Ptr  = nullptr;
            this->m_Size = 0;
        }
    }
};

} // namespace xeom::silo
