// buff_tests.cpp ----------------------------------------------------------------------------------------------------
#include "cove/xeom.h"
#include "jeeves/jeeves.h"

//-----------------------------------------------------------------------------------------------------------------

JEEVES_TEST( "silo::Buff: owning growable buffer")
{
    using namespace xeom::silo;

    Buff< int> emptyBuff = Buff< int>::NewEmpty();
    JEEVES_CHECK_MSG( emptyBuff.IsEmpty(), "Buff NewEmpty is empty");
    JEEVES_CHECK_MSG( emptyBuff.Size() == 0, "Buff NewEmpty Size == 0");

    Buff< int> fillBuff = Buff< int>::New( 4, 99);
    JEEVES_CHECK_MSG( fillBuff.Size() == 4, "Buff New size == 4");
    JEEVES_CHECK_MSG( fillBuff.First() == 99 && fillBuff.Last() == 99, "Buff New values initialized to 99");

    Buff< int> genBuff = Buff< int>::Create( 5, []( uint32_t i) {
        return static_cast< int>( i) * 10;
    });
    JEEVES_CHECK_MSG( genBuff.Size() == 5, "Buff Create size == 5");
    JEEVES_CHECK_MSG( genBuff[0] == 0 && genBuff[4] == 40, "Buff Create values 0..40");

    Buff< int> initBuff = { 1, 2, 3, 4, 5 };
    JEEVES_CHECK_MSG( initBuff.Size() == 5, "Buff initializer list size == 5");
    JEEVES_CHECK_MSG( initBuff[2] == 3, "Buff initializer list element [2] == 3");

    initBuff.Resize( 8, []( uint32_t i) {
        return static_cast< int>( i) * 100;
    });
    JEEVES_CHECK_MSG( initBuff.Size() == 8, "Buff Resize to 8");
    JEEVES_CHECK_MSG( initBuff[5] == 500 && initBuff[7] == 700, "Buff Resize dispenser values");

    int extra[2] = { 800, 900 };
    initBuff.ExtendFromSlice( extra);
    JEEVES_CHECK_MSG( initBuff.Size() == 10 && initBuff.Last() == 900, "Buff ExtendFromSlice");

    int bufA[2] = { 10, 20 };
    int bufB[3] = { 30, 40, 50 };
    Buff< int> concatBuff = Buff< int>::Concat( Arr< int>( bufA, 2), Arr< int>( bufB, 3));
    JEEVES_CHECK_MSG( concatBuff.Size() == 5, "Buff Concat size == 5");
    JEEVES_CHECK_MSG( concatBuff[0] == 10 && concatBuff[4] == 50, "Buff Concat contents [10..50]");

    Arr< int> buffArr = concatBuff;
    JEEVES_CHECK_MSG( buffArr.Size() == 5, "Buff implicitly converts to Arr with size == 5");

    IAccess< int> accessBuff = concatBuff.AsAccess();
    JEEVES_CHECK_MSG( accessBuff.Size() == 5 && accessBuff[1] == 20, "Buff AsAccess");

    IArr< int> iarrBuff = concatBuff.AsIArr();
    iarrBuff.Swap( 0, 4);
    JEEVES_CHECK_MSG( concatBuff[0] == 50 && concatBuff[4] == 10, "Buff mutated via IArr Swap");

    auto lsnip = concatBuff.LSnip( 2);
    JEEVES_CHECK_MSG( lsnip.Size() == 3 && lsnip[0] == 30, "Buff LSnip(2)");

    Buff< int> copyBuff = concatBuff;
    JEEVES_CHECK_MSG( copyBuff.Size() == concatBuff.Size(), "Buff copy constructor size match");
    JEEVES_CHECK_MSG( copyBuff[0] == concatBuff[0], "Buff copy constructor elements match");

    Buff< int> moveBuff = std::move( copyBuff);
    JEEVES_CHECK_MSG( moveBuff.Size() == 5, "Buff move constructor target has size 5");
    JEEVES_CHECK_MSG( copyBuff.IsEmpty(), "Buff moved-from is empty");

    JEEVES_CHECK_MSG( concatBuff.Format() == "[50, 20, 30, 40, 10]", "Buff Format");
}
