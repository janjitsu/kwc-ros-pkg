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

// Author: Advait Jain

#include <ros/node.h>
#include <libpr2API/pr2API.h>
#include <pr2_msgs/EndEffectorState.h>

using namespace KDL;

int main(int argc, char **argv)
{  
  ros::init(argc, argv);

  ros::node mynode("test_inter+kin_con");
  mynode.advertise<pr2_msgs::EndEffectorState>("right_pr2arm_set_end_effector");

  //In shoulder frame x 0.562689
  //In shoulder frame y -0.367447
  //In shoulder frame z -0.369594
  Rotation r = Rotation::RotZ(deg2rad*0);
  Vector v(.562689,-.367447,-.369594);

  std::cout << " rot: " << std::endl;
  std::cout << r.data[0] << std::endl;
  std::cout << r.data[1] << std::endl;
  std::cout << r.data[2] << std::endl;
  std::cout << r.data[3] << std::endl;
  std::cout << r.data[4] << std::endl;
  std::cout << r.data[5] << std::endl;
  std::cout << r.data[6] << std::endl;
  std::cout << r.data[7] << std::endl;
  std::cout << r.data[8] << std::endl;
  
  sleep(1);

  pr2_msgs::EndEffectorState efs;
  efs.set_rot_size(9);
  efs.set_trans_size(3);
  for(int i = 0; i < 9; i++)
    efs.rot[i] = r.data[i];

  for(int i = 0; i < 3; i++)
    efs.trans[i] = v.data[i];


  mynode.publish("right_pr2arm_set_end_effector",efs);
  sleep(1);
  mynode.shutdown();
  return 0;    
}
