// stash_tests.cpp ---------------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "cove/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "silo::Stash: push/pop LIFO order")
{
    using namespace xeom::silo;

    Stash< int> stash = Stash< int>::New( 16, 0, 0);
    JEEVES_CHECK_MSG( stash.Size() == 0, "Stash initial size 0");
    stash.Push( 100);
    stash.Push( 200);
    JEEVES_CHECK_MSG( stash.Size() == 2, "Stash size after 2 pushes");

    int val = 0;
    JEEVES_CHECK_MSG( stash.Pop( val) && val == 200, "Stash Pop top element");
    JEEVES_CHECK_MSG( stash.Pop( val) && val == 100, "Stash Pop second element");
    JEEVES_CHECK_MSG( stash.Size() == 0, "Stash empty after popping all");
}
