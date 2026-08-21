// arr_tests.cpp -----------------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "jeeves/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "silo::Arr: mutable slice operations")
{
    using namespace xeom::silo;

    int buffer[5] = { 10, 20, 30, 40, 50 };
    Arr< int> arr( buffer, 5);

    JEEVES_CHECK_MSG( arr.Size() == 5, "Arr Size == 5");
    JEEVES_CHECK_MSG( !arr.IsEmpty(), "Arr IsEmpty is false");
    JEEVES_CHECK_MSG( arr.First() == 10 && arr.Last() == 50, "Arr First/Last access");
    JEEVES_CHECK_MSG( arr[2] == 30, "Arr operator[] == 30");

    arr.SetAt( 0, 99);
    JEEVES_CHECK_MSG( arr.First() == 99, "Arr SetAt(0, 99)");
    arr.SetAt( 0, 10);

    int swapVal = 77;
    arr.SwapAt( 1, swapVal);
    JEEVES_CHECK_MSG( arr[1] == 77 && swapVal == 20, "Arr SwapAt swapped values");
    arr.SetAt( 1, 20);

    arr.Swap( 0, 4);
    JEEVES_CHECK_MSG( arr[0] == 50 && arr[4] == 10, "Arr Swap(0, 4)");
    arr.Swap( 0, 4); // restore

    auto lsnip = arr.LSnip( 2);
    JEEVES_CHECK_MSG( lsnip.Size() == 3 && lsnip[0] == 30 && lsnip[2] == 50, "Arr LSnip(2) matches [30, 40, 50]");

    auto rsnip = arr.RSnip( 2);
    JEEVES_CHECK_MSG( rsnip.Size() == 3 && rsnip[0] == 10 && rsnip[2] == 30, "Arr RSnip(2) matches [10, 20, 30]");

    auto subset = arr.Subset( 1, 3);
    JEEVES_CHECK_MSG( subset.Size() == 3 && subset[0] == 20 && subset[2] == 40, "Arr Subset(1, 3) matches [20, 30, 40]");

    int srcBuf[3] = { 100, 200, 300 };
    Arr< int> srcArr( srcBuf, 3);
    arr.SwapFrom( 1, srcArr, 0, 3);
    JEEVES_CHECK_MSG( arr[1] == 100 && arr[2] == 200 && arr[3] == 300, "Arr SwapFrom transferred elements");
    JEEVES_CHECK_MSG( srcArr[0] == 20 && srcArr[1] == 30 && srcArr[2] == 40, "Arr SwapFrom source received original elements");
    arr.SwapFrom( 1, srcArr, 0, 3); // restore

    int idxBuf[6] = { 0 };
    Arr< int> idxArr( idxBuf, 6);
    idxArr.DoIndexSetup();
    JEEVES_CHECK_MSG( idxArr[0] == 0 && idxArr[5] == 5, "Arr DoIndexSetup initialized indices 0..5");
    JEEVES_CHECK_MSG( idxArr.SortSanity( []( int a, int b) { return a < b; }), "Arr SortSanity on indexed array");

    JEEVES_CHECK_MSG( arr.Format() == "[10, 20, 30, 40, 50]", "Arr Format matches [10, 20, 30, 40, 50]");

    std::string_view hello = "Xeom";
    Arr< char> charArr( hello);
    JEEVES_CHECK_MSG( charArr.Size() == 4 && charArr.AsStringView() == "Xeom", "Arr<char> from string_view");

    std::vector< int> mutVec = { 5, 4, 3, 2, 1 };
    IArr< int> iarr = mutVec;
    JEEVES_CHECK_MSG( iarr.IsValid(), "IArr bound to std::vector");
    JEEVES_CHECK_MSG( iarr.Size() == 5, "IArr Size == 5");
    JEEVES_CHECK_MSG( iarr[0] == 5 && iarr.Last() == 1, "IArr indexing");

    iarr.Swap( 0, 4);
    JEEVES_CHECK_MSG( mutVec[0] == 1 && mutVec[4] == 5, "IArr Swap mutated underlying std::vector");

    IAccess< int> accessFromArr = arr.AsAccess();
    JEEVES_CHECK_MSG( accessFromArr.Size() == 5 && accessFromArr[2] == 30, "Arr converted to IAccess");
}
