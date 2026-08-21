// main_frame.cpp ---------------------------------------------------------------------------------------------------
#include "fenst/main_frame.h"

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::fenst {

enum
{
    ID_Hello = 1
};

wxBEGIN_EVENT_TABLE( FenstMainFrame, wxFrame)
    EVT_MENU( ID_Hello,   FenstMainFrame::OnHello)
    EVT_MENU( wxID_EXIT,  FenstMainFrame::OnExit)
    EVT_MENU( wxID_ABOUT, FenstMainFrame::OnAbout)
wxEND_EVENT_TABLE()

FenstMainFrame::FenstMainFrame( const wxString &title)
    : wxFrame( nullptr, wxID_ANY, title, wxDefaultPosition, wxSize( 800, 600))
{
    wxMenu *menuFile = new wxMenu;
    menuFile->Append( ID_Hello, "&Hello...\tCtrl-H", "Show a greeting");
    menuFile->AppendSeparator();
    menuFile->Append( wxID_EXIT, "E&xit\tAlt-X", "Quit this program");

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append( wxID_ABOUT, "&About", "Show about dialog");

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, "&File");
    menuBar->Append( menuHelp, "&Help");
    SetMenuBar( menuBar);

    CreateStatusBar();
    SetStatusText( "Welcome to Xeom Fenst GUI!");

    // Basic central panel
    wxPanel *panel = new wxPanel( this, wxID_ANY);
    wxStaticText *text = new wxStaticText( panel, wxID_ANY, "Xeom Framework GUI initialized successfully.", wxPoint( 20, 20));
}

void FenstMainFrame::OnExit( wxCommandEvent &WXUNUSED( event))
{
    Close( true);
}

void FenstMainFrame::OnAbout( wxCommandEvent &WXUNUSED( event))
{
    wxMessageBox( "Xeom GUI subsystem (fenst).\nPowered by wxWidgets.",
                  "About Xeom Fenst", wxOK | wxICON_INFORMATION);
}

void FenstMainFrame::OnHello( wxCommandEvent &WXUNUSED( event))
{
    wxLogMessage( "Hello from Xeom Fenst!");
}

} // namespace xeom::fenst
