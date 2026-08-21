// choretree_tests.cpp -----------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "cove/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "heist::ChoreTree: DAG execution ordering")
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

    JEEVES_CHECK_MSG( traceIdx == 15, "ChoreTree executed all 4 jobs in DAG");
}
