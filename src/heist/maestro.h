// maestro.h --------------------------------------------------------------------------------------------------------
#pragma once

#include "cove/typeincl.h"
#include "silo/buff.h"
#include "silo/stash.h"
#include "stalks/atm.h"
#include "stalks/work.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::heist {

class Atelier;

//-----------------------------------------------------------------------------------------------------------------
// Maestro — per-thread worker context managing job caching, scheduling queues, and work execution.

class Maestro : public stalks::IWorker
{
public:
    uint32_t                   m_SzProcessed{0};

private:
    uint32_t                   m_Index{0};
    Atelier                   *m_Atelier{nullptr};
    silo::Stash< uint16_t>     m_JobCache{};
    silo::Stash< uint16_t>     m_RunQueue{};
    stalks::Spinlock           m_RunQLock{};
    stalks::Atm< uint16_t>     m_CurSuccId{0};
    silo::Stash< uint16_t>     m_TempQueue{};

public:
    constexpr Maestro( void) noexcept = default;

    explicit Maestro( uint32_t maestroInd)
        : m_Index( maestroInd),
          m_JobCache( 256, 0, static_cast< uint16_t>( 0)),
          m_RunQueue( 1024, 0, static_cast< uint16_t>( 0)),
          m_CurSuccId( 0),
          m_TempQueue( 64, 0, static_cast< uint16_t>( 0))
    {
    }

    static Maestro New( uint32_t maestroInd)
    {
        return Maestro( maestroInd);
    }

    void SetAtelier( Atelier *atelier) noexcept
    {
        m_Atelier = atelier;
    }

    Atelier *AtelierRef( void) noexcept
    {
        return m_Atelier;
    }

    const Atelier *AtelierRef( void) const noexcept
    {
        return m_Atelier;
    }

    uint32_t MaestroIndex( void) const noexcept
    {
        return m_Index;
    }

    static Maestro *FromWorker( stalks::IWorker *worker) noexcept
    {
        return static_cast< Maestro *>( worker);
    }

    uint16_t ConstructJob( uint16_t succId, stalks::WorkPtr job);

    void EnqueueJob( uint16_t jobId)
    {
        m_TempQueue.Push( jobId);
    }

    uint16_t ConstructEnqueArr( uint16_t succId, silo::Buff< uint16_t> buff);

    silo::Stk< uint16_t> JobCacheStk( void) const noexcept
    {
        return m_JobCache.StkView();
    }

    silo::Arr< uint16_t> RunQueueArr( void) const noexcept
    {
        return m_RunQueue.StkView().ArrView();
    }

    void FlushTempQueue( void);

    void EnqueRunJob( uint16_t jobId)
    {
        auto guard = m_RunQLock.Lock();
        m_RunQueue.Push( jobId);
    }

    uint16_t PopJob( void)
    {
        uint16_t jobId = 0;
        if ( m_RunQueue.Size() != 0 ) {
            auto guard = m_RunQLock.Lock();
            if ( m_RunQueue.Pop( jobId) ) {
                return jobId;
            }
        }
        return 0;
    }

    uint16_t CurSuccId( void) const noexcept
    {
        return m_CurSuccId.Load( std::memory_order_acquire);
    }

    void SetCurSuccId( uint16_t val) noexcept
    {
        m_CurSuccId.Store( val, std::memory_order_release);
    }

template < typename TChoreNode>
    void PostChoreTree( const TChoreNode &node);

    void PostJob( stalks::WorkPtr job) override;
};

} // namespace xeom::heist
