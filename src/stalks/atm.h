// atm.h -----------------------------------------------------------------------------------------------------------
#pragma once

#include "cove/typeincl.h"
#include <atomic>
#include <thread>

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::stalks {

//-----------------------------------------------------------------------------------------------------------------
// Atm — lightweight wrapper encapsulating standard atomic operations.

template < typename T>
class Atm
{
private:
    std::atomic< T> m_Val{};

public:
    constexpr Atm( void) noexcept = default;

    constexpr explicit Atm( T val) noexcept
        : m_Val( val)
    {
    }

    T Load( std::memory_order order = std::memory_order_seq_cst) const noexcept
    {
        return m_Val.load( order);
    }

    void Store( T val, std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        m_Val.store( val, order);
    }

    T Get( void) const noexcept
    {
        return Load( std::memory_order_seq_cst);
    }

    void Set( T val) noexcept
    {
        Store( val, std::memory_order_seq_cst);
    }

    T FetchAdd( T val, std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        return m_Val.fetch_add( val, order);
    }

    T Add( T val) noexcept
    {
        return FetchAdd( val, std::memory_order_seq_cst);
    }

    bool CompareExchange( T &expected, T desired, std::memory_order success = std::memory_order_seq_cst, std::memory_order failure = std::memory_order_seq_cst) noexcept
    {
        return m_Val.compare_exchange_strong( expected, desired, success, failure);
    }

    bool CompareExchangeWeak( T &expected, T desired, std::memory_order success = std::memory_order_seq_cst, std::memory_order failure = std::memory_order_seq_cst) noexcept
    {
        return m_Val.compare_exchange_weak( expected, desired, success, failure);
    }
};

//-----------------------------------------------------------------------------------------------------------------
// SpinLockGuard — RAII scoped guard for Spinlock.

class Spinlock;

class SpinLockGuard
{
private:
    const Spinlock *m_Lock{nullptr};

public:
    explicit SpinLockGuard( const Spinlock *lock) noexcept;
    ~SpinLockGuard( void) noexcept;

    SpinLockGuard( const SpinLockGuard &) = delete;
    SpinLockGuard &operator=( const SpinLockGuard &) = delete;

    SpinLockGuard( SpinLockGuard &&o) noexcept
        : m_Lock( std::exchange( o.m_Lock, nullptr))
    {
    }
};

//-----------------------------------------------------------------------------------------------------------------
// Spinlock — low-latency atomic spinlock.

class Spinlock
{
private:
    mutable std::atomic< bool> m_Locked{false};

public:
    constexpr Spinlock( void) noexcept = default;

    void Acquire( void) const noexcept
    {
        while ( true) {
            if ( !m_Locked.exchange( true, std::memory_order_acquire)) {
                return;
            }
            while ( m_Locked.load( std::memory_order_relaxed)) {
#if defined(__x86_64__) || defined(_M_X64)
                _mm_pause();
#else
                std::this_thread::yield();
#endif
            }
        }
    }

    void Release( void) const noexcept
    {
        m_Locked.store( false, std::memory_order_release);
    }

    SpinLockGuard Lock( void) const noexcept
    {
        Acquire();
        return SpinLockGuard( this);
    }
};

//-----------------------------------------------------------------------------------------------------------------

inline SpinLockGuard::SpinLockGuard( const Spinlock *lock) noexcept
    : m_Lock( lock)
{
}

inline SpinLockGuard::~SpinLockGuard( void) noexcept
{
    if ( m_Lock) {
        m_Lock->Release();
    }
}

} // namespace xeom::stalks
