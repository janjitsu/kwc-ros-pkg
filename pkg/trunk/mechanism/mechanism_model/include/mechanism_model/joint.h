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
 *
 */
#ifndef JOINT_H
#define JOINT_H

#include <tinyxml/tinyxml.h>

namespace mechanism {

class JointState;

class Joint{
public:
  Joint() {}
  ~Joint() {}

  void enforceLimits(JointState *s);
  bool initXml(TiXmlElement *elt);

  std::string name_;
  int type_;

  // Constants
  double joint_limit_min_;  // In radians
  double joint_limit_max_;  // In radians
  double effort_limit_;
  double velocity_limit_;

  // Axis of motion (x,y,z).  Axis of rotation for revolute/continuous
  // joints, axis of translation for prismatic joints.
  double axis_[3];
};


class JointState
{
public:
  Joint *joint_;

  // State
  double position_;  // In radians
  double velocity_;
  double applied_effort_;

  // Command
  double commanded_effort_;

  JointState() : joint_(NULL), commanded_effort_(0) {}
  JointState(const JointState &s)
    : joint_(s.joint_), position_(s.position_), velocity_(s.velocity_),
      applied_effort_(s.applied_effort_), commanded_effort_(s.commanded_effort_)
  {}
};

enum
{
  JOINT_NONE,
  JOINT_ROTARY,
  JOINT_CONTINUOUS,
  JOINT_PRISMATIC,
  JOINT_FIXED,
  JOINT_PLANAR,
  JOINT_TYPES_MAX
};

}

#endif /* JOINT_H */
