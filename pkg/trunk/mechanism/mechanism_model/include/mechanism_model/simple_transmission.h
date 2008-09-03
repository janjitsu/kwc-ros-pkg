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
/*
 * Author: Stuart Glaser
 */
#ifndef SIMPLE_TRANSMISSION_H
#define SIMPLE_TRANSMISSION_H

#include <tinyxml/tinyxml.h>
#include "mechanism_model/transmission.h"
#include "mechanism_model/joint.h"
#include "hardware_interface/hardware_interface.h"

namespace mechanism {

class SimpleTransmission : public Transmission
{
public:
  SimpleTransmission() {}
  SimpleTransmission(Joint *joint, Actuator *actuator, double mechanical_reduction, double motor_torque_constant, double pulses_per_revolution);
  ~SimpleTransmission() {}

  bool initXml(TiXmlElement *config, Robot *robot);

  std::string name_;
  std::string joint_name_;
  std::string actuator_name_;

  Actuator *actuator_;
  Joint *joint_;

  double mechanical_reduction_;
  double motor_torque_constant_;
  double pulses_per_revolution_;

  void propagatePosition();
  void propagateEffort();
  void propagatePositionBackwards();
  void propagateEffortBackwards();
};

} // namespace mechanism

#endif
