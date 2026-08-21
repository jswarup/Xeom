// fenst.cpp --------------------------------------------------------------------------------------------------------
#include "fenst/fenst.h"
#include "fenst/app.h"
#include <wx/wx.h>

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::fenst {

int run_gui( int argc, char *argv[])
{
    ( void)argc;
    char *fakeArgv[] = { argv[0], nullptr };
    int   fakeArgc   = 1;
    return wxEntry( fakeArgc, fakeArgv);
}

} // namespace xeom::fenst
