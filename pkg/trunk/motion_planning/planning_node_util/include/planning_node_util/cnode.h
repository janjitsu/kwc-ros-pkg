/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
* 
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
* 
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/** \Author Ioan Sucan */

/**

@mainpage

@htmlinclude ../../manifest.html

@b NodeCollisionModel is a class which in addition to being aware of a robot model,
is also aware of a collision space.

<hr>

@section topic ROS topics

Subscribes to (name/type):
- @b world_3d_map/PointCloudFloat32 : point cloud with data describing the 3D environment

Additional subscriptions due to inheritance from NodeRobotModel:
- @b localizedpose/RobotBase2DOdom : localized position of the robot base

Publishes to (name/type):
- None

<hr>

@section services ROS services

Uses (name/type):
- None

Provides (name/type):
- None

<hr>

@section parameters ROS parameters
- None

**/

#include <planning_node_util/knode.h>
#include <std_msgs/PointCloudFloat32.h>
#include <collision_space/environmentODE.h>
#include <collision_space/environmentOctree.h>

namespace planning_node_util
{
    
    class NodeCollisionModel : public NodeRobotModel
    {

    public:
	
        NodeCollisionModel(ros::node *node, const std::string &robot_model,
			   collision_space::EnvironmentModel *collisionSpace = NULL) : NodeRobotModel(node, robot_model)
	{
	    if (collisionSpace)
		m_collisionSpace = collisionSpace;
	    else
		m_collisionSpace = new collision_space::EnvironmentModelODE();
	    m_collisionSpace->setSelfCollision(true);
	    m_collisionSpace->addPlane(0.0, 0.0, 1.0, -0.01);
	    
	    m_sphereSize = 0.03;
	    
	    m_node->subscribe("world_3d_map", m_worldCloud, &NodeCollisionModel::worldMapCallback, this, 1);
	}

	virtual ~NodeCollisionModel(void)
	{
	    if (m_collisionSpace)
	    {
		delete m_collisionSpace;
		m_kmodel = NULL;
	    }
	}
	
	virtual void setRobotDescription(robot_desc::URDF *file)
	{
	    NodeRobotModel::setRobotDescription(file);
	    if (m_kmodel)
	    {
		std::vector<std::string> links;
		robot_desc::URDF::Group *g = file->getGroup("collision_check");
		if (g && g->hasFlag("collision"))
		    links = g->linkNames;
		m_collisionSpace->lock();
		unsigned int cid = m_collisionSpace->addRobotModel(m_kmodel, links);
		m_collisionSpace->unlock();
		addSelfCollisionGroups(cid, file);
	    }	    
	}
	
    	virtual void defaultPosition(void)
	{
	    NodeRobotModel::defaultPosition();
	    if (m_collisionSpace && m_collisionSpace->getModelCount() == 1)
		m_collisionSpace->updateRobotModel(0);
	}
	
    protected:
	
	std_msgs::PointCloudFloat32           m_worldCloud;
	collision_space::EnvironmentModel    *m_collisionSpace;
	double                                m_sphereSize;
	
	void addSelfCollisionGroups(unsigned int cid, robot_desc::URDF *model)
	{
	    std::vector<robot_desc::URDF::Group*> groups;
	    model->getGroups(groups);
	    
	    m_collisionSpace->lock();
	    for (unsigned int i = 0 ; i < groups.size() ; ++i)
		if (groups[i]->hasFlag("self_collision"))
		    m_collisionSpace->addSelfCollisionGroup(cid, groups[i]->linkNames);
	    m_collisionSpace->unlock();
	}
	
	void worldMapCallback(void)
	{
	    unsigned int n = m_worldCloud.get_pts_size();
	    printf("received %u points (world map)\n", n);
	    
	    beforeWorldUpdate();
	    
	    ros::Time startTime = ros::Time::now();
	    double *data = new double[3 * n];	
	    for (unsigned int i = 0 ; i < n ; ++i)
	    {
		unsigned int i3 = i * 3;	    
		data[i3    ] = m_worldCloud.pts[i].x;
		data[i3 + 1] = m_worldCloud.pts[i].y;
		data[i3 + 2] = m_worldCloud.pts[i].z;
	    }
	    
	    m_collisionSpace->lock();
	    m_collisionSpace->clearObstacles();
	    m_collisionSpace->addPointCloud(n, data, m_sphereSize);
	    m_collisionSpace->unlock();
	    
	    delete[] data;
	    
	    double tupd = (ros::Time::now() - startTime).to_double();
	    printf("Updated world model in %f seconds\n", tupd);
	    
	    afterWorldUpdate();
	}
	
	virtual void beforeWorldUpdate(void)
	{
	}
	
	virtual void afterWorldUpdate(void)
	{
	}	

    };
     
}
