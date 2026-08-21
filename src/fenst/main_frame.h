// main_frame.h -----------------------------------------------------------------------------------------------------
#pragma once

#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/dirctrl.h>
#include <wx/artprov.h>

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::fenst {

class FenstMainFrame : public wxFrame
{
private:
    wxSplitterWindow *m_splitter{nullptr};
    wxGenericDirCtrl *m_dirCtrl{nullptr};
    wxPanel          *m_contentPane{nullptr};

public:
    FenstMainFrame( const wxString &title);

private:
    void OnHello( wxCommandEvent &event);
    void OnExit( wxCommandEvent &event);
    void OnAbout( wxCommandEvent &event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace xeom::fenst
