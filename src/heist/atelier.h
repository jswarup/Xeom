// atelier.h --------------------------------------------------------------------------------------------------------
#pragma once

#include "cove/typeincl.h"
#include "heist/maestro.h"
#include "heist/choretree.h"
#include "silo/buff.h"
#include "silo/stash.h"
#include "stalks/atm.h"
#include "stalks/work.h"
#include <iostream>
#include <thread>
#include <vector>

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::heist {

//-----------------------------------------------------------------------------------------------------------------
// Atelier — central work-stealing job pool, DAG dependency resolver, and multi-threaded orchestrator.

class Atelier
{
public:
    mutable stalks::Atm< uint32_t> m_SzSchedJob{0};

private:
    static constexpr uint32_t k_JobCapacity = 65536;

    silo::Buff< Maestro>                     m_Maestros{};
    silo::Buff< stalks::Atm< uint16_t>>      m_SzPreds{};
    silo::Buff< uint16_t>                    m_SuccIds{};
    stalks::Spinlock                         m_FreeJobLock{};
    silo::Stash< uint16_t>                   m_FreeJobStash{};
    silo::Buff< stalks::WorkPtr>             m_JobBuff{};
    silo::Buff< const char *>                m_JobDocBuff{};
    uint16_t                                 m_Terminal{0};

public:
    constexpr Atelier( void) noexcept = default;

    explicit Atelier( uint32_t szMaestro)
        : m_SzSchedJob( 0),
          m_Maestros( szMaestro, []( uint32_t i ) { return Maestro::New( i); }),
          m_SzPreds( k_JobCapacity, []( uint32_t ) { return stalks::Atm< uint16_t>( 0); }),
          m_SuccIds( k_JobCapacity, static_cast< uint16_t>( 0)),
          m_FreeJobStash( k_JobCapacity, 0, static_cast< uint16_t>( 0)),
          m_JobBuff( k_JobCapacity, stalks::WorkPtr::Null()),
          m_JobDocBuff( k_JobCapacity, "Free"),
          m_Terminal( 0)
    {
        m_FreeJobStash.DoIndexSetup();
        m_Terminal = ConstructJob( 0, 0, stalks::WorkPtr::Dummy(), "Terminal");
        m_Maestros[0].SetCurSuccId( m_Terminal);
    }

    static Atelier New( uint32_t szMaestro)
    {
        return Atelier( szMaestro);
    }

    Maestro *MainMaestro( void) noexcept
    {
        Maestro *m = &m_Maestros[0];
        m->SetAtelier( this);
        return m;
    }

    uint16_t Terminal( void) const noexcept
    {
        return m_Terminal;
    }

    silo::Arr< Maestro> Maestros( void) noexcept
    {
        return m_Maestros;
    }

    uint16_t SuccId( uint16_t jobId) const noexcept
    {
        return m_SuccIds[jobId];
    }

    stalks::Atm< uint16_t> *SzPred( uint16_t jobId) noexcept
    {
        return &m_SzPreds[jobId];
    }

    const char *FreeDocStr( void) const noexcept
    {
        return "Free";
    }

    uint16_t AllocJob( uint32_t maestroIdx)
    {
        Maestro &maestro = m_Maestros[maestroIdx];
        auto jobCacheStk = maestro.JobCacheStk();
        while ( true ) {
            uint16_t jobId = 0;
            if ( jobCacheStk.Size() != 0 && jobCacheStk.Pop( jobId) ) {
                return jobId;
            }
            if ( m_FreeJobStash.Size() == 0 ) {
                std::this_thread::yield();
                continue;
            }
            auto guard = m_FreeJobLock.Lock();
            m_FreeJobStash.StkView().Export( jobCacheStk, UINT32_MAX);
        }
    }

    bool FreeJob( uint32_t maestroIdx, uint16_t jobId)
    {
        m_JobDocBuff.SetAt( jobId, FreeDocStr());
        Maestro &maestro = m_Maestros[maestroIdx];
        maestro.FlushTempQueue();
        auto jobCacheStk = maestro.JobCacheStk();
        while ( true ) {
            if ( jobCacheStk.SzVoid() != 0 && jobCacheStk.PushX( jobId) ) {
                return true;
            }
            auto guard = m_FreeJobLock.Lock();
            m_FreeJobStash.StkView().Import( jobCacheStk, UINT32_MAX);
        }
    }

    void SetSucc( uint16_t jobId, uint16_t succId)
    {
        m_SuccIds.SetAt( jobId, succId);
        SzPred( succId)->Add( 1);
    }

    uint16_t ConstructJob( uint32_t maestroIdx, uint16_t succId, stalks::WorkPtr job, const char *docStr)
    {
        uint16_t jobId = AllocJob( maestroIdx);
        if ( jobId == 0 ) {
            return jobId;
        }
        m_JobBuff.SetAt( jobId, job);
        m_JobDocBuff.SetAt( jobId, docStr);
        if ( succId != 0 ) {
            SetSucc( jobId, succId);
        }
        return jobId;
    }

    uint16_t GrabJob( uint32_t idx, uint32_t &stealSeed)
    {
        const uint32_t sz = m_Maestros.Size();
        constexpr uint32_t knuthMultHash = 2654435761u;
        stealSeed = stealSeed * knuthMultHash + 1u;
        for ( uint32_t mIdx = 0; mIdx < sz; ++mIdx ) {
            uint32_t maestroIdx = ( stealSeed + mIdx) % sz;
            if ( maestroIdx == idx ) {
                continue;
            }
            uint16_t jobId = m_Maestros[maestroIdx].PopJob();
            if ( jobId != 0 ) {
                return jobId;
            }
        }
        return 0;
    }

    void ExecuteLoop( uint32_t maestroIdx)
    {
        Maestro &maestro = m_Maestros[maestroIdx];
        maestro.SetAtelier( this);
        maestro.FlushTempQueue();
        uint16_t jobId = 0;
        uint32_t stealSeed = maestroIdx;

        while ( m_SzSchedJob.Load( std::memory_order_acquire) != 0 ) {
            while ( jobId != 0 ) {
                maestro.SetCurSuccId( m_SuccIds[jobId]);
                stalks::WorkPtr job = m_JobBuff[jobId];
                job.DoWork( &maestro);

                m_JobBuff.SetAt( jobId, stalks::WorkPtr::Null());
                maestro.m_SzProcessed += 1;

                FreeJob( maestroIdx, jobId);
                uint16_t succId = maestro.CurSuccId();
                if ( succId != 0 ) {
                    uint16_t szPred = SzPred( succId)->Add( static_cast< uint16_t>( -1));
                    if ( szPred == 1 ) {
                        jobId = succId;
                        m_SzSchedJob.Add( 1);
                    } else {
                        jobId = 0;
                    }
                } else {
                    jobId = 0;
                }
                m_SzSchedJob.Add( static_cast< uint32_t>( -1));
            }

            jobId = maestro.PopJob();
            if ( jobId == 0 ) {
                jobId = GrabJob( maestroIdx, stealSeed);
            }
            if ( jobId == 0 ) {
#if defined(__x86_64__) || defined(_M_X64)
                _mm_pause();
#endif
                std::this_thread::yield();
            }
        }
    }

    void DoLaunch( void)
    {
        const uint32_t sz = m_Maestros.Size();
        std::vector< std::jthread> threads;
        threads.reserve( sz > 0 ? sz - 1 : 0);

        for ( uint32_t i = 1; i < sz; ++i ) {
            threads.emplace_back( [this, i]( ) {
                this->ExecuteLoop( i);
            });
        }
        ExecuteLoop( 0);
        for ( auto &t : threads ) {
            if ( t.joinable() ) {
                t.join();
            }
        }
    }
};

//-----------------------------------------------------------------------------------------------------------------
// Inlined Maestro implementations requiring Atelier

inline uint16_t Maestro::ConstructJob( uint16_t succId, stalks::WorkPtr job, const char *docStr)
{
    return const_cast< Atelier *>( m_Atelier)->ConstructJob( m_Index, succId, job, docStr);
}

inline uint16_t Maestro::ConstructEnqueArr( uint16_t succId, silo::Buff< uint16_t> buff, const char *docStr)
{
    return ConstructJob(
        succId,
        stalks::WorkPtr::FromLambda( [buff]( stalks::IWorker *worker ) {
            Maestro *maestro = Maestro::FromWorker( worker);
            if ( maestro ) {
                for ( uint16_t jobId : buff ) {
                    maestro->EnqueueJob( jobId);
                }
            }
        }),
        docStr
    );
}

inline void Maestro::FlushTempQueue( void)
{
    auto arr = m_TempQueue.StkView().ArrView();
    for ( uint16_t jobId : arr ) {
        if ( jobId != 0 ) {
            const_cast< Atelier *>( m_Atelier)->m_SzSchedJob.Add( 1);
            EnqueRunJob( jobId);
        }
    }
    m_TempQueue.Clear();
}

inline void Maestro::PostJob( stalks::WorkPtr job)
{
    uint16_t succId = CurSuccId();
    uint16_t jobId = ConstructJob( succId, job, "PostJob");
    EnqueueJob( jobId);
}

template < typename TChoreNode>
inline void Maestro::PostChoreTree( const TChoreNode &node)
{
    silo::Stash< uint16_t> tails = silo::Stash< uint16_t>::New( 64, 0, static_cast< uint16_t>( 0));
    uint16_t head = PostChoreNode( node, this, tails);
    uint16_t succId = CurSuccId();
    uint16_t tail = 0;
    while ( tails.Pop( tail) ) {
        const_cast< Atelier *>( m_Atelier)->SetSucc( tail, succId);
    }
    EnqueueJob( head);
}

//-----------------------------------------------------------------------------------------------------------------
// Chore & BinNode Post implementations

inline uint16_t Chore::Post( const Maestro *maestro, silo::Stash< uint16_t> &tails) const
{
    uint16_t jobId = const_cast< Maestro *>( maestro)->ConstructJob( 0, stalks::WorkPtr::FromFn( m_Closure), m_DocStr);
    tails.Push( jobId);
    return jobId;
}

template < typename L, typename R, typename Op>
inline uint16_t PostChoreNode( const stalks::BinNode< L, R, Op> &node, const Maestro *maestro, silo::Stash< uint16_t> &tails)
{
    if ( node.m_Op == stalks::BinOp::Bor ) {
        silo::Stash< uint16_t> leftTails = silo::Stash< uint16_t>::New( 64, 0, static_cast< uint16_t>( 0));
        silo::Stash< uint16_t> rightTails = silo::Stash< uint16_t>::New( 64, 0, static_cast< uint16_t>( 0));
        uint16_t headL = PostChoreNode( node.m_Left, maestro, leftTails);
        uint16_t headR = PostChoreNode( node.m_Right, maestro, rightTails);

        uint16_t t = 0;
        while ( leftTails.Pop( t) ) {
            tails.Push( t);
        }
        while ( rightTails.Pop( t) ) {
            tails.Push( t);
        }

        silo::Buff< uint16_t> heads( 2, static_cast< uint16_t>( 0));
        heads[0] = headL;
        heads[1] = headR;
        return const_cast< Maestro *>( maestro)->ConstructEnqueArr( 0, std::move( heads), "EnqPar");
    } else if ( node.m_Op == stalks::BinOp::Less ) {
        silo::Stash< uint16_t> leftTails = silo::Stash< uint16_t>::New( 64, 0, static_cast< uint16_t>( 0));
        uint16_t headL = PostChoreNode( node.m_Left, maestro, leftTails);
        uint16_t headR = PostChoreNode( node.m_Right, maestro, tails);

        uint16_t leftTail = 0;
        while ( leftTails.Pop( leftTail) ) {
            const_cast< Atelier *>( maestro->AtelierRef())->SetSucc( leftTail, headR);
        }
        return headL;
    }
    return 0;
}

//-----------------------------------------------------------------------------------------------------------------
// JobInfo — diagnostic representation of a registered job.

struct JobInfo
{
    uint16_t    m_JobId{0};
    uint16_t    m_SuccId{0};
    uint16_t    m_SzPred{0};
    const char *m_DocStr{""};

    static JobInfo New( const Atelier *atelier, uint16_t jobId)
    {
        return {
            .m_JobId  = jobId,
            .m_SuccId = atelier->SuccId( jobId),
            .m_SzPred = const_cast< Atelier *>( atelier)->SzPred( jobId)->Load(),
            .m_DocStr = ""
        };
    }
};

} // namespace xeom::heist
