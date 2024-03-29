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


#include <MoveBase.hh>
#include <std_msgs/BaseVel.h>
#include <std_msgs/PointCloudFloat32.h>
#include <std_msgs/Pose2DFloat32.h>
#include <std_msgs/Polyline2D.h>
#include <std_srvs/StaticMap.h>
#include <set>

namespace ros {
  namespace highlevel_controllers {

    MoveBase::MoveBase()
      : HighlevelController<std_msgs::Planner2DState, std_msgs::Planner2DGoal>("move_base", "state", "goal"),
	tf_(*this, true, 10000000000ULL), // cache for 10 sec, no extrapolation
	controller_(NULL),
	costMap_(NULL),
	ma_(NULL),
	laserMaxRange_(4.0) {
      
      // Initialize global pose. Will be set in control loop based on actual data.
      global_pose_.x = 0;
      global_pose_.y = 0;
      global_pose_.yaw = 0;

      // Initialize odometry
      base_odom_.vel.x = 0;
      base_odom_.vel.y = 0;
      base_odom_.vel.th = 0;

      // Initialize state message parameters that are unsused
      stateMsg.waypoint.x = 0.0;
      stateMsg.waypoint.y = 0.0;
      stateMsg.waypoint.th = 0.0;
      stateMsg.set_waypoints_size(0);
      stateMsg.waypoint_idx = -1;

      // Set up transforms
      double laser_x_offset(0.05);
      //param("laser_x_offset", laser_x_offset, 0.05);
      tf_.setWithEulers("base_laser", "base", laser_x_offset, 0.0, 0.0, 0.0, 0.0, 0.0, 0);

      // Costmap parameters
      double windowLength(1.0);
      unsigned char lethalObstacleThreshold(100);
      unsigned char noInformation(CostMap2D::NO_INFORMATION);
      double maxZ(2.0); 
      double inflationRadius(0.46);
      double robot_radius(.325);
      param("costmap_2d/dynamic_obstacle_window", windowLength, windowLength);
      param("costmap_2d/lethal_obstacle_threshold", lethalObstacleThreshold, lethalObstacleThreshold);
      param("costmap_2d/no_information_value", noInformation, noInformation);
      param("costmap_2d/z_threshold", maxZ, maxZ);
      param("costmap_2d/inflation_radius", inflationRadius, inflationRadius);

      // get map via RPC
      std_srvs::StaticMap::request  req;
      std_srvs::StaticMap::response resp;
      std::cout << "Requesting the map..." << std::endl;
      while(!ros::service::call("static_map", req, resp))
	{
	  std::cout << "Request failed; trying again..." << std::endl;
	  usleep(1000000);
	}

      std::cout << "Received a " << resp.map.width << " X " << 
	resp.map.height << " map at " << 
	resp.map.resolution << "m/pix" << std::endl;

      // We are treating cells with no information as lethal obstacles based on the input data. This is not ideal but
      // our planner and controller do not reason about the no obstacle case
      std::vector<unsigned char> inputData;
      unsigned int numCells = resp.map.width * resp.map.height;
      for(unsigned int i = 0; i < numCells; i++){
	if(resp.map.data[i] == CostMap2D::NO_INFORMATION)
	  inputData.push_back(CostMap2D::LETHAL_OBSTACLE);
	else
	  inputData.push_back((unsigned char) resp.map.data[i]);
      }

      // Now allocate the cost map and its sliding window used by the controller
      costMap_ = new CostMap2D((unsigned int)resp.map.width, (unsigned int)resp.map.height,
                               inputData , resp.map.resolution, 
			       windowLength, lethalObstacleThreshold, maxZ, inflationRadius);

      // Allocate Velocity Controller
      double mapSize(10.0);
      double pathDistanceBias(0.4);
      double goalDistanceBias(0.6);
      double accLimit_x(0.15);
      double accLimit_y(1.0);
      double accLimit_th(1.0);
      const double SIM_TIME = 2.0;
      const unsigned int SIM_STEPS = 30;
      const unsigned int SAMPLES_PER_DIM = 25;
      const double MAX_OCC_DIST = 1.0;
      const double DFAST_SCALE = 0;
      const double OCCDIST_SCALE = 0;
      param("trajectory_rollout/map_size", mapSize, 10.0);
      param("trajectory_rollout/path_distance_bias", pathDistanceBias, 0.4);
      param("trajectory_rollout/goal_distance_bias", goalDistanceBias, 0.6);
      param("trajectory_rollout/acc_limit_x", accLimit_x, 0.15);
      param("trajectory_rollout/acc_limit_y", accLimit_y, 0.15);
      param("trajectory_rollout/acc_limit_th", accLimit_th, 1.0);

      ma_ = new CostMapAccessor(*costMap_, mapSize, 0.0, 0.0);
      controller_ = new ros::highlevel_controllers::TrajectoryRolloutController(&tf_, *ma_,
										SIM_TIME,
										SIM_STEPS,
										SAMPLES_PER_DIM,
										robot_radius,
										robot_radius,
										MAX_OCC_DIST,
										pathDistanceBias,
										goalDistanceBias,
										DFAST_SCALE,
										OCCDIST_SCALE,
										accLimit_x,
										accLimit_y,
										accLimit_th);

      // Advertize messages to publish cost map updates
      advertise<std_msgs::Polyline2D>("raw_obstacles", QUEUE_MAX());
      advertise<std_msgs::Polyline2D>("inflated_obstacles", QUEUE_MAX());

      // Advertize message to publish the global plan
      advertise<std_msgs::Polyline2D>("gui_path", QUEUE_MAX());

      // Advertize message to publish local plan
      advertise<std_msgs::Polyline2D>("local_path", QUEUE_MAX());
      
      // Advertize message to publish robot footprint
      advertise<std_msgs::Polyline2D>("robot_footprint", QUEUE_MAX());

      // Advertize message to publish velocity cmds
      advertise<std_msgs::BaseVel>("cmd_vel", QUEUE_MAX());

      // Subscribe to laser scan messages
      subscribe("scan", laserScanMsg_, &MoveBase::laserScanCallback, QUEUE_MAX());

      // Subscribe to point cloud messages
      subscribe("cloud", pointCloudMsg_, &MoveBase::pointCloudCallback, QUEUE_MAX());

      // Subscribe to odometry messages
      subscribe("odom", odomMsg_, &MoveBase::odomCallback, QUEUE_MAX());

      // Now initialize
      initialize();
    }

    MoveBase::~MoveBase(){

      if(controller_ != NULL)
	delete controller_;

      if(ma_ != NULL)
	delete ma_;

      if(costMap_ != NULL)
	delete costMap_;
    }

    void MoveBase::updateGlobalPose(){
      libTF::TFPose2D robotPose;
      robotPose.x = 0;
      robotPose.y = 0;
      robotPose.yaw = 0;
      robotPose.frame = "base";
      robotPose.time = 0; 

      try{
	global_pose_ = this->tf_.transformPose2D("map", robotPose);
      }
      catch(libTF::TransformReference::LookupException& ex){
	std::cout << "No Transform available Error\n";
      }
      catch(libTF::TransformReference::ConnectivityException& ex){
	std::cout << "Connectivity Error\n";
      }
      catch(libTF::TransformReference::ExtrapolateException& ex){
	std::cout << "Extrapolation Error\n";
      }

      // Update the cost map window
      ma_->updateForRobotPosition(global_pose_.x, global_pose_.y);
    }


    void MoveBase::updateGoalMsg(){
      lock();
      stateMsg.goal.x = goalMsg.goal.x;
      stateMsg.goal.y = goalMsg.goal.y;
      stateMsg.goal.th = goalMsg.goal.th;
      unlock();

      printf("Received new goal (x=%f, y=%f, th=%f)\n", goalMsg.goal.x, goalMsg.goal.y, goalMsg.goal.th);
    }

    void MoveBase::updateStateMsg(){
      // Get the current robot pose in the map frame
      updateGlobalPose();

      // Assign state data 
      stateMsg.pos.x = global_pose_.x;
      stateMsg.pos.y = global_pose_.y;
      stateMsg.pos.th = global_pose_.yaw;
    }

    /**
     * The laserScanMsg_ member will have been updated. It is locked already too.
     */
    void MoveBase::laserScanCallback(){

      // Assemble a point cloud, in the laser's frame
      std_msgs::PointCloudFloat32 local_cloud;
      projector_.projectLaser(laserScanMsg_, local_cloud, laserMaxRange_);
    
      // Convert to a point cloud in the map frame
      std_msgs::PointCloudFloat32 global_cloud;

      try
	{
	  global_cloud = this->tf_.transformPointCloud("map", local_cloud);
	}
      catch(libTF::TransformReference::LookupException& ex)
	{
	  puts("no global->local Tx yet");
	  printf("%s\n", ex.what());
	  return;
	}
      catch(libTF::TransformReference::ConnectivityException& ex)
	{
	  puts("no global->local Tx yet");
	  printf("%s\n", ex.what());
	  return;
	}
      catch(libTF::TransformReference::ExtrapolateException& ex)
	{
	  puts("Extrapolation exception");
	}

      /*
	std::cout << "Laser scan received (Laser Scan:" << laserScanMsg_.get_ranges_size() << 
	", localCloud:" << local_cloud.get_pts_size() << 
	", globalCloud:" << global_cloud.get_pts_size() << ")\n";
      */

      // Update the cost map
      const double ts = laserScanMsg_.header.stamp.to_double();
      std::vector<unsigned int> insertions, deletions;

      // Surround with a lock since it can interact with main planning and execution thread
      lock();
      costMap_->updateDynamicObstacles(ts, global_pose_.x, global_pose_.y, global_cloud, insertions, deletions);
      handleMapUpdates(insertions, deletions);
      publishLocalCostMap();
      unlock();
    }


    /**
     * Point clouds are produced in the map frame so no transform is required. We simply use point clouds if produced
     */
    void MoveBase::pointCloudCallback(){
      // Update the cost map
      const double ts = pointCloudMsg_.header.stamp.to_double();
      std::vector<unsigned int> insertions, deletions;

      // Surround with a lock since it can interact with main planning and execution thread
      lock();
      costMap_->updateDynamicObstacles(ts, pointCloudMsg_, insertions, deletions);
      handleMapUpdates(insertions, deletions);
      publishLocalCostMap();
      unlock();
    }

    /**
     * The odomMsg_ will be updates and we will do the transform to update the odom in the base frame
     */
    void MoveBase::odomCallback(){
      lock();
      try
	{
	  libTF::TFVector v_in, v_out;
	  v_in.x = odomMsg_.vel.x;
	  v_in.y = odomMsg_.vel.y;
	  v_in.z = odomMsg_.vel.th;	  
	  v_in.time = 0; // Gets the latest
	  v_in.frame = "odom";
	  v_out = tf_.transformVector("base", v_in);
	  base_odom_.vel.x = v_in.x;
	  base_odom_.vel.y = v_in.y;
	  base_odom_.vel.th = v_in.z;
	}
      catch(libTF::TransformReference::LookupException& ex)
	{
	  puts("no odom->base Tx yet");
	  printf("%s\n", ex.what());
	}
      catch(libTF::TransformReference::ConnectivityException& ex)
	{
	  puts("no odom->base Tx yet");
	  printf("%s\n", ex.what());
	}
      catch(libTF::TransformReference::ExtrapolateException& ex)
	{
	  puts("Extrapolation exception");
	}

      unlock();
    }

    void MoveBase::updatePlan(const std::list<std_msgs::Pose2DFloat32>& newPlan){
      plan_.clear();
      plan_ = newPlan;
    }
    
    bool MoveBase::inCollision() const {
      for(std::list<std_msgs::Pose2DFloat32>::const_iterator it = plan_.begin(); it != plan_.end(); ++it){
	const std_msgs::Pose2DFloat32& w = *it;
	unsigned int ind = costMap_->WC_IND(w.x, w.y);
	if((*costMap_)[ind] >= CostMap2D::LETHAL_OBSTACLE){
	  printf("path in collision at <%f, %f>\n", w.x, w.y);
	  return true;
	}
      }

      return false;
    }

    void MoveBase::publishFootprint(double x, double y, double th){
      std::vector<std_msgs::Point2DFloat32> footprint = controller_->drawFootprint(x, y, th);
      std_msgs::Polyline2D footprint_msg;
      footprint_msg.set_points_size(footprint.size());
      footprint_msg.color.r = 1.0;
      footprint_msg.color.g = 0;
      footprint_msg.color.b = 0;
      footprint_msg.color.a = 0;
      for(unsigned int i = 0; i < footprint.size(); ++i){
        footprint_msg.points[i].x = footprint[i].x;
        footprint_msg.points[i].y = footprint[i].y;
      }
      publish("robot_footprint", footprint_msg);
    }

    void MoveBase::publishPath(bool isGlobal, const std::list<std_msgs::Pose2DFloat32>& path) {
      std_msgs::Polyline2D guiPathMsg;
      guiPathMsg.set_points_size(path.size());
 
      unsigned int i = 0;
      for(std::list<std_msgs::Pose2DFloat32>::const_iterator it = path.begin(); it != path.end(); ++it){
	const std_msgs::Pose2DFloat32& w = *it;
	guiPathMsg.points[i].x = w.x;
	guiPathMsg.points[i].y = w.y;
	i++;
      }

      if(isGlobal){
	guiPathMsg.color.r = 0;
	guiPathMsg.color.g = 1.0;
	guiPathMsg.color.b = 0;
	guiPathMsg.color.a = 0;
	publish("gui_path", guiPathMsg);
      }
      else {
	guiPathMsg.color.r = 0;
	guiPathMsg.color.g = 0;
	guiPathMsg.color.b = 1.0;
	guiPathMsg.color.a = 0;
	publish("local_path", guiPathMsg);
      }

    }

    bool MoveBase::goalReached(){
      // Publish the global plan
      publishPath(true, plan_);

      // If the plan has been executed (i.e. empty) and we are within a required distance of the target orientation,
      // and we have stopped the robot, then we are done
      if(plan_.empty() && 
	 fabs(global_pose_.yaw - stateMsg.goal.th) < 10){

	printf("Goal achieved at: (%f, %f, %f) for (%f, %f, %f)\n",
	       global_pose_.x, global_pose_.y, global_pose_.yaw,
	       stateMsg.goal.x, stateMsg.goal.y, stateMsg.goal.th);

	// The last act will issue stop command
	stopRobot();

	return true;
      }

      // If we have reached the end of the path then clear the plan
      if(!plan_.empty() &&
	 withinDistance(global_pose_.x, global_pose_.y, global_pose_.yaw,
			stateMsg.goal.x, stateMsg.goal.y, global_pose_.yaw)){
	printf("Last waypoint achieved at: (%f, %f, %f) for (%f, %f, %f)\n",
	       global_pose_.x, global_pose_.y, global_pose_.yaw,
	       stateMsg.goal.x, stateMsg.goal.y, stateMsg.goal.th);

	plan_.clear();
      }

      return false;
    }

    bool MoveBase::dispatchCommands(){
      bool planOk = true; //!inCollision(); // Return value to trigger replanning or not
      std_msgs::BaseVel cmdVel; // Commanded velocities      

      // if we have achieved all our waypoints but have yet to achieve the goal, then we know that we wish to accomplish our desired
      // orientation
      if(plan_.empty()){
	std::cout << "Moving to desired goal orientation\n";
	cmdVel.vx = 0;
	cmdVel.vy = 0;
	cmdVel.vw =  stateMsg.goal.th - global_pose_.yaw;
      }
      else {
	// Refine the plan to reflect progress made. If no part of the plan is in the local cost window then
	// the global plan has failed since we are nowhere near the plan. We also prune parts of the plan that are behind us as we go. We determine this
	// by assuming that we start within a certain distance from the beginning of the plan and we can stay within a maximum error of the planned
	// path
	std::list<std_msgs::Pose2DFloat32>::iterator it = plan_.begin();
	while(it != plan_.end()){
	  const std_msgs::Pose2DFloat32& w = *it;
	  // Fixed error bound of 2 meters for now. Can reduce to a portion of the map size or based on the resolution
	  if(fabs(global_pose_.x - w.x) < 2 && fabs(global_pose_.y - w.y) < 2){
	    printf("Nearest waypoint to <%f, %f> is <%f, %f>\n", global_pose_.x, global_pose_.y, w.x, w.y);
	    break;
	  }

	  it = plan_.erase(it);
	}

	// The plan is bogus if it is empty
	planOk = planOk && !plan_.empty();

	// Set current velocities from odometry
	std_msgs::BaseVel currentVel;
	currentVel.vx = base_odom_.vel.x;
	currentVel.vy = base_odom_.vel.y;
	currentVel.vw = base_odom_.vel.th;

	// Create a window onto the global cost map for the velocity controller
	std::list<std_msgs::Pose2DFloat32> localPlan; // Capture local plan for display
	planOk = planOk && controller_->computeVelocityCommands(plan_, global_pose_, currentVel, cmdVel, localPlan);

	if(!planOk){
	  // Zero out the velocities
	  cmdVel.vx = 0;
	  cmdVel.vy = 0;
	  cmdVel.vw = 0;
	  std::cout << "Local planning has failed :-(\n";
	}

	publishPath(false, localPlan);
	publishPath(true, plan_);
	std_msgs::Pose2DFloat32& pt = localPlan.front();
	publishFootprint(pt.x, pt.y, pt.th);
      }

      printf("Dispatching velocity vector: (%f, %f, %f)\n", cmdVel.vx, cmdVel.vy, cmdVel.vw);

      publish("cmd_vel", cmdVel);

      return planOk;
    }

    /**
     * @todo Make based on loaded tolerances
     */
    bool MoveBase::withinDistance(double x1, double y1, double th1, double x2, double y2, double th2) const {
      if(fabs(x1 - x2) < 2 * getCostMap().getResolution() &&
	 fabs(y1 - y2) < 2 * getCostMap().getResolution() &&
	 fabs(th1- th2) < 10)
	return true;

      return false;
    }

    /**
     * @brief Utility to output local obstacles. Make the local cost map accessor. It is very cheap :-) Then
     * render the obstacles.
     */
    void MoveBase::publishLocalCostMap() {

      // Publish obstacle data for each obstacle cell
      std::vector< std::pair<double, double> > rawObstacles, inflatedObstacles;
      double origin_x, origin_y;
      ma_->getOriginInWorldCoordinates(origin_x, origin_y);
      for(unsigned int i = 0; i<ma_->getWidth(); i++)
	for(unsigned int j = 0; j<ma_->getHeight();j++){
	  double wx, wy;
	  wx = i * ma_->getResolution() + origin_x;
	  wy = j * ma_->getResolution() + origin_y;
	  std::pair<double, double> p(wx, wy);

	  if(ma_->isObstacle(i, j))
	    rawObstacles.push_back(p);
	  else if(ma_->isInflatedObstacle(i, j))
	    inflatedObstacles.push_back(p);
	}


      // First publish raw obstacles in red
      std_msgs::Polyline2D pointCloudMsg;
      unsigned int pointCount = rawObstacles.size();
      pointCloudMsg.set_points_size(pointCount);
      pointCloudMsg.color.a = 0.0;
      pointCloudMsg.color.r = 1.0;
      pointCloudMsg.color.b = 0.0;
      pointCloudMsg.color.g = 0.0;

      for(unsigned int i=0;i<pointCount;i++){
	pointCloudMsg.points[i].x = rawObstacles[i].first;
	pointCloudMsg.points[i].y = rawObstacles[i].second;
      }

      publish("raw_obstacles", pointCloudMsg);

      // Now do inflated obstacles in blue
      pointCount = inflatedObstacles.size();
      pointCloudMsg.set_points_size(pointCount);
      pointCloudMsg.color.a = 0.0;
      pointCloudMsg.color.r = 0.0;
      pointCloudMsg.color.b = 1.0;
      pointCloudMsg.color.g = 0.0;

      for(unsigned int i=0;i<pointCount;i++){
	pointCloudMsg.points[i].x = inflatedObstacles[i].first;
	pointCloudMsg.points[i].y = inflatedObstacles[i].second;
      }

      publish("inflated_obstacles", pointCloudMsg);
    }

    void MoveBase::stopRobot(){
      std::cout << "Stopping the robot now!\n";
      std_msgs::BaseVel cmdVel; // Commanded velocities
      cmdVel.vx = 0.0;
      cmdVel.vy = 0.0;
      cmdVel.vw = 0.0;
      publish("cmd_vel", cmdVel);
    }
  }
}
