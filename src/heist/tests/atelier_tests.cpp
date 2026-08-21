// atelier_tests.cpp -------------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "cove/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "heist::Atelier: DoLaunch executes master & child jobs")
{
    using namespace xeom::heist;
    using namespace xeom::stalks;
    using namespace xeom::silo;

    static std::atomic< int> executedCount{0};
    executedCount = 0;

    Atelier atelier = Atelier::New( 4);
    Maestro *mainMaestro = atelier.MainMaestro();

    uint16_t jobId = mainMaestro->ConstructJob(
        0,
        WorkPtr::FromLambda( []( IWorker *w ) {
            Maestro *m = Maestro::FromWorker( w);
            executedCount += 1;
            uint16_t child1 = m->ConstructJob(
                m->CurSuccId(),
                WorkPtr::FromLambda( []( IWorker * ) {
                    executedCount += 10;
                }),
                "Child1"
            );
            m->EnqueueJob( child1);
        }),
        "TrialJob"
    );
    mainMaestro->EnqueueJob( jobId);
    atelier.DoLaunch();

    JEEVES_CHECK_MSG( executedCount == 11, "Atelier DoLaunch executed master & child jobs");
}
