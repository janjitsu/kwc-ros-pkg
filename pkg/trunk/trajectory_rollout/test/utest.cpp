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
#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <utility>
#include <trajectory_rollout/map_cell.h>
#include <trajectory_rollout/map_grid.h>
#include <trajectory_rollout/trajectory.h>
#include <trajectory_rollout/trajectory_controller.h>
#include <math.h>


using namespace std;

//make sure that we are getting the path distance map expected
TEST(TrajectoryController, correctPathDistance){
  MapGrid mg(6, 6);
  mg(2, 0).path_dist = 0.0;
  mg(2, 1).path_dist = 0.0;
  mg(3, 1).path_dist = 0.0;
  mg(4, 1).path_dist = 0.0;
  mg(4, 2).path_dist = 0.0;
  mg(4, 3).path_dist = 0.0;
  mg(3, 3).path_dist = 0.0;
  mg(2, 3).path_dist = 0.0;
  mg(1, 3).path_dist = 0.0;
  mg(1, 4).path_dist = 0.0;
  mg(1, 5).path_dist = 0.0;
  mg(2, 5).path_dist = 0.0;
  mg(3, 5).path_dist = 0.0;
  
  //place some obstacles
  mg(3,2).occ_state = 1;
  mg(5,3).occ_state = 1;
  mg(2,4).occ_state = 1;
  mg(0,5).occ_state = 1;

  //create a trajectory_controller
  TrajectoryController tc(mg, 1, 1, 1, 2.0, 20, 20, NULL);

  tc.computePathDistance();

  //test enough of the 36 cell grid to be convinced
  EXPECT_FLOAT_EQ(tc.map_(0,0).path_dist, 2.0);
  EXPECT_FLOAT_EQ(tc.map_(1,0).path_dist, 1.0);
  EXPECT_FLOAT_EQ(tc.map_(2,4).path_dist, DBL_MAX);
  EXPECT_FLOAT_EQ(tc.map_(5,0).path_dist, sqrt(2));
  EXPECT_FLOAT_EQ(tc.map_(0,4).path_dist, 1.0);
  EXPECT_FLOAT_EQ(tc.map_(3,3).path_dist, 0.0);
  EXPECT_FLOAT_EQ(tc.map_(0,1).path_dist, 2.0);
  EXPECT_FLOAT_EQ(tc.map_(0,5).path_dist, DBL_MAX);

  //reset the map
  for(unsigned int i = 0; i < mg.map_.size(); ++i)
    if(mg.map_[i].path_dist > 0.0)
      mg.map_[i].path_dist = DBL_MAX;

  mg(5,5).occ_state = 1;

  tc.computePathDistance();

  EXPECT_FLOAT_EQ(tc.map_(5,5).path_dist, DBL_MAX);

  //print the results
  /*
  cout.precision(2);
  for(int k = tc.map_.rows_ - 1 ; k >= 0; --k){
    for(unsigned int m = 0; m < tc.map_.cols_; ++m){
      cout << tc.map_(k, m).path_dist << " | ";
    }
    cout << endl;
  }
  */
}

//convince ourselves that trajectories generate as expected
TEST(TrajectoryController, properIntegration){
  MapGrid mg(6, 6);

  //create a path through the world
  mg(2, 0).path_dist = 0.0;
  mg(2, 1).path_dist = 0.0;
  mg(3, 1).path_dist = 0.0;
  mg(4, 1).path_dist = 0.0;
  mg(4, 2).path_dist = 0.0;
  mg(4, 3).path_dist = 0.0;
  mg(3, 3).path_dist = 0.0;
  mg(2, 3).path_dist = 0.0;
  mg(1, 3).path_dist = 0.0;
  mg(1, 4).path_dist = 0.0;
  mg(1, 5).path_dist = 0.0;
  mg(2, 5).path_dist = 0.0;
  mg(3, 5).path_dist = 0.0;

  //place some obstacles
  mg(3,2).occ_state = 1;
  mg(5,3).occ_state = 1;
  mg(2,4).occ_state = 1;
  mg(0,5).occ_state = 1;

  //create a trajectory_controller
  TrajectoryController tc(mg, 1, 1, 1, 2.0, 20, 20, NULL);

  tc.computePathDistance();

  Trajectory t1 = tc.generateTrajectory(2, 1, 0, 0, 0, 0, 1, 1, 1);

  //check x integration fo position and velocity
  EXPECT_FLOAT_EQ(t1.points_[t1.points_.size() - 1].x_, 3.45);
  EXPECT_FLOAT_EQ(t1.points_[t1.points_.size() - 1].xv_, 1.00);
  EXPECT_FLOAT_EQ(t1.points_[2].xv_, 0.20);

  //check y integration fo position and velocity
  EXPECT_FLOAT_EQ(t1.points_[t1.points_.size() - 1].y_, 2.45);
  EXPECT_FLOAT_EQ(t1.points_[t1.points_.size() - 1].yv_, 1.00);
  EXPECT_FLOAT_EQ(t1.points_[2].yv_, 0.20);

  //check theta integration fo position and velocity
  EXPECT_FLOAT_EQ(t1.points_[t1.points_.size() - 1].theta_, 1.45);
  EXPECT_FLOAT_EQ(t1.points_[t1.points_.size() - 1].thetav_, 1.00);
  EXPECT_FLOAT_EQ(t1.points_[2].thetav_, 0.20);

  /*
  for(unsigned int i = 0; i < t1.points_.size(); ++i){
    TrajectoryPoint p = t1.points_[i];
    printf("%d -  time: %4f, x: %4f, y: %4f, theta: %4f\n", i, p.t_, p.x_, p.y_, p.theta_);
    printf("%d -  time: %4f, xv: %4f, yv: %4f, thetav: %4f\n", i, p.t_, p.xv_, p.yv_, p.thetav_);
  }
  */

}

//sanity check to make sure the grid functions correctly
TEST(MapGrid, properGridConstruction){
  MapGrid mg(10, 10);
  MapCell mc;

  for(int i = 0; i < 10; ++i){
    for(int j = 0; j < 10; ++j){
      mc.ci = i;
      mc.cj = j;
      mg(i, j) = mc;
    }
  }

  for(int i = 0; i < 10; ++i){
    for(int j = 0; j < 10; ++j){
      EXPECT_FLOAT_EQ(mg(i, j).ci, i);
      EXPECT_FLOAT_EQ(mg(i, j).cj, j);
    }
  }
}

//test some stuff
int main(int argc, char** argv){
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
