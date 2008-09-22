///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 21 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "visualization_panel_generated.h"

///////////////////////////////////////////////////////////////////////////

VisualizationPanelGenerated::VisualizationPanelGenerated( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style ) : wxPanel( parent, id, pos, size, style )
{
	wxBoxSizer* bSizer23;
	bSizer23 = new wxBoxSizer( wxVERTICAL );
	
	main_splitter_ = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	main_splitter_->SetMinimumPaneSize( 100 );
	main_splitter_->Connect( wxEVT_IDLE, wxIdleEventHandler( VisualizationPanelGenerated::main_splitter_OnIdle ), NULL, this );
	left_panel_ = new wxPanel( main_splitter_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	left_panel_->SetMinSize( wxSize( 200,-1 ) );
	
	wxBoxSizer* bSizer25;
	bSizer25 = new wxBoxSizer( wxVERTICAL );
	
	bSizer25->SetMinSize( wxSize( 200,200 ) ); 
	display_splitter_ = new wxSplitterWindow( left_panel_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
	display_splitter_->SetSashGravity( 1 );
	display_splitter_->SetMinimumPaneSize( 100 );
	display_splitter_->Connect( wxEVT_IDLE, wxIdleEventHandler( VisualizationPanelGenerated::display_splitter_OnIdle ), NULL, this );
	displays_panel_ = new wxPanel( display_splitter_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	displays_panel_->SetMinSize( wxSize( 200,100 ) );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText1 = new wxStaticText( displays_panel_, wxID_ANY, wxT("Displays"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer8->Add( m_staticText1, 0, wxALL, 5 );
	
	wxArrayString displays_Choices;
	displays_ = new wxCheckListBox( displays_panel_, wxID_ANY, wxDefaultPosition, wxDefaultSize, displays_Choices, 0 );
	displays_->SetMinSize( wxSize( 150,-1 ) );
	
	bSizer8->Add( displays_, 1, wxALL|wxEXPAND, 5 );
	
	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer( wxHORIZONTAL );
	
	new_display_ = new wxButton( displays_panel_, wxID_ANY, wxT("Add"), wxDefaultPosition, wxDefaultSize, 0 );
	new_display_->SetToolTip( wxT("Add a new display.") );
	
	bSizer7->Add( new_display_, 0, wxALL, 5 );
	
	delete_display_ = new wxButton( displays_panel_, wxID_ANY, wxT("Remove"), wxDefaultPosition, wxDefaultSize, 0 );
	delete_display_->SetToolTip( wxT("Remove the selected display.  Displays with a '*' next to them are defaults, and cannot be removed.") );
	
	bSizer7->Add( delete_display_, 0, wxALL, 5 );
	
	bSizer8->Add( bSizer7, 0, wxEXPAND, 5 );
	
	displays_panel_->SetSizer( bSizer8 );
	displays_panel_->Layout();
	bSizer8->Fit( displays_panel_ );
	display_properties_panel_ = new wxPanel( display_splitter_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	display_properties_panel_->SetMinSize( wxSize( 200,100 ) );
	
	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText2 = new wxStaticText( display_properties_panel_, wxID_ANY, wxT("Display Properties"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizer9->Add( m_staticText2, 0, wxALL, 5 );
	
	properties_panel_ = new wxPanel( display_properties_panel_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	properties_panel_sizer_ = new wxBoxSizer( wxVERTICAL );
	
	properties_panel_->SetSizer( properties_panel_sizer_ );
	properties_panel_->Layout();
	properties_panel_sizer_->Fit( properties_panel_ );
	bSizer9->Add( properties_panel_, 1, wxEXPAND | wxALL, 5 );
	
	display_properties_panel_->SetSizer( bSizer9 );
	display_properties_panel_->Layout();
	bSizer9->Fit( display_properties_panel_ );
	display_splitter_->SplitHorizontally( displays_panel_, display_properties_panel_, 392 );
	bSizer25->Add( display_splitter_, 1, wxEXPAND, 5 );
	
	left_panel_->SetSizer( bSizer25 );
	left_panel_->Layout();
	bSizer25->Fit( left_panel_ );
	render_panel_ = new wxPanel( main_splitter_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	render_sizer_ = new wxBoxSizer( wxVERTICAL );
	
	views_ = new wxToolBar( render_panel_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL|wxTB_NOICONS|wxTB_TEXT ); 
	views_->Realize();
	
	render_sizer_->Add( views_, 0, wxEXPAND, 0 );
	
	render_panel_->SetSizer( render_sizer_ );
	render_panel_->Layout();
	render_sizer_->Fit( render_panel_ );
	main_splitter_->SplitVertically( left_panel_, render_panel_, 200 );
	bSizer23->Add( main_splitter_, 1, wxEXPAND, 5 );
	
	this->SetSizer( bSizer23 );
	this->Layout();
	
	// Connect Events
	new_display_->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( VisualizationPanelGenerated::onNewDisplay ), NULL, this );
	delete_display_->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( VisualizationPanelGenerated::onDeleteDisplay ), NULL, this );
}

VisualizationPanelGenerated::~VisualizationPanelGenerated()
{
	// Disconnect Events
	new_display_->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( VisualizationPanelGenerated::onNewDisplay ), NULL, this );
	delete_display_->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( VisualizationPanelGenerated::onDeleteDisplay ), NULL, this );
}

NewDisplayDialogGenerated::NewDisplayDialogGenerated( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Display Type") ), wxVERTICAL );
	
	types_ = new wxListBox( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	sbSizer1->Add( types_, 1, wxALL|wxEXPAND, 5 );
	
	bSizer8->Add( sbSizer1, 1, wxEXPAND, 5 );
	
	wxStaticBoxSizer* sbSizer2;
	sbSizer2 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Display Name") ), wxVERTICAL );
	
	name_ = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	sbSizer2->Add( name_, 0, wxALL|wxEXPAND, 5 );
	
	bSizer8->Add( sbSizer2, 0, wxEXPAND, 5 );
	
	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();
	bSizer8->Add( m_sdbSizer1, 0, wxEXPAND, 5 );
	
	this->SetSizer( bSizer8 );
	this->Layout();
	
	// Connect Events
	name_->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( NewDisplayDialogGenerated::onNameEnter ), NULL, this );
	m_sdbSizer1Cancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( NewDisplayDialogGenerated::onCancel ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( NewDisplayDialogGenerated::onOK ), NULL, this );
}

NewDisplayDialogGenerated::~NewDisplayDialogGenerated()
{
	// Disconnect Events
	name_->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( NewDisplayDialogGenerated::onNameEnter ), NULL, this );
	m_sdbSizer1Cancel->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( NewDisplayDialogGenerated::onCancel ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( NewDisplayDialogGenerated::onOK ), NULL, this );
}
