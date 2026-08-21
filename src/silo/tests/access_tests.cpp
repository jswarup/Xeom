// access_tests.cpp --------------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "jeeves/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "silo::IAccess: container-agnostic read access")
{
    using namespace xeom::silo;

    std::vector< int> vec = { 10, 20, 30, 40, 50 };
    IAccess< int> accessVec = vec;

    JEEVES_CHECK_MSG( accessVec.IsValid(), "IAccess bound to std::vector");
    JEEVES_CHECK_MSG( accessVec.Size() == 5, "IAccess Size == 5");
    JEEVES_CHECK_MSG( !accessVec.IsEmpty(), "IAccess IsEmpty is false");
    JEEVES_CHECK_MSG( accessVec.First() == 10, "IAccess First == 10");
    JEEVES_CHECK_MSG( accessVec.Last() == 50, "IAccess Last == 50");
    JEEVES_CHECK_MSG( accessVec[2] == 30, "IAccess operator[] == 30");
    JEEVES_CHECK_MSG( accessVec.At( 3) == 40, "IAccess At(3) == 40");

    auto seg = accessVec.USeg();
    JEEVES_CHECK_MSG( seg.Begin() == 0 && seg.Size() == 5, "IAccess USeg matches [0, 5)");

    int sumTraverse = 0;
    accessVec.Traverse( [&]( int val) {
        sumTraverse += val;
    });
    JEEVES_CHECK_MSG( sumTraverse == 150, "IAccess Traverse accumulated 150");

    bool allPositive = accessVec.Span( []( int val) {
        return val > 0;
    });
    JEEVES_CHECK_MSG( allPositive, "IAccess Span verified all positive");

    bool sorted = accessVec.SortSanity( []( int a, int b) {
        return a < b;
    });
    JEEVES_CHECK_MSG( sorted, "IAccess SortSanity verified ascending order");

    int sumIter = 0;
    for ( int x : accessVec) {
        sumIter += x;
    }
    JEEVES_CHECK_MSG( sumIter == 150, "IAccess range-based for loop accumulated 150");

    std::string formatted = accessVec.Format();
    JEEVES_CHECK_MSG( formatted == "[10, 20, 30, 40, 50]", "IAccess Format matches [10, 20, 30, 40, 50]");

    std::array< float, 3> arr = { 1.5f, 2.5f, 3.5f };
    IAccess< float> accessArr = arr;
    JEEVES_CHECK_MSG( accessArr.Size() == 3, "IAccess bound to std::array with Size 3");
    JEEVES_CHECK_MSG( accessArr.First() == 1.5f && accessArr.Last() == 3.5f, "IAccess First/Last on std::array");

    int rawArr[3] = { 20, 30, 40 };
    Arr< int> xeomArr = rawArr;
    IAccess< int> accessArrDirect = xeomArr;
    JEEVES_CHECK_MSG( accessArrDirect.Size() == 3, "IAccess bound to Arr with Size 3");
    JEEVES_CHECK_MSG( accessArrDirect.First() == 20 && accessArrDirect.Last() == 40, "IAccess First/Last on Arr");
}
