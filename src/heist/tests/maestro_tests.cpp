// maestro_tests.cpp -------------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "cove/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "heist::Maestro: index and successor id bookkeeping")
{
    using namespace xeom::heist;
    using namespace xeom::silo;

    Atelier atelier = Atelier::New( 4);
    {
        auto maestros = atelier.Maestros();
        Maestro &m2 = maestros[2];
        m2.SetAtelier( &atelier);
        m2.SetCurSuccId( 42);
    }
    auto maestros = atelier.Maestros();
    JEEVES_CHECK_MSG( maestros[2].MaestroIndex() == 2, "MaestroIndex == 2");
    JEEVES_CHECK_MSG( maestros[2].CurSuccId() == 42, "CurSuccId == 42");
}
