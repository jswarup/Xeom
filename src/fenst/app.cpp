// app.cpp ----------------------------------------------------------------------------------------------------------
#include "fenst/app.h"
#include "fenst/main_frame.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::fenst {

bool FenstApp::OnInit()
{
    if ( !wxApp::OnInit() ) {
        return false;
    }

    FenstMainFrame *frame = new FenstMainFrame( "Xeom Fenst Interface");
    frame->Show( true);
    
    return true;
}

} // namespace xeom::fenst
