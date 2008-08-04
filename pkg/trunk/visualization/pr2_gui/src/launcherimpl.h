#ifndef __launcherimpl__
#define __launcherimpl__

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008, Willow Garage Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of Stanford University nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////////////////
/**
@mainpage

@htmlinclude manifest.html

@b Subclass of the Launcher window

<hr>

@section topic ROS topics

Subscribes to (name/type):
- @b "PTZL_image"/std_msgs::Image : Image received from the left PTZ
- @b "PTZR_image"/std_msgs::Image : Image received from the right PTZ
- @b "WristL_image"/std_msgs::Image : Image received from the left wrist camera
- @b "WristR_image"/std_msgs::Image : Image received from the right wrist camera
- @b "PTZL_state"/std_msgs::PTZActuatorState : Receives state from the left PTZ
- @b "PTZR_state"/std_msgs::PTZActuatorState : Receives state from the right PTZ

Publishes to (name/type):
- @b "PTZR_cmd"/std_msgs::std_msgs::PTZActuatorCmd : Moves the right PTZ to the given position
- @b "PTZL_cmd"/std_msgs::std_msgs::PTZActuatorCmd : Moves the left PTZ to the given position


@todo
- Turn me into subclasses and change widgets to the ros panels
**/

/**
@file
Subclass of launcher, which is generated by wxFormBuilder.
*/

//gui stuff
#include "pr2_gui.h"
#include "Vis3d.h"
#include "image_utils/image_codec.h"
#include <wx/dcclient.h>
#include <wx/mstream.h>
#include <wx/image.h>

//ros stuff
#include "ros/node.h"
#include "std_msgs/Image.h"
#include "std_msgs/PTZActuatorCmd.h"
#include "std_msgs/PTZActuatorState.h"
#include "rostools/Log.h"
/** Implementing launcher */
class LauncherImpl : public launcher
{
protected:
	// Handlers for launcher events.
	///Enables and disables the head hokuyo point cloud
	void startStopHeadPtCld( wxCommandEvent& event );
	///Enables and disables the base mounted hokuyo laser scan
	void startStopFloorPtCld( wxCommandEvent& event );
	///Enables and disables the stereo vision point cloud
	void startStopStereoPtCld( wxCommandEvent& event );
	///Enables and disables the 3d model
	void startStopModel( wxCommandEvent& event );
	///Enables and disalbes the UCS
	void startStopUCS( wxCommandEvent& event );
	///Enables and disables the grid
	void startStopGrid( wxCommandEvent& event );
	///Enables and disables the user inserted objects
	void startStopObjects( wxCommandEvent& event );
	///Changes to the selected view
	void viewChanged( wxCommandEvent& event );
	///Changes the head hokuyo scan type
	void HeadLaserChanged( wxCommandEvent& event );
	///Enables or disables the 3d visualization
	void startStop_Visualization( wxCommandEvent& event );
	///Enables or disables the topdown 2d visualization
	void startStop_Topdown( wxCommandEvent& event );
	///Enables or disables the left PTZ camera
	void startStop_PTZL( wxCommandEvent& event );
	///Enables or disables the right PTZ camera
	void startStop_PTZR( wxCommandEvent& event );
	///Enables or disables the left wrist camera
	void startStop_WristL( wxCommandEvent& event );
	///Enables or disables the right wrist camera
	void startStop_WristR( wxCommandEvent& event );
	//Stops the robot from moving more (not implemented)
	void EmergencyStop( wxCommandEvent& event );
	///Draws the left PTZ image
	void PTZLDrawPic( wxCommandEvent& event );
	///Draws the right PTZ image
	void PTZRDrawPic( wxCommandEvent& event );
	///Draws the right wrist image
	void WristRDrawPic( wxCommandEvent& event );
	///Draws the left wrist image
	void WristLDrawPic( wxCommandEvent& event );
	///Writes to the gui's console (instead of the command line)
	void consoleOut(wxString Line);
	///Writes roserr messages to the gui's ros console
	void errorOut();

	///(Callback) Gets an image for the left PTZ
	void incomingPTZLImageConn();
	///(Callback) Gets an image for the right PTZ
	void incomingPTZRImageConn();
	///(Callback) Gets an image for the right wrist
	void incomingWristRImageConn();
	///(Callback) Gets an image for the left wrist
	void incomingWristLImageConn();
	///(Callback) Gets an state for the left PTZ
	void incomingPTZLState();
	///(Callback) Gets an state for the right PTZ
	void incomingPTZRState();

	///(Publisher) Sends a position command to the right PTZ
	void PTZR_ptzChanged(wxScrollEvent& event);
	///(Publisher) Sends a position command to the left PTZ
	void PTZL_ptzChanged(wxScrollEvent& event);
	///Centers the right PTZ on a clicked location
	void PTZR_click( wxMouseEvent& event);
	///Centers the left PTZ on a clicked location
	void PTZL_click( wxMouseEvent& event);

public:
//Variables
	///ROS node
	ros::node *myNode;
	///Message used for viewing ros error messages
	rostools::Log rosErrMsg;
	///Image for the left PTZ
	std_msgs::Image PTZLImage;
	///Image for the right PTZ
	std_msgs::Image PTZRImage;
	///Image for the left wrist
	std_msgs::Image WristLImage;
	///Image for the right wrist
	std_msgs::Image WristRImage;
	///Command message shared between the PTZs
	std_msgs::PTZActuatorCmd ptz_cmd;
	///State of the left PTZ
	std_msgs::PTZActuatorState PTZL_state;
	///State of the right PTZ
	std_msgs::PTZActuatorState PTZR_state;
	///3d visualization object (irrlicht window)
	Vis3d *vis3d_Window;

	///Copy of the left PTZ image data
	uint8_t *PTZLImageData;
	///Copy of the right PTZ image data
	uint8_t *PTZRImageData;
	///Copy of the left wrist image data
	uint8_t *WristLImageData;
	///Copy of the right wrist image data
	uint8_t *WristRImageData;

	///Flag stating a new frame can be displayed for the left PTZ
	bool PTZL_GET_NEW_IMAGE;
	///Flag stating a new frame can be displayed for the right PTZ
	bool PTZR_GET_NEW_IMAGE;
	///Flag stating a new frame can be displayed for the right wrist
	bool WristR_GET_NEW_IMAGE;
	///Flag stating a new frame can be displayed for the left wrist
	bool WristL_GET_NEW_IMAGE;
	///Floats to keep track of PTZs
	float panPTZR, tiltPTZR, panPTZL, tiltPTZL;
	///wx bitmap object necessary for displaying camera/pictures
	wxBitmap *PTZL_bmp;
	///wx bitmap object necessary for displaying camera/pictures
	wxBitmap *PTZR_bmp;
	///wx bitmap object necessary for displaying camera/pictures
	wxBitmap *WristR_bmp;
	///wx bitmap object necessary for displaying camera/pictures
	wxBitmap *WristL_bmp;
	///wx image object necessary for displaying camera/pictures
	wxImage *PTZL_im;
	///wx image object necessary for displaying camera/pictures
	wxImage *PTZR_im;
	///wx image object necessary for displaying camera/pictures
	wxImage *WristR_im;
	///wx image object necessary for displaying camera/pictures
	wxImage *WristL_im;

	ImageCodec<std_msgs::Image> *PTZLCodec;
	ImageCodec<std_msgs::Image> *PTZRCodec;
	ImageCodec<std_msgs::Image> *WristLCodec;
	ImageCodec<std_msgs::Image> *WristRCodec;
	/** Constructor */
	LauncherImpl( wxWindow* parent );

};

#endif // __launcherimpl__
