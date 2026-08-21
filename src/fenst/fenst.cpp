// fenst.cpp --------------------------------------------------------------------------------------------------------
#include "fenst/fenst.h"
#include "fenst/app.h"
#include <wx/wx.h>

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::fenst {

int run_gui( int argc, char *argv[])
{
    // Instead of using wxIMPLEMENT_APP which hijacks main(), we manually initialize
    // the wxWidgets framework and start the application loop.
    wxApp::SetInstance( new FenstApp());
    wxEntryStart( argc, argv);
    int exitCode = wxTheApp->OnRun();
    wxEntryCleanup();
    return exitCode;
}

} // namespace xeom::fenst
