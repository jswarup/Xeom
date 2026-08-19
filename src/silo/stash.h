// stash.h ---------------------------------------------------------------------------------------------------------
#pragma once

#include "common_types.h"
#include "buff.h"
#include "stk.h"
#include "../stalks/atm.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::silo {

//-----------------------------------------------------------------------------------------------------------------
// Stash — owning container providing an atomic stack (Stk) backed by a Buff.

template < typename T>
class Stash
{
private:
    Buff< T>                      m_Buff{};
    mutable stalks::Atm< uint32_t> m_Sz{0};

public:
    constexpr Stash( void) noexcept = default;

    Stash( uint32_t capacity, uint32_t initialSize, const T &def)
        requires std::is_copy_constructible_v< T>
        : m_Buff( capacity, def),
          m_Sz( initialSize)
    {
    }

template < typename Dispenser>
        requires std::is_invocable_r_v< T, Dispenser, uint32_t>
    Stash( uint32_t capacity, uint32_t initialSize, Dispenser &&dispenser)
        : m_Buff( capacity, std::forward< Dispenser>( dispenser)),
          m_Sz( initialSize)
    {
    }

    static Stash New( uint32_t capacity, uint32_t initialSize, const T &def)
        requires std::is_copy_constructible_v< T>
    {
        return Stash( capacity, initialSize, def);
    }

    static Stash NewEmpty( void) noexcept
    {
        return {};
    }

template < typename Dispenser>
        requires std::is_invocable_r_v< T, Dispenser, uint32_t>
    static Stash Create( uint32_t capacity, uint32_t initialSize, Dispenser &&dispenser)
    {
        return Stash( capacity, initialSize, std::forward< Dispenser>( dispenser));
    }

    uint32_t Size( void) const noexcept
    {
        return m_Sz.Load( std::memory_order_acquire);
    }

    void Clear( void) const noexcept
    {
        m_Sz.Store( 0, std::memory_order_release);
    }

    Stk< T> StkView( void) const noexcept
    {
        return Stk< T>( &m_Sz, Arr< T>( const_cast< T *>( m_Buff.Data()), m_Buff.Size()));
    }

    bool Pop( T &val) const
    {
        return StkView().Pop( val);
    }

    void Push( T val)
    {
        while ( !StkView().Push( val) ) {
            if ( Size() == m_Buff.Size() ) {
                uint32_t newCap = ( m_Buff.Size() == 0) ? 1 : ( m_Buff.Size() * 2);
                m_Buff.Resize( newCap, [&]( uint32_t ) { return val; });
            }
        }
    }

    void DoIndexSetup( void) const
        requires std::constructible_from< T, size_t>
    {
        m_Buff.DoIndexSetup();
        m_Sz.Store( m_Buff.Size(), std::memory_order_release);
    }
};

} // namespace xeom::silo
