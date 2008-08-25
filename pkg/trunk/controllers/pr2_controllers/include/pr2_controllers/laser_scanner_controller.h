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

#pragma once

#include <ros/node.h>

#include <generic_controllers/controller.h>
#include <generic_controllers/joint_position_controller.h>

// Services
#include <generic_controllers/SetCommand.h>
#include <generic_controllers/GetCommand.h>

namespace controller
{

class LaserScannerController : public Controller
{
public:

  enum LaserControllerMode
  {
    MANUAL,SAWTOOTH,SINEWAVE,DYNAMIC_SAWTOOTH,DYNAMIC_SINEWAVE,AUTO_LEVEL
  };

  /*!
   * \brief Default Constructor of the JointController class.
   *
   */
  LaserScannerController();

  /*!
   * \brief Destructor of the JointController class.
   */
  ~LaserScannerController();

  /*!
   * \brief Functional way to initialize limits and gains.
   *
   */
  void init(double p_gain, double i_gain, double d_gain, double windup, double time, std::string name, mechanism::Robot *robot);
  bool initXml(mechanism::Robot *robot, TiXmlElement *config);

  /*!
   * \brief Give set position of the joint for next update: revolute (angle) and prismatic (position)
   *
   * \param double pos Position command to issue
   */
  void setCommand(double command);

  /*!
   * \brief Get latest position command to the joint: revolute (angle) and prismatic (position).
   */
  double getCommand();

  /*!
   * \brief Read the torque of the motor
   */
  double getMeasuredPosition();

  /*!
   * \brief Issues commands to the joint. Should be called at regular intervals
   */

  virtual void update();

  /*!
   * \brief Set automatic profile to sawtooth
   *\param double period Period of signal
   *\param double amplitude Peak to peak amplitude of signal
   *\param int num_elements Number of points along one period of sawtooth wave
   *\param double offset Offset of minimum point of signal to zero
   *\param double current_time Used to determine start of cycle
   */

  void setSawtoothProfile(double period, double amplitude, int num_elements, double offset);

  /*!
   * \brief Set automatic profile to sawtooth, dynamically calculate desired position at each timestep
   *\param double period Period of signal
   *\param double amplitude Peak to peak amplitude of signal
   *\param double offset Offset of minimum point of signal to zero
   *\param double current_time Used to determine start of cycle
   */
  void setSawtoothProfile(double period, double amplitude, double offset);

  /*!
   * \brief Set automatic profile to sinewave
   *\param double period Period of signal
   *\param double amplitude Peak to peak amplitude of signal
   *\param int num_elements Number of points along one period of sine wave
   *\param double offset Offset of minimum point of signal to zero
   *\param double current_time Used to determine start of cycle
   */
  void setSinewaveProfile(double period, double amplitude, int num_elements, double offset);

  /*!
   * \brief Set automatic profile to sinewave, dynamically calculate desired position at each timestep
   *\param double period Period of signal
   *\param double amplitude Peak to peak amplitude of signal
   *\param double offset Offset of minimum point of signal to zero
   *\param double current_time Used to determine start of cycle
   */
  void setSinewaveProfile(double period, double amplitude,double offset);

  /*!
   * \brief Starts the process of auto-leveling
   */
  void startAutoLevelSequence();

  /*!
   * \brief Returns a value indicating whether auto leveling has finished
   */
  bool checkAutoLevelStatus();

  /*!
   * \brief Returns whether auto level completed successfully
   */
  bool checkAutoLevelResult();

private:
  /*!
   * \brief Actually issue torque set command of the joint motor.
   */
  void setJointEffort(double torque);

  /*!
   * \brief Get dynamically calculated sinewave position based on time
   *\param double time_from_start Time elapsed since beginning of current period
   */
  void setDynamicSinewave(double time_from_start);

   /*!
   * \brief Get dynamically calculated sawtooth position based on time
   *\param double time_from_start Time elapsed since beginning of current period
   */
  void setDynamicSawtooth(double time_from_start);


  mechanism::Joint* joint_; /*!< Joint we're controlling>*/
  JointPositionController joint_position_controller_; /*!< Internal PID controller>*/
  double last_time_; /*!< Last time stamp of update> */
  double command_; /*!< Last commanded position> */
  mechanism::Robot *robot_; /*!< Pointer to robot structure>*/
  double* profile_locations_; /**<Contains locations for profile>*/
  double* profile_dt_; /**<Contains timesteps for profile locations>*/
  int profile_index_; /**<Track location in profile>*/
  int profile_length_; /**<Number of points in one cycle>*/
  double cycle_start_time_; //**<Start of the last cycle for profile>*/

  double time_of_last_point_;/*!<Time of last setpoint>*/
  double period_;/*!<Period for use in dynamic profile calculation>*/
  double amplitude_;/*!<Amplitude for use in dynamic profile calculation>*/
  double offset_;/*!<Offset for use in dynamic profile calculation>*/


  LaserControllerMode current_mode_; /*!<Indicates the current status of the controller>*/
  bool auto_level_result_; /*!<Indicates whether the auto_level_routine finished correct>*/

};

class LaserScannerControllerNode : public Controller
{
public:
  /*!
   * \brief Default Constructor
   *
   */
  LaserScannerControllerNode();

  /*!
   * \brief Destructor
   */
  ~LaserScannerControllerNode();

  double getMeasuredPosition();

  void update();

  bool initXml(mechanism::Robot *robot, TiXmlElement *config);

  bool setTestCommand(double command);
  // Services
  bool setCommand(generic_controllers::SetCommand::request &req,
                  generic_controllers::SetCommand::response &resp);

  bool getCommand(generic_controllers::GetCommand::request &req,
                  generic_controllers::GetCommand::response &resp);

private:
  LaserScannerController *c_;
};
}


