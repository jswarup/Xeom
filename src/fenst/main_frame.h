// main_frame.h -----------------------------------------------------------------------------------------------------
#pragma once

#include <wx/wx.h>

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::fenst {

class FenstMainFrame : public wxFrame
{
public:
    FenstMainFrame( const wxString &title);

private:
    void OnHello( wxCommandEvent &event);
    void OnExit( wxCommandEvent &event);
    void OnAbout( wxCommandEvent &event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace xeom::fenst
