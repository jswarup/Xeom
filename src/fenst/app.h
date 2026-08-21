// app.h ------------------------------------------------------------------------------------------------------------
#pragma once

#include <wx/wx.h>
#include <wx/cmdline.h>

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::fenst {

class FenstApp : public wxApp
{
public:
    virtual bool OnInit( void) override;
    virtual void OnInitCmdLine( wxCmdLineParser &parser) override;
    virtual bool OnCmdLineParsed( wxCmdLineParser &parser) override;
};

} // namespace xeom::fenst
