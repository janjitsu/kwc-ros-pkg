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
 @mainpage
   Desc: Force Feed Back Ground Truth
   Author: Sachin Chitta and John Hsu
   Date: 1 June 2008
   SVN info: $Id$
 @htmlinclude manifest.html
 @b F3D plugin broadcasts forces acting on the body specified by name.
 */

#include <gazebo/Global.hh>
#include <gazebo/XMLConfig.hh>
#include <gazebo/Model.hh>
#include <gazebo/HingeJoint.hh>
#include <gazebo/Body.hh>
#include <gazebo/SliderJoint.hh>
#include <gazebo/Simulator.hh>
#include <gazebo/gazebo.h>
#include <gazebo/GazeboError.hh>
#include <gazebo/ControllerFactory.hh>
#include <gazebo_plugin/F3D.hh>

using namespace gazebo;

GZ_REGISTER_DYNAMIC_CONTROLLER("F3D", F3D);


////////////////////////////////////////////////////////////////////////////////
// Constructor
F3D::F3D(Entity *parent )
   : Controller(parent)
{
   this->myParent = dynamic_cast<Model*>(this->parent);

   if (!this->myParent)
      gzthrow("F3D controller requires a Model as its parent");

  rosnode = ros::g_node; // comes from where?
  int argc = 0;
  char** argv = NULL;
  if (rosnode == NULL)
  {
    // this only works for a single camera.
    ros::init(argc,argv);
    rosnode = new ros::node("ros_gazebo",ros::node::DONT_HANDLE_SIGINT);
    printf("-------------------- starting node in F3D \n");
  }
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
F3D::~F3D()
{
}

////////////////////////////////////////////////////////////////////////////////
// Load the controller
void F3D::LoadChild(XMLConfigNode *node)
{
  std::string bodyName = node->GetString("bodyName", "", 1);
  this->myBody = dynamic_cast<Body*>(this->myParent->GetBody(bodyName));
//  this->myBody = dynamic_cast<Body*>(this->myParent->GetBody(bodyName));

  this->topicName = node->GetString("topicName", "", 1);
  this->frameName = node->GetString("frameName", "", 1);

  std::cout << "==== topic name for F3D ======== " << this->topicName << std::endl;
  rosnode->advertise<std_msgs::Vector3Stamped>(this->topicName,10);
}

////////////////////////////////////////////////////////////////////////////////
// Initialize the controller
void F3D::InitChild()
{
}

////////////////////////////////////////////////////////////////////////////////
// Update the controller
void F3D::UpdateChild()
{
  Vector3 torque;
  Vector3 force;

  // get force on body
  force = this->myBody->GetForce();

  // get torque on body
  torque = this->myBody->GetTorque();

  //FIXME:  toque not published, need new message type of new topic name for torque
  //FIXME:  use name space of body id (link name)?
  this->lock.lock();
  // copy data into pose message
  this->vector3Msg.header.frame_id = this->frameName;
  this->vector3Msg.header.stamp.sec = (unsigned long)floor(Simulator::Instance()->GetSimTime());
  this->vector3Msg.header.stamp.nsec = (unsigned long)floor(  1e9 * (  Simulator::Instance()->GetSimTime() - this->vector3Msg.header.stamp.sec) );

  this->vector3Msg.vector.x    = force.x;
  this->vector3Msg.vector.y    = force.y;
  this->vector3Msg.vector.z    = force.z;

  //std::cout << "F3D: " << this->topicName
  //          << "  f: " << force
  //          << "  t: " << torque << std::endl;
  // publish to ros
  rosnode->publish(this->topicName,this->vector3Msg);
  this->lock.unlock();

}

////////////////////////////////////////////////////////////////////////////////
// Finalize the controller
void F3D::FiniChild()
{
  rosnode->unadvertise(this->topicName);
}
