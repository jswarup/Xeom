// primitives_tests.cpp ----------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "jeeves/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "stalks: Atm, Spinlock, Worker, BinNode primitives")
{
    using namespace xeom::stalks;

    Atm< uint32_t> atm( 10);
    JEEVES_CHECK_MSG( atm.Get() == 10, "Atm Get initial value");
    atm.Set( 20);
    JEEVES_CHECK_MSG( atm.Get() == 20, "Atm Set value");
    uint32_t prev = atm.FetchAdd( 5);
    JEEVES_CHECK_MSG( prev == 20 && atm.Get() == 25, "Atm FetchAdd");

    Spinlock spinlock;
    {
        auto guard = spinlock.Lock();
    }
    JEEVES_CHECK_MSG( true, "Spinlock RAII acquire & release");

    int executed = 0;
    Worker worker = Worker::New();
    worker.Post( [&]( IWorker * ) {
        executed += 1;
    });
    JEEVES_CHECK_MSG( executed == 1, "Worker executed posted lambda");

    BinNode< int, int> node( 10, 20, BinOp::Sum);
    JEEVES_CHECK_MSG( node.m_Left == 10 && node.m_Right == 20 && node.m_Op == BinOp::Sum, "BinNode structure");
}
