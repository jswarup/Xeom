// app.cpp ----------------------------------------------------------------------------------------------------------
#include "fenst/app.h"
#include "fenst/main_frame.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::fenst {

void FenstApp::OnInitCmdLine( wxCmdLineParser &parser)
{
    parser.SetCmdLine( 0, static_cast< char **>( nullptr));
}

bool FenstApp::OnCmdLineParsed( wxCmdLineParser &WXUNUSED( parser))
{
    return true;
}

bool FenstApp::OnInit( void)
{
    if ( !wxApp::OnInit()) {
        return false;
    }

    FenstMainFrame *frame = new FenstMainFrame( "Xeom Fenst Interface");
    frame->Show( true);
    SetTopWindow( frame);
    
    return true;
}

} // namespace xeom::fenst

wxIMPLEMENT_APP_NO_MAIN( xeom::fenst::FenstApp);

