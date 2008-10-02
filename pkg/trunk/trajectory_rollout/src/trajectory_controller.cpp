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
#include <trajectory_rollout/trajectory_controller.h>

namespace ublas = boost::numeric::ublas;
using namespace std;
using namespace std_msgs;

TrajectoryController::TrajectoryController(MapGrid& mg, double sim_time, int num_steps, int samples_per_dim,
    double robot_front_radius, double robot_side_radius, double max_occ_dist, double pdist_scale, double gdist_scale,
    double dfast_scale, double occdist_scale, double acc_lim_x, double acc_lim_y, double acc_lim_theta, rosTFClient* tf,
    const costmap_2d::ObstacleMapAccessor& ma)
  : map_(mg), num_steps_(num_steps), sim_time_(sim_time), samples_per_dim_(samples_per_dim), robot_front_radius_(robot_front_radius),
  robot_side_radius_(robot_side_radius), max_occ_dist_(max_occ_dist), 
  pdist_scale_(pdist_scale), gdist_scale_(gdist_scale), dfast_scale_(dfast_scale), occdist_scale_(occdist_scale), 
  acc_lim_x_(acc_lim_x), acc_lim_y_(acc_lim_y), acc_lim_theta_(acc_lim_theta), tf_(tf), ma_(ma)
{
  //regularly sample the forward velocity space... sample rotational vel space... sample backward vel space
  num_trajectories_ = samples_per_dim * samples_per_dim * samples_per_dim + samples_per_dim + samples_per_dim;
  int total_pts = num_trajectories_ * num_steps_;

  //even though we only have x,y we need to multiply by a 4x4 matrix
  trajectory_pts_ = ublas::zero_matrix<double>(4, total_pts);

  //storage for our theta values
  trajectory_theta_ = ublas::zero_matrix<double>(1, total_pts);

  //the robot is not stuck to begin with
  stuck_left = false;
  stuck_right = false;
}

//update what map cells are considered path based on the global_plan
void TrajectoryController::setPathCells(){
  map_.resetPathDist();
  int local_goal_x = -1;
  int local_goal_y = -1;
  bool started_path = false;
  queue<MapCell*> path_dist_queue;
  queue<MapCell*> goal_dist_queue;
  for(unsigned int i = 0; i < global_plan_.size(); ++i){
    double g_x = global_plan_[i].x;
    double g_y = global_plan_[i].y;
    unsigned int map_x, map_y;
    if(ma_.WC_MC(g_x, g_y, map_x, map_y)){
      printf("%f, %f => %d, %d\n", g_x, g_y, map_x, map_y);
      MapCell& current = map_(map_x, map_y);
      current.path_dist = 0.0;
      current.path_mark = true;
      path_dist_queue.push(&current);
      local_goal_x = map_x;
      local_goal_y = map_y;
      started_path = true;
      //printf("Valid Cell: (%.2f, %.2f) - (%d, %d), ", global_plan_[i].x, global_plan_[i].y, map_x, map_y);
    }
    else{
      if(started_path)
        break;
    }
  }
  //printf("\n");

  if(local_goal_x >= 0 && local_goal_y >= 0){
    MapCell& current = map_(local_goal_x, local_goal_y);
    current.goal_dist = 0.0;
    current.goal_mark = true;
    goal_dist_queue.push(&current);
  }
  //compute our distances
  computePathDistance(path_dist_queue);
  computeGoalDistance(goal_dist_queue);
}

void TrajectoryController::computePathDistance(queue<MapCell*>& dist_queue){
  MapCell* current_cell;
  MapCell* check_cell;
  unsigned int last_col = map_.size_x_ - 1;
  unsigned int last_row = map_.size_y_ - 1;
  while(!dist_queue.empty()){
    current_cell = dist_queue.front();
    check_cell = current_cell;
    dist_queue.pop();

    if(current_cell->cx > 0){
      check_cell = current_cell - 1;
      if(!check_cell->path_mark){
        updatePathCell(current_cell, check_cell, dist_queue);
      }
    }

    if(current_cell->cx < last_col){
      check_cell = current_cell + 1;
      if(!check_cell->path_mark){
        updatePathCell(current_cell, check_cell, dist_queue);
      }
    }

    if(current_cell->cy > 0){
      check_cell = current_cell - map_.size_x_;
      if(!check_cell->path_mark){
        updatePathCell(current_cell, check_cell, dist_queue);
      }
    }

    if(current_cell->cy < last_row){
      check_cell = current_cell + map_.size_x_;
      if(!check_cell->path_mark){
        updatePathCell(current_cell, check_cell, dist_queue);
      }
    }
  }
}

void TrajectoryController::computeGoalDistance(queue<MapCell*>& dist_queue){
  MapCell* current_cell;
  MapCell* check_cell;
  unsigned int last_col = map_.size_x_ - 1;
  unsigned int last_row = map_.size_y_ - 1;
  while(!dist_queue.empty()){
    current_cell = dist_queue.front();
    current_cell->goal_mark = true;
    check_cell = current_cell;
    dist_queue.pop();

    if(current_cell->cx > 0){
      check_cell = current_cell - 1;
      if(!check_cell->goal_mark){
        updateGoalCell(current_cell, check_cell, dist_queue);
      }
    }

    if(current_cell->cx < last_col){
      check_cell = current_cell + 1;
      if(!check_cell->goal_mark){
        updateGoalCell(current_cell, check_cell, dist_queue);
      }
    }

    if(current_cell->cy > 0){
      check_cell = current_cell - map_.size_x_;
      if(!check_cell->goal_mark){
        updateGoalCell(current_cell, check_cell, dist_queue);
      }
    }

    if(current_cell->cy < last_row){
      check_cell = current_cell + map_.size_x_;
      if(!check_cell->goal_mark){
        updateGoalCell(current_cell, check_cell, dist_queue);
      }
    }
  }
}

//create a trajectory given the current pose of the robot and selected velocities
Trajectory TrajectoryController::generateTrajectory(int t_num, double x, double y, double theta, double vx, double vy, 
    double vtheta, double vx_samp, double vy_samp, double vtheta_samp, double acc_x, double acc_y, double acc_theta){
  double x_i = x;
  double y_i = y;
  double theta_i = theta;
  double vx_i = vx;
  double vy_i = vy;
  double vtheta_i = vtheta;
  double dt = sim_time_ / num_steps_;

  //get the index for this trajectory in the matricies
  int mat_index = t_num * num_steps_;

  Trajectory traj(vx_samp, vy_samp, vtheta_samp);

  for(int i = 0; i < num_steps_; ++i){
    //add the point to the matrix
    trajectory_pts_(0, mat_index) = x_i;
    trajectory_pts_(1, mat_index) = y_i;
    trajectory_pts_(2, mat_index) = 0;
    trajectory_pts_(3, mat_index) = 1;

    //add theta to the matrix
    trajectory_theta_(0, mat_index) = theta_i;

    ++mat_index;

    //calculate velocities
    vx_i = computeNewVelocity(vx_samp, vx_i, acc_x, dt);
    vy_i = computeNewVelocity(vy_samp, vy_i, acc_y, dt);
    vtheta_i = computeNewVelocity(vtheta_samp, vtheta_i, acc_theta, dt);

    //calculate positions
    x_i = computeNewXPosition(x_i, vx_i, vy_i, theta_i, dt);
    y_i = computeNewYPosition(y_i, vx_i, vy_i, theta_i, dt);
    theta_i = computeNewThetaPosition(theta_i, vtheta_i, dt);
    
  }
  
  return traj;
}

void TrajectoryController::updatePlan(const vector<Point2DFloat32>& new_plan){
  global_plan_.resize(new_plan.size());
  for(unsigned int i = 0; i < new_plan.size(); ++i){
    global_plan_[i] = new_plan[i];
  }
}

void TrajectoryController::trajectoriesToWorld(){
  libTF::TFPose2D robot_pose, global_pose;
  robot_pose.x = 0;
  robot_pose.y = 0;
  robot_pose.yaw = 0;
  robot_pose.frame = "base";
  robot_pose.time = 0;

  if(tf_){
    try
    {
      global_pose = tf_->transformPose2D("map", robot_pose);
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
      //      puts("extrapolation required");
      //      printf("%s\n", ex.what());
      return;
    }
  }

  struct timeval start;
  struct timeval end;
  double start_t, end_t, t_diff;
  gettimeofday(&start,NULL);

  transformTrajects(global_pose.x, global_pose.y, global_pose.yaw);

  gettimeofday(&end,NULL);
  start_t = start.tv_sec + double(start.tv_usec) / 1e6;
  end_t = end.tv_sec + double(end.tv_usec) / 1e6;
  t_diff = end_t - start_t;
  //fprintf(stderr, "Matrix Time: %.3f\n", t_diff);

}

//transform trajectories from robot space to map space
void TrajectoryController::transformTrajects(double x_i, double y_i, double th_i){
  double cos_th = cos(th_i);
  double sin_th = sin(th_i);

  double new_x, new_y, old_x, old_y;
  for(unsigned int i = 0; i < trajectory_pts_.size2(); ++i){
    old_x = trajectory_pts_(0, i);
    old_y = trajectory_pts_(1, i);
    new_x = x_i + old_x * cos_th - old_y * sin_th;
    new_y = y_i + old_x * sin_th + old_y * cos_th;
    trajectory_pts_(0, i) = new_x;
    trajectory_pts_(1, i) = new_y;
    trajectory_theta_(0, i) += th_i;
  }
}

//create the trajectories we wish to score
void TrajectoryController::createTrajectories(double x, double y, double theta, double vx, double vy, double vtheta,
    double acc_x, double acc_y, double acc_theta){
  //compute feasible velocity limits in robot space
  double max_vel_x, max_vel_y, max_vel_theta;
  double min_vel_x, min_vel_y, min_vel_theta;

  max_vel_x = min(1.0, vx + acc_x * sim_time_);
  min_vel_x = max(0.1, vx - acc_x * sim_time_);

  max_vel_y = vy + acc_y * sim_time_;
  min_vel_y = vy - acc_y * sim_time_;

  //max_vel_theta = vtheta + acc_theta * sim_time_;
  //min_vel_theta = vtheta - acc_theta * sim_time_;

  max_vel_theta = 1.0;
  min_vel_theta = -1.0;

  //we want to sample the velocity space regularly
  double dvx = (max_vel_x - min_vel_x) / samples_per_dim_;
  double dvy = (max_vel_y - min_vel_y) / samples_per_dim_;
  double dvtheta = (max_vel_theta - min_vel_theta) / samples_per_dim_;

  double vx_samp = min_vel_x;
  double vtheta_samp = min_vel_theta;
  //double vy_samp = min_vel_y;
  double vy_samp = 0.0;

  //make sure to reset the list of trajectories
  trajectories_.clear();

  //keep track of which trajectory we're working on
  int t_num = 0;

  //generate trajectories for regularly sampled velocities
  for(int i = 0; i < samples_per_dim_; ++i){
    //vy_samp = min_vel_y;
    vtheta_samp = min_vel_theta;
    for(int j = 0; j < samples_per_dim_; ++j){
      //vy_samp = min_vel_y;
      //for(int k = 0; k < samples_per_dim_; ++k){
        trajectories_.push_back(generateTrajectory(t_num, x, y, theta, vx, vy, vtheta, vx_samp, vy_samp, vtheta_samp, acc_x, acc_y, acc_theta));
        ++t_num;
        //vy_samp += dvy;
      //}
      vtheta_samp += dvtheta;
    }
    vx_samp += dvx;
  }

  //next we want to generate trajectories for rotating in place
  vtheta_samp = min_vel_theta;
  vx_samp = 0.0;
  vy_samp = 0.0;
  for(int i = 0; i < samples_per_dim_; ++i){
    trajectories_.push_back(generateTrajectory(t_num, x, y, theta, vx, vy, vtheta, vx_samp, vy_samp, vtheta_samp, acc_x, acc_y, acc_theta));
    ++t_num;
    vtheta_samp += dvtheta;
  }

  //and finally we want to generate trajectories that move backwards slowly
  //vtheta_samp = min_vel_theta;
  vtheta_samp = 0.0;
  vx_samp = -0.05;
  vy_samp = 0.0;
  for(int i = 0; i < samples_per_dim_; ++i){
    trajectories_.push_back(generateTrajectory(t_num, x, y, theta, vx, vy, vtheta, vx_samp, vy_samp, vtheta_samp, acc_x, acc_y, acc_theta));
    ++t_num;
    //vtheta_samp += dvtheta;
  }
  
}

//given the current state of the robot, find a good trajectory
int TrajectoryController::findBestPath(libTF::TFPose2D global_pose, libTF::TFPose2D global_vel, 
    libTF::TFPose2D& drive_velocities){
  //make sure that we update our path based on the global plan and compute costs
  setPathCells();
  printf("Path/Goal distance computed\n");

  /*
  //If we want to print a ppm file to draw goal dist
  printf("P3\n");
  printf("%d %d\n", map_.size_x_, map_.size_y_);
  printf("255\n");
  for(int j = map_.size_y_ - 1; j >= 0; --j){
    for(unsigned int i = 0; i < map_.size_x_; ++i){
      int g_dist = 255 - int(map_(i, j).goal_dist);
      int p_dist = 255 - int(map_(i, j).path_dist);
      if(g_dist < 0)
        g_dist = 0;
      if(p_dist < 0)
        p_dist = 0;
      printf("%d 0 %d ", g_dist, 0);
    }
    printf("\n");
  }
  */

  //next create the trajectories we wish to explore
  createTrajectories(global_pose.x, global_pose.y, global_pose.yaw, global_vel.x, global_vel.y, global_vel.yaw, 
      acc_lim_x_, acc_lim_y_, acc_lim_theta_);
  printf("Trajectories created\n");

  //we need to transform the trajectories to world space for scoring
  trajectoriesToWorld();
  printf("Trajectories converted\n");

  //now we want to score the trajectories that we've created and return the best one
  double min_cost = DBL_MAX;

  //default to a trajectory that goes nowhere
  int best_index = -1;

  //anything with a cost greater than the size of the map is impossible
  double impossible_cost = map_.map_.size();

  double x = trajectory_pts_(0, 0);
  double y = trajectory_pts_(1, 0);
  double theta = trajectory_theta_(0, 0);
  vector<std_msgs::Position2DInt> footprint_list = getFootprintCells(x, y, theta, true);

  //mark cells within the initial footprint of the robot
  for(unsigned int i = 0; i < footprint_list.size(); ++i){
    map_(footprint_list[i].x, footprint_list[i].y).within_robot = true;
  }

  //we know that everything except the last 2 sets of trajectories are forward
  unsigned int forward_traj_end = trajectories_.size() - 2 * samples_per_dim_;
  for(unsigned int i = 0; i < forward_traj_end; ++i){
    double cost = trajectoryCost(i, pdist_scale_, gdist_scale_, occdist_scale_, dfast_scale_, impossible_cost);

    //so we can draw with cost info
    trajectories_[i].cost_ = cost;

    //find the minimum cost path
    if(cost >= 0 && cost < min_cost){
      best_index = i;
      min_cost = cost;
    }
  }
  
  unsigned int rot_traj_end = trajectories_.size() - samples_per_dim_;
  //we want to explore rotational trajectories as well
  bool legal_right = false;
  double max_vel = 0;
  for(unsigned int i = forward_traj_end; i < rot_traj_end; ++i){
    double cost = -1;

    if(trajectories_[i].thetav_ < 0 && !stuck_right){
      cost = trajectoryCost(i, pdist_scale_, gdist_scale_, occdist_scale_, dfast_scale_, impossible_cost);
      if(cost >= 0)
        legal_right = true;
    }
    else if(trajectories_[i].thetav_ > 0 && stuck_right && !stuck_left){
      cost = trajectoryCost(i, pdist_scale_, gdist_scale_, occdist_scale_, dfast_scale_, impossible_cost);
    }

    //so we can draw with cost info
    trajectories_[i].cost_ = cost;

    //find the minimum cost path
    double abs_speed = abs(trajectories_[i].thetav_);
    if(cost >= 0 && cost <= min_cost && abs_speed > max_vel){
      best_index = i;
      min_cost = cost;
      max_vel = abs_speed;
    }
  }

  if(stuck_right && best_index < 0)
    stuck_left = true;

  if(!legal_right)
    stuck_right = true;


  //if we have a valid path... return
  if(best_index >= 0){
    if(trajectories_[best_index].xv_ > 0){
      stuck_left = false;
      stuck_right = false;
    }
    drive_velocities = getDriveVelocities(best_index);
    return best_index;
  }

  //the last set of trajectories is for moving backwards
  for(unsigned int i = rot_traj_end; i < trajectories_.size(); ++i){
    double cost = trajectoryCost(i, pdist_scale_, gdist_scale_, occdist_scale_, dfast_scale_, impossible_cost);

    //so we can draw with cost info
    trajectories_[i].cost_ = cost;

    //find the minimum cost path
    if(cost >= 0 && cost < min_cost){
      best_index = i;
      min_cost = cost;
    }
    best_index = i;
  }
  //printf("Trajectories scored\n");

  drive_velocities = getDriveVelocities(best_index);
  stuck_left = false;
  stuck_right = false;
  return best_index;
}

//compute the cost for a single trajectory
double TrajectoryController::trajectoryCost(int t_index, double pdist_scale,
    double gdist_scale, double occdist_scale, double dfast_scale, double impossible_cost){
  Trajectory t = trajectories_[t_index];
  double path_dist = 0.0;
  double goal_dist = 0.0;
  double occ_dist = 0.0;
  int start_index = t_index * num_steps_;
  for(int i = 0; i < num_steps_; ++i){
    int mat_index = start_index + i;
    double x = trajectory_pts_(0, mat_index);
    double y = trajectory_pts_(1, mat_index);
    double theta = trajectory_theta_(0, mat_index);

    //convert world to map coords
    unsigned int cell_x, cell_y;
    //we don't want a path that goes off the known map
    if(!ma_.WC_MC(x, y, cell_x, cell_y))
      return -1.0;

    //we need to check if we have to lay down the footprint of the robot
    if(ma_.isInflatedObstacle(cell_x, cell_y)){
      //if we do compute the obstacle cost for each cell in the footprint
      double footprint_cost = footprintCost(x, y, theta);
      if(footprint_cost < 0)
        return -1.0;
      occ_dist += footprint_cost;
    }

    double cell_pdist = map_(cell_x, cell_y).path_dist;
    double cell_gdist = map_(cell_x, cell_y).goal_dist;


    path_dist = cell_pdist;
    goal_dist = cell_gdist;

  }

  //if we cannot follow the path let the global planner know it is impossible
  if(impossible_cost <= goal_dist || impossible_cost <= path_dist)
    return -1.0;

  double cost = pdist_scale * path_dist + gdist_scale * goal_dist + dfast_scale * (1.0 / ((.05 + t.xv_) * (.05 + t.xv_))) + occdist_scale *  (1 / ((occ_dist + .05) * (occ_dist + .05)));
  
  return cost;
}

//given a trajectory in map space get the drive commands to send to the robot
libTF::TFPose2D TrajectoryController::getDriveVelocities(int t_num){
  libTF::TFPose2D tVel;
  //if no legal trajectory was found... stop the robot
  if(t_num < 0){
    tVel.x = 0.0;
    tVel.y = 0.0;
    tVel.yaw = 0.0;
    tVel.frame = "base";
    tVel.time = 0;
    return tVel;
  }

  //otherwise return the velocity cmds
  const Trajectory& t = trajectories_[t_num];
  tVel.x = t.xv_;
  tVel.y = t.yv_;
  tVel.yaw = t.thetav_;
  tVel.frame = "base";
  tVel.time = 0;
  return tVel;
}

void TrajectoryController::swap(int& a, int& b){
  int temp = a;
  a = b;
  b = temp;
}

double TrajectoryController::pointCost(int x, int y){
  //if the cell is in an obstacle the path is invalid
  if(ma_.isObstacle(x, y) && !(map_(x, y).within_robot)){
    return -1;
  }

  //check if we need to add an obstacle distance
  if(map_(x, y).occ_dist < max_occ_dist_)
    return map_(x, y).occ_dist;

  return 0.0;
}

//calculate the cost of a ray-traced line
double TrajectoryController::lineCost(int x0, int x1, 
    int y0, int y1, vector<std_msgs::Position2DInt>& footprint_cells){
  //Bresenham Ray-Tracing
  int deltax = abs(x1 - x0);        // The difference between the x's
  int deltay = abs(y1 - y0);        // The difference between the y's
  int x = x0;                       // Start x off at the first pixel
  int y = y0;                       // Start y off at the first pixel

  int xinc1, xinc2, yinc1, yinc2;
  int den, num, numadd, numpixels;

  double line_cost = 0.0;
  double point_cost = -1.0;

  if (x1 >= x0)                 // The x-values are increasing
  {
    xinc1 = 1;
    xinc2 = 1;
  }
  else                          // The x-values are decreasing
  {
    xinc1 = -1;
    xinc2 = -1;
  }

  if (y1 >= y0)                 // The y-values are increasing
  {
    yinc1 = 1;
    yinc2 = 1;
  }
  else                          // The y-values are decreasing
  {
    yinc1 = -1;
    yinc2 = -1;
  }

  if (deltax >= deltay)         // There is at least one x-value for every y-value
  {
    xinc1 = 0;                  // Don't change the x when numerator >= denominator
    yinc2 = 0;                  // Don't change the y for every iteration
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax;         // There are more x-values than y-values
  }
  else                          // There is at least one y-value for every x-value
  {
    xinc2 = 0;                  // Don't change the x for every iteration
    yinc1 = 0;                  // Don't change the y when numerator >= denominator
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay;         // There are more y-values than x-values
  }

  for (int curpixel = 0; curpixel <= numpixels; curpixel++)
  {
    point_cost = pointCost(x, y); //Score the current point

    if(point_cost < 0)
      return -1;

    Position2DInt pt;
    pt.x = x;
    pt.y = y;
    footprint_cells.push_back(pt);
    line_cost += point_cost;

    num += numadd;              // Increase the numerator by the top of the fraction
    if (num >= den)             // Check if numerator >= denominator
    {
      num -= den;               // Calculate the new numerator value
      x += xinc1;               // Change the x as appropriate
      y += yinc1;               // Change the y as appropriate
    }
    x += xinc2;                 // Change the x as appropriate
    y += yinc2;                 // Change the y as appropriate
  }

  return line_cost;
}

//we need to take the footprint of the robot into account when we calculate cost to obstacles
double TrajectoryController::footprintCost(double x_i, double y_i, double theta_i){
  //need to keep track of what cells are in the footprint
  vector<std_msgs::Position2DInt> footprint_cells;

  double cos_th = cos(theta_i);
  double sin_th = sin(theta_i);

  double footprint_dist = 0.0;
  double line_dist = -1.0;

  //upper right corner
  double old_x = 0.0 + robot_front_radius_;
  double old_y = 0.0 + robot_side_radius_;
  double new_x = x_i + old_x * cos_th - old_y * sin_th;
  double new_y = y_i + old_x * sin_th + old_y * cos_th;

  unsigned int x0, y0;
  if(!ma_.WC_MC(new_x, new_y, x0, y0))
    return -1.0;

  //lower right corner
  old_x = 0.0 + robot_front_radius_;
  old_y = 0.0 - robot_side_radius_;
  new_x = x_i + old_x * cos_th - old_y * sin_th;
  new_y = y_i + old_x * sin_th + old_y * cos_th;
  unsigned int x1, y1;
  if(!ma_.WC_MC(new_x, new_y, x1, y1))
    return -1.0;

  //check the front line
  line_dist = lineCost(x0, x1, y0, y1, footprint_cells);
  if(line_dist < 0)
    return -1;

  footprint_dist += line_dist;

  //lower left corner
  old_x = 0.0 - robot_front_radius_;
  old_y = 0.0 - robot_side_radius_;
  new_x = x_i + old_x * cos_th - old_y * sin_th;
  new_y = y_i + old_x * sin_th + old_y * cos_th;
  unsigned int x2, y2;
  if(!ma_.WC_MC(new_x, new_y, x2, y2))
    return -1.0;

  //check the right side line
  line_dist = lineCost(x1, x2, y1, y2, footprint_cells);
  if(line_dist < 0)
    return -1;

  footprint_dist += line_dist;
  
  //upper left corner
  old_x = 0.0 - robot_front_radius_;
  old_y = 0.0 + robot_side_radius_;
  new_x = x_i + old_x * cos_th - old_y * sin_th;
  new_y = y_i + old_x * sin_th + old_y * cos_th;
  unsigned int x3, y3;
  if(!ma_.WC_MC(new_x, new_y, x3, y3))
    return -1.0;

  //check the back line
  line_dist = lineCost(x2, x3, y2, y3, footprint_cells);
  if(line_dist < 0)
    return -1;

  footprint_dist += line_dist;

  //check the left side line
  line_dist = lineCost(x3, x0, y3, y0, footprint_cells);
  if(line_dist < 0)
    return -1;

  footprint_dist += line_dist;

  double fill_dist = -1;
  fill_dist = fillCost(footprint_cells);

  return footprint_dist;
}

//we need to fill in the footprint of the robot
double TrajectoryController::fillCost(vector<std_msgs::Position2DInt>& footprint){
  //quick bubble sort to sort pts by x
  std_msgs::Position2DInt swap;
  unsigned int i = 0;
  while(i < footprint.size() - 1){
    if(footprint[i].x > footprint[i + 1].x){
      swap = footprint[i];
      footprint[i] = footprint[i + 1];
      footprint[i + 1] = swap;
      if(i > 0)
        --i;
    }
    else
      ++i;
  }

  i = 0;
  std_msgs::Position2DInt min_pt;
  std_msgs::Position2DInt max_pt;
  unsigned int min_x = footprint[0].x;
  unsigned int max_x = footprint[footprint.size() -1].x;
  double fill_cost = 0.0;
  double point_cost = -1.0;
  //walk through each column and mark cells inside the footprint
  for(unsigned int x = min_x; x <= max_x; ++x){
    if(i >= footprint.size() - 1)
      break;

    if(footprint[i].y < footprint[i + 1].y){
      min_pt = footprint[i];
      max_pt = footprint[i + 1];
    }
    else{
      min_pt = footprint[i + 1];
      max_pt = footprint[i];
    }

    i += 2;
    while(i < footprint.size() && footprint[i].x == x){
      if(footprint[i].y < min_pt.y)
        min_pt = footprint[i];
      else if(footprint[i].y > max_pt.y)
        max_pt = footprint[i];
      ++i;
    }

    //loop though cells in the column
    for(unsigned int y = min_pt.y; y < max_pt.y; ++y){
      point_cost = pointCost(x, y);
      if(point_cost < 0)
        return -1;
      fill_cost += point_cost;
    }
  }

  return fill_cost;
}

void TrajectoryController::getLineCells(int x0, int x1, int y0, int y1, vector<std_msgs::Position2DInt>& pts){
  //Bresenham Ray-Tracing
  int deltax = abs(x1 - x0);        // The difference between the x's
  int deltay = abs(y1 - y0);        // The difference between the y's
  int x = x0;                       // Start x off at the first pixel
  int y = y0;                       // Start y off at the first pixel

  int xinc1, xinc2, yinc1, yinc2;
  int den, num, numadd, numpixels;

  std_msgs::Position2DInt pt;

  if (x1 >= x0)                 // The x-values are increasing
  {
    xinc1 = 1;
    xinc2 = 1;
  }
  else                          // The x-values are decreasing
  {
    xinc1 = -1;
    xinc2 = -1;
  }

  if (y1 >= y0)                 // The y-values are increasing
  {
    yinc1 = 1;
    yinc2 = 1;
  }
  else                          // The y-values are decreasing
  {
    yinc1 = -1;
    yinc2 = -1;
  }

  if (deltax >= deltay)         // There is at least one x-value for every y-value
  {
    xinc1 = 0;                  // Don't change the x when numerator >= denominator
    yinc2 = 0;                  // Don't change the y for every iteration
    den = deltax;
    num = deltax / 2;
    numadd = deltay;
    numpixels = deltax;         // There are more x-values than y-values
  }
  else                          // There is at least one y-value for every x-value
  {
    xinc2 = 0;                  // Don't change the x for every iteration
    yinc1 = 0;                  // Don't change the y when numerator >= denominator
    den = deltay;
    num = deltay / 2;
    numadd = deltax;
    numpixels = deltay;         // There are more y-values than x-values
  }

  for (int curpixel = 0; curpixel <= numpixels; curpixel++)
  {
    pt.x = x;      //Draw the current pixel
    pt.y = y;
    pts.push_back(pt);

    num += numadd;              // Increase the numerator by the top of the fraction
    if (num >= den)             // Check if numerator >= denominator
    {
      num -= den;               // Calculate the new numerator value
      x += xinc1;               // Change the x as appropriate
      y += yinc1;               // Change the y as appropriate
    }
    x += xinc2;                 // Change the x as appropriate
    y += yinc2;                 // Change the y as appropriate
  }
}

//its nice to be able to draw a footprint for a particular point for debugging info
vector<std_msgs::Point2DFloat32> TrajectoryController::drawFootprint(double x_i, double y_i, double theta_i){
  vector<std_msgs::Position2DInt> footprint_cells = getFootprintCells(x_i, y_i, theta_i, false);
  vector<std_msgs::Point2DFloat32> footprint_pts;
  Point2DFloat32 pt;
  for(unsigned int i = 0; i < footprint_cells.size(); ++i){
    double pt_x, pt_y;
    ma_.MC_WC(footprint_cells[i].x, footprint_cells[i].y, pt_x, pt_y);
    pt.x = pt_x;
    pt.y = pt_y;
    footprint_pts.push_back(pt);
  }
  return footprint_pts;
}

//get the cellsof a footprint at a given position
vector<std_msgs::Position2DInt> TrajectoryController::getFootprintCells(double x_i, double y_i, double theta_i, bool fill){
  vector<std_msgs::Position2DInt> footprint_cells;
  double cos_th = cos(theta_i);
  double sin_th = sin(theta_i);

  //upper right corner
  double old_x = 0.0 + robot_front_radius_;
  double old_y = 0.0 + robot_side_radius_;
  double new_x = x_i + (old_x * cos_th - old_y * sin_th);
  double new_y = y_i + (old_x * sin_th + old_y * cos_th);

  unsigned int x0 = 0, y0 = 0;
  assert(ma_.WC_MC(new_x, new_y, x0, y0));

  //lower right corner
  old_x = 0.0 + robot_front_radius_;
  old_y = 0.0 - robot_side_radius_;
  new_x = x_i + (old_x * cos_th - old_y * sin_th);
  new_y = y_i + (old_x * sin_th + old_y * cos_th);

  unsigned int x1 = 0, y1 = 0;
  assert(ma_.WC_MC(new_x, new_y, x1, y1));

  //check the front line
  getLineCells(x0, x1, y0, y1, footprint_cells);

  //lower left corner
  old_x = 0.0 - robot_front_radius_;
  old_y = 0.0 - robot_side_radius_;
  new_x = x_i + (old_x * cos_th - old_y * sin_th);
  new_y = y_i + (old_x * sin_th + old_y * cos_th);

  unsigned int x2 = 0, y2 = 0;
  assert(ma_.WC_MC(new_x, new_y, x2, y2));

  //check the right side line
  getLineCells(x1, x2, y1, y2, footprint_cells);
  
  //upper left corner
  old_x = 0.0 - robot_front_radius_;
  old_y = 0.0 + robot_side_radius_;
  new_x = x_i + (old_x * cos_th - old_y * sin_th);
  new_y = y_i + (old_x * sin_th + old_y * cos_th);
  
  unsigned int x3 = 0, y3 = 0;
  assert(ma_.WC_MC(new_x, new_y, x3, y3));

  //check the back line
  getLineCells(x2, x3, y2, y3, footprint_cells);

  //check the left side line
  getLineCells(x3, x0, y3, y0, footprint_cells);

  if(fill)
    getFillCells(footprint_cells);

  return footprint_cells;
}

void TrajectoryController::getFillCells(vector<std_msgs::Position2DInt>& footprint){
  //quick bubble sort to sort pts by x
  std_msgs::Position2DInt swap, pt;
  unsigned int i = 0;
  while(i < footprint.size() - 1){
    if(footprint[i].x > footprint[i + 1].x){
      swap = footprint[i];
      footprint[i] = footprint[i + 1];
      footprint[i + 1] = swap;
      if(i > 0)
        --i;
    }
    else
      ++i;
  }

  i = 0;
  std_msgs::Position2DInt min_pt;
  std_msgs::Position2DInt max_pt;
  unsigned int min_x = footprint[0].x;
  unsigned int max_x = footprint[footprint.size() -1].x;
  //walk through each column and mark cells inside the footprint
  for(unsigned int x = min_x; x <= max_x; ++x){
    if(i >= footprint.size() - 1)
      break;

    if(footprint[i].y < footprint[i + 1].y){
      min_pt = footprint[i];
      max_pt = footprint[i + 1];
    }
    else{
      min_pt = footprint[i + 1];
      max_pt = footprint[i];
    }

    i += 2;
    while(i < footprint.size() && footprint[i].x == x){
      if(footprint[i].y < min_pt.y)
        min_pt = footprint[i];
      else if(footprint[i].y > max_pt.y)
        max_pt = footprint[i];
      ++i;
    }

    //loop though cells in the column
    for(unsigned int y = min_pt.y; y < max_pt.y; ++y){
      pt.x = x;
      pt.y = y;
      footprint.push_back(pt);
    }
  }
}
