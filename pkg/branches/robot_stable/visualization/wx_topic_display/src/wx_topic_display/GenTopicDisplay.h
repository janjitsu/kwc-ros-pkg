///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 21 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __GenTopicDisplay__
#define __GenTopicDisplay__

#include <wx/treectrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class GenTopicDisplay
///////////////////////////////////////////////////////////////////////////////
class GenTopicDisplay : public wxPanel 
{
	private:
	
	protected:
		wxTreeCtrl* topicTree;
		
		// Virtual event handlers, overide them in your derived class
		virtual void checkIsTopic( wxTreeEvent& event ){ event.Skip(); }
		
	
	public:
		GenTopicDisplay( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL );
		~GenTopicDisplay();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class GenTopicDisplayDialog
///////////////////////////////////////////////////////////////////////////////
class GenTopicDisplayDialog : public wxDialog 
{
	private:
	
	protected:
		wxPanel* m_TreePanel;
		wxButton* m_OK;
		wxButton* m_Cancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void onOK( wxCommandEvent& event ){ event.Skip(); }
		virtual void onCancel( wxCommandEvent& event ){ event.Skip(); }
		
	
	public:
		GenTopicDisplayDialog( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Browse Topics"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 600,425 ), long style = wxDEFAULT_DIALOG_STYLE );
		~GenTopicDisplayDialog();
	
};

#endif //__GenTopicDisplay__
