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

/**

@mainpage

@htmlinclude manifest.html

@b KinematicPlanning is a node capable of planning kinematic paths for
a set of robot models.

<hr>

@section usage Usage
@verbatim
$ kinematic_planning [standard ROS args]
@endverbatim

@par Example

@verbatim
$ kinematic_planning
@endverbatim

<hr>

@section topic ROS topics

Subscribes to (name/type):
- @b world_3d_map/PointCloudFloat32 : point cloud with data describing the 3D environment

Publishes to (name/type):
- None

<hr>

@section services ROS services

Uses (name/type):
- None

Provides (name/type):
- @b "plan_kinematic_path"/KinematicMotionPlan : given a robot model, starting ang goal states, this service computes a collision free path

<hr>

@section parameters ROS parameters
- None

**/

#include <ros/node.h>
#include <std_msgs/PointCloudFloat32.h>
#include <std_msgs/KinematicPath.h>
#include <std_srvs/KinematicMotionPlan.h>

#include <kinematic_planning/definitions.h>
#include <collision_space/environmentODE.h>
#include <ompl/extension/samplingbased/kinematic/extension/rrt/RRT.h>

using namespace std_msgs;
using namespace std_srvs;

class KinematicPlanning : public ros::node
{
public:

    KinematicPlanning(void) : ros::node("kinematic_planning")
    {
	//	m_si  = new ompl::SpaceInformationKinematic();
	//	m_rrt = new ompl::RRT(m_si);
	
	advertise_service("plan_kinematic_path", &KinematicPlanning::plan);
	subscribe("world_3d_map", cloud, &KinematicPlanning::pointCloudCallback);
    }
    
    void pointCloudCallback(void)
    {
	unsigned int n = cloud.get_pts_size();
	printf("received %u points\n", n);

	double *data = new double[3 * n];	
	for (unsigned int i = 0 ; i < n ; ++i)
	{
	    unsigned int i3 = i * 3;	    
	    data[i3    ] = cloud.pts[i].x;
	    data[i3 + 1] = cloud.pts[i].y;
	    data[i3 + 2] = cloud.pts[i].z;
	}
	delete[] data;
	
    }
    
    bool plan(KinematicMotionPlan::request &req, KinematicMotionPlan::response &res)
    {
	//	const int dim = req.start_state.vals_size;
	//	ompl::SpaceInformationKinematic::GoalStateKinematic_t goal = new ompl::SpaceInformationKinematic::GoalStateKinematic(m_si);
	//	m_si->setGoal(goal);
	
	/*
	std::vector<double*> path;
	double start[dim];
	double goal[dim];
	
	for (int i = 0 ; i < dim ; ++i)
	    start[i] = req.start_state.vals[i];
	for (int i = 0 ; i < dim ; ++i)
	    goal[i] = req.goal_state.vals[i];
	
	/////////////////
	
	res.path.set_states_size(path.size());
	for (unsigned int i = 0 ; i < path.size() ; ++i)
	{
	    res.path.states[i].set_vals_size(dim);
	    for (int j = 0 ; j < dim ; ++j)
		res.path.states[i].vals[j] = path[i][j];
	    delete[] path[i];
	}
	*/
	//	m_si->clearGoal();	
	
	return true;	
    }
    
private:
    
    PointCloudFloat32 cloud;

    struct Model
    {
	ompl::SpaceInformationKinematic_t si;
	ompl::MotionPlanner_t             mp;
	// collision space
    };
    
    std::vector<Model> models; 
    
};


int main(int argc, char **argv)
{  
    ros::init(argc, argv);
    
    KinematicPlanning planner;
    planner.spin();
    planner.shutdown();
    
    return 0;    
}
