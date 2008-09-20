/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "visualization_panel.h"
#include "visualizer_base.h"
#include "common.h"
#include "factory.h"
#include "new_display_dialog.h"

#include "ogre_tools/wx_ogre_render_window.h"
#include "ogre_tools/fps_camera.h"
#include "ogre_tools/orbit_camera.h"

#include "ros/common.h"
#include "ros/node.h"
#include <rosTF/rosTF.h>

#include <Ogre.h>
#include <wx/timer.h>
#include <wx/propgrid/propgrid.h>
#include <wx/confbase.h>

#include <boost/bind.hpp>

namespace ogre_vis
{

namespace IDs
{
enum ID
{
  FPS = wxID_HIGHEST + 1,
  Orbit,
};
}
typedef IDs::ID ID;

BEGIN_DECLARE_EVENT_TYPES()
DECLARE_EVENT_TYPE(EVT_RENDER, wxID_ANY)
END_DECLARE_EVENT_TYPES()

DEFINE_EVENT_TYPE(EVT_RENDER)

VisualizationPanel::VisualizationPanel( wxWindow* parent )
    : VisualizationPanelGenerated( parent )
    , ogre_root_( Ogre::Root::getSingletonPtr() )
    , selected_visualizer_( NULL )
    , left_mouse_down_( false )
    , middle_mouse_down_( false )
    , right_mouse_down_( false )
    , mouse_x_( 0 )
    , mouse_y_( 0 )
{
  initializeCommon();
  registerFactories( this );

  ros_node_ = ros::node::instance();

  /// @todo This should go away once creation of the ros::node is more well-defined
  if (!ros_node_)
  {
    int argc = 0;
    ros::init( argc, 0 );
    ros_node_ = new ros::node( "VisualizationPanel" );
  }
  ROS_ASSERT( ros_node_ );

  tf_client_ = new rosTFClient( *ros_node_ );

  scene_manager_ = ogre_root_->createSceneManager( Ogre::ST_GENERIC );
  ray_scene_query_ = scene_manager_->createRayQuery( Ogre::Ray() );

  render_panel_ = new ogre_tools::wxOgreRenderWindow( ogre_root_, VisualizationPanelGenerated::render_panel_ );
  render_sizer_->Add( render_panel_, 1, wxALL|wxEXPAND, 0 );

  selection_bounds_particle_system_ = scene_manager_->createParticleSystem( "VisualizationPanelSelectionBoundsParticleSystem" );
  selection_bounds_particle_system_->setMaterialName( "BaseWhiteNoLighting" );
  Ogre::SceneNode* node = scene_manager_->getRootSceneNode()->createChildSceneNode();
  node->attachObject( selection_bounds_particle_system_ );

  fps_camera_ = new ogre_tools::FPSCamera( scene_manager_ );
  fps_camera_->getOgreCamera()->setNearClipDistance( 0.1f );
  fps_camera_->setPosition( 0, 0, 15 );

  orbit_camera_ = new ogre_tools::OrbitCamera( scene_manager_ );
  orbit_camera_->getOgreCamera()->setNearClipDistance( 0.1f );
  orbit_camera_->setPosition( 0, 0, 15 );

  Ogre::Light* directional_light = scene_manager_->createLight( "MainDirectional" );
  directional_light->setType( Ogre::Light::LT_DIRECTIONAL );
  directional_light->setDirection( Ogre::Vector3( 0, -1, 1 ) );
  directional_light->setDiffuseColour( Ogre::ColourValue( 1.0f, 1.0f, 1.0f ) );

  current_camera_ = orbit_camera_;

  render_panel_->getViewport()->setCamera( current_camera_->getOgreCamera() );

  views_->AddRadioTool( IDs::Orbit, wxT("Orbit"), wxNullBitmap, wxNullBitmap, wxT("Orbit Camera Controls") );
  views_->AddRadioTool( IDs::FPS, wxT("FPS"), wxNullBitmap, wxNullBitmap, wxT("FPS Camera Controls") );

  views_->Connect( wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( VisualizationPanel::onViewClicked ), NULL, this );

  displays_->Connect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler( VisualizationPanel::onDisplayToggled ), NULL, this );
  displays_->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( VisualizationPanel::onDisplaySelected ), NULL, this );

  render_panel_->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Connect( wxEVT_MOTION, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Connect( wxEVT_RIGHT_UP, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );

  render_panel_->setPreRenderCallback( boost::bind( &VisualizationPanel::lockRender, this ) );
  render_panel_->setPostRenderCallback( boost::bind( &VisualizationPanel::unlockRender, this ) );

  update_timer_ = new wxTimer( this );
  update_timer_->Start( 10 );
  update_stopwatch_.Start();
  Connect( update_timer_->GetId(), wxEVT_TIMER, wxTimerEventHandler( VisualizationPanel::onUpdate ), NULL, this );

  Connect( EVT_RENDER, wxCommandEventHandler( VisualizationPanel::onRender ), NULL, this );

  property_grid_ = new wxPropertyGrid( properties_panel_, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxPG_SPLITTER_AUTO_CENTER | wxTAB_TRAVERSAL | wxPG_DEFAULT_STYLE );
  properties_panel_sizer_->Add( property_grid_, 1, wxEXPAND, 5 );

  property_grid_->SetExtraStyle( wxPG_EX_HELP_AS_TOOLTIPS );

  property_grid_->Connect( wxEVT_PG_CHANGING, wxPropertyGridEventHandler( VisualizationPanel::onPropertyChanging ), NULL, this );
  property_grid_->Connect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( VisualizationPanel::onPropertyChanged ), NULL, this );
}

VisualizationPanel::~VisualizationPanel()
{
  views_->Disconnect( wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( VisualizationPanel::onViewClicked ), NULL, this );

  displays_->Disconnect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, wxCommandEventHandler( VisualizationPanel::onDisplayToggled ), NULL, this );
  displays_->Disconnect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( VisualizationPanel::onDisplaySelected ), NULL, this );

  render_panel_->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Disconnect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Disconnect( wxEVT_MOTION, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Disconnect( wxEVT_LEFT_UP, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Disconnect( wxEVT_MIDDLE_UP, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Disconnect( wxEVT_RIGHT_UP, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Disconnect( wxEVT_MOUSEWHEEL, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );
  render_panel_->Disconnect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( VisualizationPanel::onRenderWindowMouseEvents ), NULL, this );

  Disconnect( wxEVT_TIMER, update_timer_->GetId(), wxTimerEventHandler( VisualizationPanel::onUpdate ), NULL, this );
  delete update_timer_;

  Disconnect( EVT_RENDER, wxCommandEventHandler( VisualizationPanel::onRender ), NULL, this );

  property_grid_->Disconnect( wxEVT_PG_CHANGING, wxPropertyGridEventHandler( VisualizationPanel::onPropertyChanging ), NULL, this );
  property_grid_->Disconnect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( VisualizationPanel::onPropertyChanged ), NULL, this );
  property_grid_->Destroy();

  V_Visualizer::iterator vis_it = visualizers_.begin();
  V_Visualizer::iterator vis_end = visualizers_.end();
  for ( ; vis_it != vis_end; ++vis_it )
  {
    VisualizerBase* visualizer = *vis_it;
    delete visualizer;
  }
  visualizers_.clear();

  M_Factory::iterator factory_it = factories_.begin();
  M_Factory::iterator factory_end = factories_.end();
  for ( ; factory_it != factory_end; ++factory_it )
  {
    delete factory_it->second;
  }
  factories_.clear();

  render_panel_->Destroy();

  delete fps_camera_;
  delete orbit_camera_;

  scene_manager_->destroyParticleSystem( selection_bounds_particle_system_ );
  scene_manager_->destroyQuery( ray_scene_query_ );
  ogre_root_->destroySceneManager( scene_manager_ );
}

void VisualizationPanel::queueRender()
{
  wxCommandEvent event( EVT_RENDER, GetId() );
  wxPostEvent( this, event );
}

void VisualizationPanel::onRender( wxCommandEvent& event )
{
  render_panel_->Refresh();
}

void VisualizationPanel::onViewClicked( wxCommandEvent& event )
{
  ogre_tools::CameraBase* prev_camera = current_camera_;

  switch ( event.GetId() )
  {
  case IDs::FPS:
    {
      current_camera_ = fps_camera_;
    }
    break;

  case IDs::Orbit:
    {
      current_camera_ = orbit_camera_;
    }
    break;
  }

  current_camera_->setFrom( prev_camera );

  render_panel_->setCamera( current_camera_->getOgreCamera() );
}

void VisualizationPanel::onDisplayToggled( wxCommandEvent& event )
{
  int selectionIndex = event.GetSelection();

  VisualizerBase* visualizer = (VisualizerBase*)displays_->GetClientData( selectionIndex );

  if ( displays_->IsChecked( selectionIndex ) )
  {
    visualizer->enable();
  }
  else
  {
    visualizer->disable();
  }

  queueRender();
}

void VisualizationPanel::onDisplaySelected( wxCommandEvent& event )
{
  int selectionIndex = event.GetSelection();
  if ( selectionIndex == wxNOT_FOUND )
  {
    return;
  }

  selected_visualizer_ = (VisualizerBase*)displays_->GetClientData( selectionIndex );

  property_grid_->Freeze();
  property_grid_->Clear();
  selected_visualizer_->fillPropertyGrid( property_grid_ );
  property_grid_->Refresh();
  property_grid_->Thaw();

  render_panel_->Refresh();
}

void VisualizationPanel::onUpdate( wxTimerEvent& event )
{
  long millis = update_stopwatch_.Time();
  float dt = millis / 1000.0f;

  update_stopwatch_.Start();

  V_Visualizer::iterator vis_it = visualizers_.begin();
  V_Visualizer::iterator vis_end = visualizers_.end();
  for ( ; vis_it != vis_end; ++vis_it )
  {
    VisualizerBase* visualizer = *vis_it;

    if ( visualizer->isEnabled() )
    {
      visualizer->update( dt );
    }
  }

  static float render_update_time = 0.0f;
  render_update_time += dt;
  if ( selection_bounds_particle_system_->getNumParticles() > 0 && render_update_time > 0.1 )
  {
    queueRender();
    render_update_time = 0.0f;
  }
}

void VisualizationPanel::onPropertyChanging( wxPropertyGridEvent& event )
{
  if ( selected_visualizer_ )
  {
    wxPGProperty* property = event.GetProperty();

    if ( !property )
    {
      return;
    }

    selected_visualizer_->propertyChanging( event );
  }
}

void VisualizationPanel::onPropertyChanged( wxPropertyGridEvent& event )
{
  if ( selected_visualizer_ )
  {
    wxPGProperty* property = event.GetProperty();

    if ( !property )
    {
      return;
    }

    selected_visualizer_->propertyChanged( event );
  }
}

void VisualizationPanel::addVisualizer( VisualizerBase* visualizer )
{
  visualizers_.push_back( visualizer );

  displays_->Append( wxString::FromAscii( visualizer->getName().c_str() ), visualizer );
  displays_->Check( displays_->GetCount() - 1, visualizer->isEnabled() );

  visualizer->setRenderCallback( boost::bind( &VisualizationPanel::queueRender, this ) );
  visualizer->setLockRenderCallback( boost::bind( &VisualizationPanel::lockRender, this ) );
  visualizer->setUnlockRenderCallback( boost::bind( &VisualizationPanel::unlockRender, this ) );
}

void VisualizationPanel::onRenderWindowMouseEvents( wxMouseEvent& event )
{
  int last_x = mouse_x_;
  int last_y = mouse_y_;

  mouse_x_ = event.GetX();
  mouse_y_ = event.GetY();

  if ( event.LeftDown() )
  {
    if ( event.ControlDown() )
    {
      pick( mouse_x_, mouse_y_ );
    }
    else
    {
      left_mouse_down_ = true;
      middle_mouse_down_ = false;
      right_mouse_down_ = false;
    }
  }
  else if ( event.MiddleDown() )
  {
    left_mouse_down_ = false;
    middle_mouse_down_ = true;
    right_mouse_down_ = false;
  }
  else if ( event.RightDown() )
  {
    left_mouse_down_ = false;
    middle_mouse_down_ = false;
    right_mouse_down_ = true;
  }
  else if ( event.LeftUp() )
  {
    left_mouse_down_ = false;
  }
  else if ( event.MiddleUp() )
  {
    middle_mouse_down_ = false;
  }
  else if ( event.RightUp() )
  {
    right_mouse_down_ = false;
  }
  else if ( event.Dragging() )
  {
    int32_t diff_x = mouse_x_ - last_x;
    int32_t diff_y = mouse_y_ - last_y;

    bool handled = false;
    if ( left_mouse_down_ )
    {
      current_camera_->mouseLeftDrag( diff_x, diff_y );

      handled = true;
    }
    else if ( middle_mouse_down_ )
    {
      current_camera_->mouseMiddleDrag( diff_x, diff_y );

      handled = true;
    }
    else if ( right_mouse_down_ )
    {
      current_camera_->mouseRightDrag( diff_x, diff_y );

      handled = true;
    }

    if ( handled )
    {
      queueRender();
    }
  }

  if ( event.GetWheelRotation() != 0 )
  {
    current_camera_->scrollWheel( event.GetWheelRotation() );

    queueRender();
  }
}

inline void createParticle( Ogre::ParticleSystem* particle_sys, const Ogre::Vector3& position, float size )
{
  Ogre::Particle* p = particle_sys->createParticle();
  if ( p )
  {
    p->setDimensions( size, size );
    p->colour = Ogre::ColourValue( 1.0f, 1.0f, 1.0f, 1.0f );
    p->timeToLive = p->totalTimeToLive = 2.0f;
    p->rotationSpeed = 0.1f;
    p->position = position;
  }
}

void createParticleLine( Ogre::ParticleSystem* particle_sys, const Ogre::Vector3& point1, const Ogre::Vector3& point2, float size, float step )
{
  createParticle( particle_sys, point1, size );

  float distance = point1.distance( point2 );
  int num_particles = distance / step;
  Ogre::Vector3 dir = point2 - point1;
  dir.normalise();
  for ( int i = 1; i < num_particles; ++i )
  {
    Ogre::Vector3 point = point1 + ( dir * ( step * i ) );

    createParticle( particle_sys, point, size );
  }

  createParticle( particle_sys, point2, size );
}

void VisualizationPanel::pick( int mouse_x, int mouse_y )
{
  int width, height;
  render_panel_->GetSize( &width, &height );

  Ogre::Ray mouse_ray = current_camera_->getOgreCamera()->getCameraToViewportRay( (float)mouse_x / (float)width, (float)mouse_y / (float)height );
  ray_scene_query_->setRay( mouse_ray );

  Ogre::RaySceneQueryResult& result = ray_scene_query_->execute();

  Ogre::MovableObject* picked = NULL;
  float closest_distance = 9999999.0f;

  // find the closest object that is also selectable
  Ogre::RaySceneQueryResult::iterator it = result.begin();
  Ogre::RaySceneQueryResult::iterator end = result.end();
  for ( ; it != end; ++it )
  {
    Ogre::RaySceneQueryResultEntry& entry = *it;

    const Ogre::Any& user_any = entry.movable->getUserAny();
    if ( user_any.isEmpty() )
    {
      continue;
    }

    // ugh -- can't just any_cast to VisualizerBase because it's abstract
    /// @todo This is dangerous, should find a better way
    const VisualizerBase* visualizer = reinterpret_cast<const VisualizerBase*>( Ogre::any_cast<void*>( user_any ) );

    if ( visualizer && visualizer->isObjectPickable( entry.movable ) )
    {
      if ( entry.distance < closest_distance )
      {
        closest_distance = entry.distance;
        picked = entry.movable;
      }
    }
  }

  if ( picked )
  {
    Ogre::SceneNode* scene_node = picked->getParentSceneNode();
    if ( scene_node )
    {
      selection_bounds_particle_system_->clear();

      const Ogre::AxisAlignedBox& aabb = scene_node->_getWorldAABB();

      const Ogre::Vector3* corners = aabb.getAllCorners();
      const float size = 0.01f;
      const float step = 0.02f;
      createParticleLine( selection_bounds_particle_system_, corners[0], corners[1], size, step );
      createParticleLine( selection_bounds_particle_system_, corners[1], corners[2], size, step );
      createParticleLine( selection_bounds_particle_system_, corners[2], corners[3], size, step );
      createParticleLine( selection_bounds_particle_system_, corners[3], corners[0], size, step );
      createParticleLine( selection_bounds_particle_system_, corners[4], corners[5], size, step );
      createParticleLine( selection_bounds_particle_system_, corners[5], corners[6], size, step );
      createParticleLine( selection_bounds_particle_system_, corners[6], corners[7], size, step );
      createParticleLine( selection_bounds_particle_system_, corners[7], corners[4], size, step );
      createParticleLine( selection_bounds_particle_system_, corners[1], corners[5], size, step );
      createParticleLine( selection_bounds_particle_system_, corners[2], corners[4], size, step );
      createParticleLine( selection_bounds_particle_system_, corners[3], corners[7], size, step );
      createParticleLine( selection_bounds_particle_system_, corners[0], corners[6], size, step );

      current_camera_->lookAt( aabb.getCenter() );

      queueRender();
    }
  }
}

VisualizerBase* VisualizationPanel::getVisualizer( const std::string& name )
{
  V_Visualizer::iterator vis_it = visualizers_.begin();
  V_Visualizer::iterator vis_end = visualizers_.end();
  for ( ; vis_it != vis_end; ++vis_it )
  {
    VisualizerBase* visualizer = *vis_it;

    if ( visualizer->getName() == name )
    {
      return visualizer;
    }
  }

  return NULL;
}

void VisualizationPanel::setVisualizerEnabled( VisualizerBase* visualizer, bool enabled )
{
  if ( enabled )
  {
    visualizer->enable();
  }
  else
  {
    visualizer->disable();
  }

  int num_displays = displays_->GetCount();
  for ( int i = 0; i < num_displays; ++i )
  {
    if ( displays_->GetClientData(i) == visualizer )
    {
      displays_->Check( i, enabled );
      break;
    }
  }
}

void VisualizationPanel::loadConfig( wxConfigBase* config )
{
  int i = 0;
  while (1)
  {
    wxString type, name;
    type.Printf( wxT("Display%d_Type"), i );
    name.Printf( wxT("Display%d_Name"), i );

    wxString vis_type, vis_name;
    if ( !config->Read( type, &vis_type ) )
    {
      break;
    }

    if ( !config->Read( name, &vis_name ) )
    {
      break;
    }

    createVisualizer( (const char*)vis_type.fn_str(), (const char*)vis_name.fn_str(), false );

    ++i;
  }

  V_Visualizer::iterator vis_it = visualizers_.begin();
  V_Visualizer::iterator vis_end = visualizers_.end();
  for ( ; vis_it != vis_end; ++vis_it )
  {
    VisualizerBase* visualizer = *vis_it;

    wxString old_path = config->GetPath();
    wxString sub_path = old_path + wxT("/") + wxString::FromAscii(visualizer->getName().c_str());

    config->SetPath(sub_path);

    bool enabled;
    config->Read(wxT("Enabled"), &enabled, visualizer->isEnabled());

    setVisualizerEnabled( visualizer, enabled );

    visualizer->loadProperties(config);

    config->SetPath(old_path);
  }
}

void VisualizationPanel::saveConfig( wxConfigBase* config )
{
  int i = 0;
  V_Visualizer::iterator vis_it = visualizers_.begin();
  V_Visualizer::iterator vis_end = visualizers_.end();
  for ( ; vis_it != vis_end; ++vis_it, ++i )
  {
    VisualizerBase* visualizer = *vis_it;

    wxString type, name;
    type.Printf( wxT("Display%d_Type"), i );
    name.Printf( wxT("Display%d_Name"), i );
    config->Write( type, wxString::FromAscii( visualizer->getType() ) );
    config->Write( name, wxString::FromAscii( visualizer->getName().c_str() ) );

    wxString old_path = config->GetPath();

    config->SetPath(old_path + wxT("/") + wxString::FromAscii(visualizer->getName().c_str()));

    config->Write(wxT("Enabled"), visualizer->isEnabled());

    visualizer->saveProperties(config);

    config->SetPath(old_path);
  }
}

bool VisualizationPanel::registerFactory( const std::string& type, VisualizerFactory* factory )
{
  M_Factory::iterator it = factories_.find( type );
  if ( it != factories_.end() )
  {
    return false;
  }

  factories_.insert( std::make_pair( type, factory ) );

  return true;
}

VisualizerBase* VisualizationPanel::createVisualizer( const std::string& type, const std::string& name, bool enabled )
{
  M_Factory::iterator it = factories_.find( type );
  if ( it == factories_.end() )
  {
    return NULL;
  }

  VisualizerBase* current_vis = getVisualizer( name );
  if ( current_vis )
  {
    return NULL;
  }

  VisualizerFactory* factory = it->second;
  VisualizerBase* visualizer = factory->create( scene_manager_, ros_node_, tf_client_, name );
  if ( enabled )
  {
    visualizer->enable( true );
  }
  else
  {
    visualizer->disable( true );
  }

  addVisualizer( visualizer );

  return visualizer;
}

void VisualizationPanel::removeVisualizer( VisualizerBase* visualizer )
{
  int num_displays = displays_->GetCount();
  for ( int i = 0; i < num_displays; ++i )
  {
    if ( displays_->GetClientData( i ) == visualizer )
    {
      displays_->Delete( i );
      break;
    }
  }

  V_Visualizer::iterator it = std::find( visualizers_.begin(), visualizers_.end(), visualizer );
  ROS_ASSERT( it != visualizers_.end() );

  visualizers_.erase( it );

  property_grid_->Clear();
  selected_visualizer_ = NULL;

  delete visualizer;

  queueRender();
}

void VisualizationPanel::removeVisualizer( const std::string& name )
{
  VisualizerBase* visualizer = getVisualizer( name );

  if ( !visualizer )
  {
    return;
  }

  removeVisualizer( visualizer );
}

void VisualizationPanel::onNewDisplay( wxCommandEvent& event )
{
  std::vector<std::string> types;
  M_Factory::iterator factory_it = factories_.begin();
  M_Factory::iterator factory_end = factories_.end();
  for ( ; factory_it != factory_end; ++factory_it )
  {
    types.push_back( factory_it->first );
  }

  NewDisplayDialog dialog( this, types );
  while (1)
  {
    if ( dialog.ShowModal() == wxOK )
    {
      std::string type = dialog.getTypeName();
      std::string name = dialog.getVisualizerName();

      if ( getVisualizer( name ) != NULL )
      {
        wxMessageBox( wxT("A visualizer with that name already exists!"), wxT("Invalid name"), wxICON_ERROR | wxOK, this );
        continue;
      }

      VisualizerBase* visualizer = createVisualizer( type, name, true );
      ROS_ASSERT(visualizer);

      break;
    }
    else
    {
      break;
    }
  }
}

void VisualizationPanel::onDeleteDisplay( wxCommandEvent& event )
{
  int selection = displays_->GetSelection();
  if ( selection == wxNOT_FOUND )
  {
    wxMessageBox( wxT("No display selected"), wxT("No selection"), wxICON_ERROR | wxOK, this );
    return;
  }

  VisualizerBase* visualizer = (VisualizerBase*)displays_->GetClientData(selection);
  removeVisualizer( visualizer );
}

} // namespace ogre_vis
