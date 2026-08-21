// stk.h -----------------------------------------------------------------------------------------------------------
#pragma once

#include "cove/typeincl.h"
#include "silo/arr.h"
#include "silo/seg.h"
#include "stalks/atm.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::silo {

//-----------------------------------------------------------------------------------------------------------------
// Stk — atomic stack view over a pre-allocated Arr buffer and shared atomic size counter.

template < typename T>
class Stk
{
private:
    stalks::Atm< uint32_t> *m_Size{nullptr};
    Arr< T>                 m_Arr{};

public:
    constexpr Stk( void) noexcept = default;

    constexpr Stk( stalks::Atm< uint32_t> *sizePtr, Arr< T> arr) noexcept
        : m_Size( sizePtr),
          m_Arr( arr)
    {
    }

    static constexpr Stk Create( stalks::Atm< uint32_t> *sizePtr, Arr< T> arr) noexcept
    {
        return { sizePtr, arr };
    }

    uint32_t Size( void) const noexcept
    {
        return m_Size ? m_Size->Load( std::memory_order_acquire) : 0;
    }

    void SetSize( uint32_t size) noexcept
    {
        if ( m_Size ) {
            m_Size->Store( size, std::memory_order_release);
        }
    }

    uint32_t SzVoid( void) const noexcept
    {
        return m_Arr.Size() - Size();
    }

    USeg USeg( void) const noexcept
    {
        return silo::USeg::New( 0, Size());
    }

    Arr< T> ArrView( void) const noexcept
    {
        return m_Arr.RSnip( m_Arr.Size() - Size());
    }

    bool Pop( T &val) const
    {
        if ( !m_Size ) {
            return false;
        }
        uint32_t sz = Size();
        if ( sz == 0 || !m_Size->CompareExchange( sz, sz - 1, std::memory_order_acquire, std::memory_order_relaxed) ) {
            return false;
        }
        m_Arr.SwapAt( sz - 1, val);
        return true;
    }

    bool PushX( T &val) const
    {
        if ( !m_Size ) {
            return false;
        }
        uint32_t sz = Size();
        if ( sz >= m_Arr.Size() ) {
            return false;
        }
        m_Arr.SwapAt( sz, val);
        if ( !m_Size->CompareExchange( sz, sz + 1, std::memory_order_release, std::memory_order_relaxed) ) {
            m_Arr.SwapAt( sz, val);
            return false;
        }
        return true;
    }

    bool Push( T val) const
    {
        T tmp = std::move( val);
        return PushX( tmp);
    }

    uint32_t Import( const Stk< T> &other, uint32_t maxMov = UINT32_MAX) const
        requires std::is_trivially_copyable_v< T>
    {
        if ( !m_Size || !other.m_Size ) {
            return 0;
        }
        uint32_t szAlloc = 0;
        uint32_t oldSz = 0;
        while ( true ) {
            uint32_t sz = Size();
            uint32_t szCacheVoid = m_Arr.Size() - sz;
            uint32_t otherSz = other.Size();
            szAlloc = ( szCacheVoid < otherSz) ? szCacheVoid : otherSz;
            if ( szAlloc > maxMov ) {
                szAlloc = maxMov;
            }
            if ( szAlloc == 0 ) {
                return 0;
            }
            if ( m_Size->CompareExchange( sz, sz + szAlloc, std::memory_order_acq_rel, std::memory_order_acquire) ) {
                oldSz = sz;
                break;
            }
        }
        uint32_t otherOldSz = other.m_Size->FetchAdd( static_cast< uint32_t>( -static_cast< int32_t>( szAlloc)), std::memory_order_acq_rel);
        uint32_t stkSz = otherOldSz - szAlloc;
        m_Arr.SwapFrom( oldSz, other.m_Arr, stkSz, szAlloc);
        return szAlloc;
    }

    uint32_t Export( const Stk< T> &other, uint32_t maxMov = UINT32_MAX) const
        requires std::is_trivially_copyable_v< T>
    {
        if ( !m_Size || !other.m_Size ) {
            return 0;
        }
        uint32_t szAlloc = 0;
        uint32_t oldSz = 0;
        while ( true ) {
            uint32_t szStk = other.Size();
            uint32_t szStkVoid = other.m_Arr.Size() - szStk;
            uint32_t sz = Size();
            szAlloc = ( szStkVoid < sz) ? szStkVoid : sz;
            if ( szAlloc > maxMov ) {
                szAlloc = maxMov;
            }
            if ( szAlloc == 0 ) {
                return 0;
            }
            if ( m_Size->CompareExchange( sz, sz - szAlloc, std::memory_order_acq_rel, std::memory_order_acquire) ) {
                oldSz = sz;
                break;
            }
        }
        uint32_t szStk = other.m_Size->FetchAdd( szAlloc, std::memory_order_acq_rel);
        other.m_Arr.SwapFrom( szStk, m_Arr, oldSz - szAlloc, szAlloc);
        return szAlloc;
    }
};

} // namespace xeom::silo
