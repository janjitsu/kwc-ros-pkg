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

#include "odom_estimation_node.h"


using namespace MatrixWrapper;
using namespace std;
using namespace ros;
using namespace tf;


namespace estimation
{
  // constructor
  odom_estimation_node::odom_estimation_node()
    : ros::node("odom_estimation"),
      _vel_desi(2),
      _vel_active(false),
      _odom_active(false),
      _imu_active(false),
      _vo_active(false)
  {
    // advertise our estimation
    advertise<std_msgs::PoseStamped>("odom_estimation",10);

    // subscribe to messages
    subscribe("cmd_vel",      _vel,  &odom_estimation_node::vel_callback,  10);
    subscribe("odom",         _odom, &odom_estimation_node::odom_callback, 10);
    subscribe("imu_data",     _imu,  &odom_estimation_node::imu_callback,  10);
    subscribe("vo_data",      _vo,   &odom_estimation_node::vo_callback,   10);

    _odom_file.open("odom_file.txt");
    _imu_file.open("imu_file.txt");
    _vo_file.open("vo_file.txt");
    _corr_file.open("corr_file.txt");
  };



  // destructor
  odom_estimation_node::~odom_estimation_node(){
    _odom_file.close();
    _imu_file.close();
    _vo_file.close();
    _corr_file.close();
  };




  // update filter
  void odom_estimation_node::Update(const Time& time)
  {
    // update filter
    if ( _my_filter.IsInitialized() )  {
      // update filter
      _my_filter.Update(_odom_time, _odom_active, _imu_time,  _imu_active, _vo_time,   _vo_active,  time);
      
      // write to file
      ColumnVector estimate; Time tm;
      _my_filter.GetEstimate(estimate, tm);
      for (unsigned int i=1; i<=6; i++)
	_corr_file << estimate(i) << " ";
      _corr_file << tm << endl;
      
      // output estimate message
      _my_filter.GetEstimate(_output);
      publish("odom_estimation", _output);
    }

    // initialize filer with odometry frame
    if ( _odom_active && !_my_filter.IsInitialized())
      _my_filter.Initialize(_odom_meas, _odom_time);

  };





  // callback function for odom data
  void odom_estimation_node::odom_callback()
  {
    // receive data
    _filter_mutex.lock();
    _odom_time = _odom.header.stamp;
    _odom_meas = Transform(Quaternion(_odom.pos.th,0,0), Vector3(_odom.pos.x, _odom.pos.y, 0));
    _my_filter.AddMeasurement(_odom_meas, "odom", "base", _odom.header.stamp);
    // update filter
    this->Update(_odom_time);
    _filter_mutex.unlock();

    // write to file
    double tmp, yaw;
    _odom_meas.getBasis().getEulerZYX(yaw, tmp, tmp);
    _odom_file << _odom_meas.getOrigin().x() << " " << _odom_meas.getOrigin().y() << "  " << yaw << endl;

    // activate odom
    if (!_odom_active) _odom_active = true;
  };




  // callback function for imu data
  void odom_estimation_node::imu_callback()
  {
    // receive data
    _filter_mutex.lock();
    _imu_time = _imu.header.stamp;
    PoseMsgToTF(_imu.pos, _imu_meas);
    _my_filter.AddMeasurement(_imu_meas, "imu", "base", _imu.header.stamp);
    _filter_mutex.unlock();

    // write to file
    double tmp, yaw;
    _imu_meas.getBasis().getEulerZYX(yaw, tmp, tmp);
    _imu_file << yaw << endl;

    // activate imu
    if (!_imu_active) _imu_active = true;
  };




  // callback function for VO data
  void odom_estimation_node::vo_callback()
  {
    // receive data
    _filter_mutex.lock();
    _vo_time =  _vo.header.stamp;
    PoseMsgToTF(_vo.pose, _vo_meas);
    _my_filter.AddMeasurement(_vo_meas, "vo", "base", _vo.header.stamp);
    _filter_mutex.unlock();

    // activate vo
    if (!_vo_active) _vo_active = true;
  };




  // callback function for vel data
  void odom_estimation_node::vel_callback()
  {
    // receive data
    _filter_mutex.lock();
    _vel_desi(1) = _vel.vx;   _vel_desi(2) = _vel.vw;
    _filter_mutex.unlock();

    // active
    //if (!_vel_active) _vel_active = true;
  };


}; // namespace






// ----------
// -- MAIN --
// ----------
using namespace estimation;
int main(int argc, char **argv)
{
  // Initialize ROS
  ros::init(argc, argv);

  // create filter class
  odom_estimation_node my_filter_node;

  // wait for filter to finish
  my_filter_node.spin();

  // Clean up
  ros::fini();
  return 0;
}
