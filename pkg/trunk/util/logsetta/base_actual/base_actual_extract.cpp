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

#include "robot_msgs/BaseActualMsg.h"
#include <string>
#include "logging/LogPlayer.h"

void odom_callback(std::string name, ros::msg* m, ros::Time t, void* f)
{
  FILE* file = (FILE*)f;
  robot_msgs::BaseActualMsg* baseActual = (robot_msgs::BaseActualMsg*)(m);

  fprintf(file, "%.5f ",t.to_double());

  for(unsigned int i=0; i < baseActual->get_baseSteerPosition_size(); i++)
    {
      fprintf(file,"%.5f ",baseActual->baseSteerPosition[i]);
    }
  for(unsigned int i=0; i < baseActual->get_baseWheelPosition_size(); i++)
    {
      fprintf(file,"%.5f ",baseActual->baseWheelPosition[i]);
    }

  for(unsigned int i=0; i < baseActual->get_baseSteerVelocity_size(); i++)
    {
      fprintf(file,"%.5f ",baseActual->baseSteerVelocity[i]);
    }
  for(unsigned int i=0; i < baseActual->get_baseWheelVelocity_size(); i++)
    {
      fprintf(file,"%.5f ",baseActual->baseWheelVelocity[i]);
    }

  fprintf(file, "\n");
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("usage: imu_extract LOG\n");
    return 1;
  }

  LogPlayer player;

  player.open(std::string(argv[1]), ros::Time(0));

  int count;

  FILE* file = fopen("base_actual.txt", "w");

  count = player.addHandler<robot_msgs::BaseActualMsg>(std::string("baseActual"), &odom_callback, file, true);

  if (count != 1)
  {
    printf("Found %d '/odom' topics when expecting 1", count);
    return 1;
  }

  while(player.nextMsg())  {}

  fclose(file);

  return 0;
}
