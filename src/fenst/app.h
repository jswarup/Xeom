// app.h ------------------------------------------------------------------------------------------------------------
#pragma once

#include <wx/wx.h>

//-----------------------------------------------------------------------------------------------------------------

namespace xeom::fenst {

class FenstApp : public wxApp
{
public:
    virtual bool OnInit() override;
};

} // namespace xeom::fenst
