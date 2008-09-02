/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003  
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * Desc: A ros ptz controller
 * Author: Nathan Koenig
 * Date: 26 Nov 2007
 * SVN: $Id$
 */

#ifndef ROS_PTZ_HH
#define ROS_PTZ_HH

#include <gazebo/Param.hh>
#include <gazebo/Controller.hh>

// ros messages
#include <ros/node.h>

// messages for controlling ptz
#include <axis_cam/PTZActuatorState.h>
#include <axis_cam/PTZActuatorCmd.h>

namespace gazebo
{
  class HingeJoint;
  class PTZIface;

/// @addtogroup gazebo_controller
/// @{
/** \defgroup rosptz ros ptz

  \brief Ros pan-tilt-zoom controller.
  
  This is a controller that controls a pan, tilt, zoom unit 

  \verbatim
  <model:physical name="ptz_model">
    <body:empty name="ptz_body">
      <controller:ros_ptz name="controller-name">
        <interface:ptz name="iface-name"/>
      </controller:ros_ptz>
    </body:empty>
  </model:phyiscal>
  \endverbatim
 
\{
*/

  /// \brief Ros ptz controller.
  /// 
  /// This is a controller for a ros PTZ
  class Ros_PTZ : public Controller
  {
    /// \brief Constructor
    /// \param parent The parent entity, must be a Model or a Sensor
    public: Ros_PTZ(Entity *parent);
  
    /// \brief Destructor
    public: virtual ~Ros_PTZ();
  
    /// \brief Load the controller
    /// \param node XML config node
    protected: virtual void LoadChild(XMLConfigNode *node);

    /// \brief Save the controller.
    /// \stream Output stream
    protected: void SaveChild(std::string &prefix, std::ostream &stream);

    /// \brief Init the controller
    protected: virtual void InitChild();
  
    /// \brief Update the controller
    protected: virtual void UpdateChild();
  
    /// \brief Finalize the controller
    protected: virtual void FiniChild();
              
    /// \brief Reset the controller
    protected: virtual void ResetChild();
  
    /// \brief Put camera data to the iface
    private: void PutPTZData();
  
    /// The camera interface
    private: PTZIface *ptzIface;
  
    /// The parent sensor
    private: Model *myParent;

    /// Pan joint
    private: HingeJoint *panJoint;

    /// Tilt joint
    private: HingeJoint *tiltJoint;

    private: float cmdTilt;
    private: float cmdPan;

    private: Param<double> *motionGainP;
    private: Param<double> *forceP;

    private: Param<std::string> *panJointNameP;
    private: Param<std::string> *tiltJointNameP;
    private: Param<std::string> *commandTopicNameP;
    private: Param<std::string> *stateTopicNameP;

    // pointer to ros node
    private: ros::node *rosnode;
    // ros message
    private: axis_cam::PTZActuatorState PTZStateMessage;
    private: axis_cam::PTZActuatorCmd   PTZControlMessage;

    // receive message
    private: void PTZCommandReceived();

    // topic name
    private: std::string commandTopicName;
    private: std::string stateTopicName;

    // frame transform name, should match link name
    // FIXME: extract link name directly? currently using joint names
    private: std::string panFrameName;
    private: std::string tiltFrameName;

    // A mutex to lock access to fields that are used in message callbacks
    private: ros::thread::mutex lock;

  };
  
  /** /} */
  /// @}

}

#endif

