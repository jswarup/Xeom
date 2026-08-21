// seg_tests.cpp -----------------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "jeeves/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "silo::Seg: bounds, slicing and traversal")
{
    using namespace xeom::silo;

    USeg seg = USeg::New( 2, 8);
    JEEVES_CHECK_MSG( seg.Begin() == 2, "Seg Begin == 2");
    JEEVES_CHECK_MSG( seg.Size() == 8, "Seg Size == 8");
    JEEVES_CHECK_MSG( seg.End() == 10, "Seg End == 10");
    JEEVES_CHECK_MSG( !seg.IsEmpty(), "Seg IsEmpty is false");

    USeg emptySeg = USeg::New( 0, 0);
    JEEVES_CHECK_MSG( emptySeg.IsEmpty(), "Seg IsEmpty is true for size 0");

    USeg rsnip = seg.RSnip( 3);
    JEEVES_CHECK_MSG( rsnip.Begin() == 2 && rsnip.Size() == 5, "Seg RSnip(3)");

    USeg lsnip = seg.LSnip( 3);
    JEEVES_CHECK_MSG( lsnip.Begin() == 5 && lsnip.Size() == 5, "Seg LSnip(3)");

    uint32_t count = 0;
    seg.Traverse( [&]( uint32_t) {
        ++count;
    });
    JEEVES_CHECK_MSG( count == 8, "Seg Traverse visited 8 elements");

    bool spanOk = seg.Span( []( uint32_t idx) {
        return idx >= 2 && idx < 10;
    });
    JEEVES_CHECK_MSG( spanOk, "Seg Span verified interval [2, 10)");
}
