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

    // Create Toolbar
    wxToolBar *toolBar = CreateToolBar( wxTB_FLAT | wxTB_HORIZONTAL);
    toolBar->AddTool( ID_Hello, "Greeting", wxArtProvider::GetBitmap( wxART_INFORMATION, wxART_TOOLBAR), "Show Greeting");
    toolBar->AddSeparator();
    toolBar->AddTool( wxID_ABOUT, "About", wxArtProvider::GetBitmap( wxART_HELP, wxART_TOOLBAR), "About Xeom");
    toolBar->AddTool( wxID_EXIT, "Exit", wxArtProvider::GetBitmap( wxART_QUIT, wxART_TOOLBAR), "Quit Application");
    toolBar->Realize();

    // Create the main splitter window
    m_splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    m_splitter->SetMinimumPaneSize( 150);

    // Left Pane: File Explorer
    m_dirCtrl = new wxGenericDirCtrl( m_splitter, wxID_ANY, wxGetCwd(), wxDefaultPosition, wxDefaultSize, wxDIRCTRL_3D_INTERNAL | wxSUNKEN_BORDER);

    // Right Pane: Content Workspace
    m_contentPane = new wxPanel( m_splitter, wxID_ANY);
    wxStaticText *text = new wxStaticText( m_contentPane, wxID_ANY, "Xeom Framework GUI initialized successfully.\nSelect an operation from the left to begin.", wxPoint( 20, 20));

    // Split the window
    m_splitter->SplitVertically( m_dirCtrl, m_contentPane, 250);
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
