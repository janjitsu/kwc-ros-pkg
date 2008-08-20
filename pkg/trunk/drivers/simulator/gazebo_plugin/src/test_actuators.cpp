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

#include <gazebo_plugin/test_actuators.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <unistd.h>
#include <stl_utils/stl_utils.h>
#include <math_utils/angles.h>
#include <gazebo/Global.hh>
#include <gazebo/XMLConfig.hh>
#include <gazebo/Model.hh>
#include <gazebo/HingeJoint.hh>
#include <gazebo/SliderJoint.hh>
#include <gazebo/Simulator.hh>
#include <gazebo/gazebo.h>
#include <gazebo/GazeboError.hh>
#include <gazebo/ControllerFactory.hh>

// new parser by stu
#include <urdf/parser.h>

namespace gazebo {

  GZ_REGISTER_DYNAMIC_CONTROLLER("test_actuators", TestActuators);

  TestActuators::TestActuators(Entity *parent)
    : Controller(parent) , hw_(0), mc_(&hw_), rmc_(&hw_) , mcn_(&mc_), rmcn_(&rmc_)
  {
     this->parent_model_ = dynamic_cast<Model*>(this->parent);

     if (!this->parent_model_)
        gzthrow("TestActuators controller requires a Model as its parent");

    rosnode_ = ros::g_node; // comes from where?
    int argc = 0;
    char** argv = NULL;
    if (rosnode_ == NULL)
    {
      // this only works for a single camera.
      ros::init(argc,argv);
      rosnode_ = new ros::node("ros_gazebo",ros::node::DONT_HANDLE_SIGINT);
      printf("-------------------- starting node in test actuators \n");
    }
    tfs = new rosTFServer(*rosnode_); //, true, 1 * 1000000000ULL, 0ULL);

    // uses info from wg_robot_description_parser/send.xml
    std::string pr2Content;

    // get pr2.xml for Ioan's parser
    //rosnode_->get_param("robotdesc/pr2",pr2Content);
    // parse the big pr2.xml string from ros, or use below so we don't need to roslaunch send.xml
    //pr2Description.loadString(pr2Content.c_str());

    // using tiny xml
    pr2Doc_ = new TiXmlDocument();
    pr2Doc_->SetUserData(NULL);
    pr2Doc_->Parse(pr2Content.c_str());

    AdvertiseSubscribeMessages();

  }

  TestActuators::~TestActuators()
  {
    //deleteElements(&gazebo_joints_);
  }

  void TestActuators::LoadChild(XMLConfigNode *node)
  {
    // if we were doing some ros publishing...
    //this->topicName = node->GetString("topicName","default_ros_camera",0); //read from xml file
    //this->frameName = node->GetString("frameName","default_ros_camera",0); //read from xml file
    //std::cout << "================= " << this->topicName << std::endl;
    //rosnode_->advertise<std_msgs::Image>(this->topicName);

    //---------------------------------------------------------------------
    // setup mechanism control - non-gazebo stuff,
    // same as implemented on realtime system
    //
    // for the topside, we need to setup:
    //   robot->joints
    //   actuators
    //   transmissions
    // 
    //---------------------------------------------------------------------
    LoadMC(node);

  }

  void TestActuators::InitChild()
  {
    // TODO: mc_.init();

    hw_.current_time_ = Simulator::Instance()->GetSimTime();

    // for internal gripper pid use
    currentTime = Simulator::Instance()->GetSimTime();
    lastTime    = Simulator::Instance()->GetSimTime();
  }

  void TestActuators::UpdateChild()
  {
    //--------------------------------------------------
    //  run the reattime mc update
    //--------------------------------------------------
    UpdateMC();

  }

  void TestActuators::FiniChild()
  {
    // TODO: will be replaced by global ros node eventually
    if (rosnode_ != NULL)
    {
      std::cout << "shutdown rosnode in test_actuators" << std::endl;
      //ros::fini();
      rosnode_->shutdown();
      //delete rosnode_;
    }
  }

  void TestActuators::LoadMC(XMLConfigNode *node)
  {
    //-------------------------------------------------------------------------------------------
    //
    // GET INFORMATION FROM PR2.XML FROM PARAM SERVER
    // AND PUT IT INTO ROBOT_
    //
    // create a set of mech_joint_ for the robot and
    //
    // create a set of reverse_mech_joint_ for the reverse transmission results
    //
    //-------------------------------------------------------------------------------------------

    // parse pr2.xml from filename specified
    pr2Description.loadFile(node->GetString("robot_filename","",1).c_str());

    // get all links in pr2.xml
    pr2Description.getLinks(pr2Links);
    std::cout << " pr2.xml link size: " << pr2Links.size() << std::endl;

    // as the name states
    LoadFrameTransformOffsets();



    //-----------------------------------------------------------------------------------------
    //
    // Read XML's and normalize const and const_blocks
    //
    //-----------------------------------------------------------------------------------------
    TiXmlDocument *pr2_xml = new TiXmlDocument();
    TiXmlDocument *controller_xml = new TiXmlDocument();
    TiXmlDocument *transmission_xml = new TiXmlDocument();
    TiXmlDocument *actuator_xml = new TiXmlDocument();

    std::cout << " robot        file name: " << node->GetString("robot_filename","",1) << std::endl;
    std::cout << " controller   file name: " << node->GetString("controller_filename","",1) << std::endl;
    std::cout << " transmission file name: " << node->GetString("transmission_filename","",1) << std::endl;
    std::cout << " actuator     file name: " << node->GetString("actuator_filename","",1) << std::endl;

    pr2_xml->LoadFile(node->GetString("robot_filename","",1));
    controller_xml->LoadFile(node->GetString("controller_filename","",1));
    transmission_xml->LoadFile(node->GetString("transmission_filename","",1));
    actuator_xml->LoadFile(node->GetString("actuator_filename","",1));

    urdf::normalizeXml( pr2_xml->RootElement() );
    //urdf::normalizeXml( controller_xml->RootElement() );
    //urdf::normalizeXml( transmission_xml->RootElement() );
    //urdf::normalizeXml( actuator_xml->RootElement() );


    //-----------------------------------------------------------------------------------------
    //
    //  parse for MechanismControl joints
    //
    //-----------------------------------------------------------------------------------------
    mcn_.initXml(pr2_xml->FirstChildElement("robot"));
    rmcn_.initXml(pr2_xml->FirstChildElement("robot"));
    //-----------------------------------------------------------------------------------------
    //
    //  how the mechanism joints relate to the gazebo_joints
    //
    //-----------------------------------------------------------------------------------------
    // The gazebo joints and mechanism joints should match up.
    std::cout << " Loading gazebo joints : " <<  std::endl;

    for (XMLConfigNode *jNode = node->GetChild("robot")->GetChild("joint"); jNode; )
    {
      std::string *joint_name = new std::string(jNode->GetString("name","",1));
      std::cout << "processing mech joint (" << *joint_name << ") map to gazebo joint. " << std::endl;

      // joint exist in model, proceed to create gazebo joint and mapping
      Gazebo_joint_* gj = new Gazebo_joint_();
      gj->name_      = joint_name;

      // check for special cases -- Gripper!
      if (!rmc_.model_.getJoint(*joint_name))
      {
        // joint does not exist in MechanismControl
        std::cout << std::endl << "mech joint (" << *joint_name << ") does not exist in mechanism control, check pr2.xml. " << std::endl;
        // create this joint inside MechanismControl if it is an abstract type like a gripper
        if (jNode->GetString("abstract","",0)== "gripper")
        {
           mechanism::Joint *joint = new mechanism::Joint();
           joint->name_ = joint_name->c_str();
           joint->type_ = mechanism::JOINT_ROTARY;
           joint->position_         = 0;     // from transmission
           joint->velocity_         = 0;     // from transmission
           joint->applied_effort_   = 0;     // from transmission
           joint->commanded_effort_ = 0;     // to transmission
           joint->joint_limit_min_  = 0;
           joint->joint_limit_max_  = 0;
           joint->effort_limit_     = jNode->GetDouble("effortLimit",0.0,0);
           joint->velocity_limit_   = jNode->GetDouble("velocityLimit",0.0,0);
           mc_.addJoint(joint);
           rmc_.addJoint(joint);
           // get controller name from xml
           gj->gripper_controller_name_     = new std::string(jNode->GetString("gripper_controller","",1));
           std::cout << " gripper controller name is : " << *(gj->gripper_controller_name_) << std::endl;
        }
      }

      // add a link to the mechanism control joint
      gj->rmc_joint_ = rmc_.model_.getJoint(*joint_name);

      // read gazebo specific joint properties
      gj->saturationTorque           = jNode->GetDouble("saturationTorque",0.0,0);
      gj->explicitDampingCoefficient = jNode->GetDouble("explicitDampingCoefficient",0.0,0);

      // deal with special case -- Gripper!
      if (jNode->GetString("abstract","",0) == "gripper")
        gj->isGripper                = true;
      else
        gj->isGripper                = false;

      if (gj->isGripper)
      {
        std::string f_l_joint_name     = jNode->GetString("left_proximal","",1);
        std::string f_r_joint_name     = jNode->GetString("right_proximal","",1);
        std::string f_tip_l_joint_name = jNode->GetString("left_distal","",1);
        std::string f_tip_r_joint_name = jNode->GetString("right_distal","",1);
        gazebo::HingeJoint* gj_f_l     = (gazebo::HingeJoint*)parent_model_->GetJoint(f_l_joint_name)    ;
        gazebo::HingeJoint* gj_f_r     = (gazebo::HingeJoint*)parent_model_->GetJoint(f_r_joint_name)    ;
        gazebo::HingeJoint* gj_f_tip_l = (gazebo::HingeJoint*)parent_model_->GetJoint(f_tip_l_joint_name);
        gazebo::HingeJoint* gj_f_tip_r = (gazebo::HingeJoint*)parent_model_->GetJoint(f_tip_r_joint_name);
        gj->gaz_joints_.push_back(gj_f_l    );
        gj->gaz_joints_.push_back(gj_f_r    );
        gj->gaz_joints_.push_back(gj_f_tip_l);
        gj->gaz_joints_.push_back(gj_f_tip_r);

        gj->gaz_gripper_pids_.push_back( new controller::Pid() ); gj->gaz_gripper_pids_.back()->initPid( 1.0, 0.01, 0.0, 0.2, -0.2);
        gj->gaz_gripper_pids_.push_back( new controller::Pid() ); gj->gaz_gripper_pids_.back()->initPid( 1.0, 0.01, 0.0, 0.2, -0.2);
        gj->gaz_gripper_pids_.push_back( new controller::Pid() ); gj->gaz_gripper_pids_.back()->initPid( 1.0, 0.01, 0.0, 0.2, -0.2);
        gj->gaz_gripper_pids_.push_back( new controller::Pid() ); gj->gaz_gripper_pids_.back()->initPid( 1.0, 0.01, 0.0, 0.2, -0.2);

        // initialize for torque control mode
        gj_f_l    ->SetParam(dParamVel , 0);
        gj_f_l    ->SetParam(dParamFMax, 0);
        gj_f_r    ->SetParam(dParamVel , 0);
        gj_f_r    ->SetParam(dParamFMax, 0);
        gj_f_tip_l->SetParam(dParamVel , 0);
        gj_f_tip_l->SetParam(dParamFMax, 0);
        gj_f_tip_r->SetParam(dParamVel , 0);
        gj_f_tip_r->SetParam(dParamFMax, 0);

      }
      else
      {
        gazebo::Joint* ggj = (gazebo::Joint*)parent_model_->GetJoint(*joint_name);
        gj->gaz_joints_.push_back(ggj);
        // initialize for torque control mode
        ggj->SetParam(dParamVel , 0);
        ggj->SetParam(dParamFMax, 0);

      }

      gazebo_joints_.push_back(gj);
      jNode = jNode->GetNext("joint");
    }
    //-----------------------------------------------------------------------------------------
    //
    // ACTUATOR XML
    //
    // Pulls out the list of actuators used in the robot configuration.
    //
    //-----------------------------------------------------------------------------------------
    struct GetActuators : public TiXmlVisitor
    {
      std::set<std::string> actuators;
      virtual bool VisitEnter(const TiXmlElement &elt, const TiXmlAttribute *)
      {
        if (elt.ValueStr() == std::string("actuator") && elt.Attribute("name"))
          actuators.insert(elt.Attribute("name"));
        return true;
      }
    } get_actuators;
    actuator_xml->RootElement()->Accept(&get_actuators);

    // Places the found actuators into the hardware interface.
    std::set<std::string>::iterator it;
    for (it = get_actuators.actuators.begin(); it != get_actuators.actuators.end(); ++it)
    {
      std::cout << "adding actuator " << (*it) << std::endl;
      hw_.actuators_.push_back(new Actuator(*it));
    }

    //-----------------------------------------------------------------------------------------
    //
    // TRANSMISSION XML
    //
    // make mc parse xml for transmissions
    //
    //-----------------------------------------------------------------------------------------
    mcn_.initXml(transmission_xml->FirstChildElement("robot"));
    rmcn_.initXml(transmission_xml->FirstChildElement("robot"));

    //-----------------------------------------------------------------------------------------
    //
    // CONTROLLER XML
    //
    //  spawn controllers
    //
    //-----------------------------------------------------------------------------------------
    // make mc parse xml for controllers
    std::cout << " Loading controllers : " <<  std::endl;
    for (TiXmlElement *xit = controller_xml->FirstChildElement("robot"); xit ; xit = xit->NextSiblingElement("robot") )
    for (TiXmlElement *zit = xit->FirstChildElement("controller"); zit ; zit = zit->NextSiblingElement("controller") )
    {
      std::string* controller_name = new std::string(zit->Attribute("name"));
      std::string* controller_type = new std::string(zit->Attribute("type"));
      std::cout << " LoadChild controller name: " <<  *controller_name << " type " << *controller_type << std::endl;

      // initialize controller
      std::cout << " adding to mc_ " ;
      mc_.spawnController(*controller_type,
                          *controller_name,
                          zit);

      std::cout << " adding to rmc_ " ;
      rmc_.spawnController(*controller_type,
                           *controller_name,
                           zit);

    }

  }


  //-------------------------------------------------------------------------------------------//
  //                                                                                           //
  //                                                                                           //
  //                                                                                           //
  //  MAIN UPDATE LOOP FOR MC AND ROS PUBLISH                                                  //
  //                                                                                           //
  //                                                                                           //
  //                                                                                           //
  //-------------------------------------------------------------------------------------------//
  void TestActuators::UpdateMC()
  {
    // pass time to robot
    hw_.current_time_ = Simulator::Instance()->GetSimTime();
    currentTime = Simulator::Instance()->GetSimTime();


    this->lock.lock();
    /***************************************************************/
    /*                                                             */
    /*  publish time to ros                                        */
    /*                                                             */
    /***************************************************************/
    timeMsg.rostime.sec  = (unsigned long)floor(hw_.current_time_);
    timeMsg.rostime.nsec = (unsigned long)floor(  1e9 * (  hw_.current_time_ - timeMsg.rostime.sec) );
    rosnode_->publish("time",timeMsg);
    /***************************************************************/
    /*                                                             */
    /*  odometry                                                   */
    /*                                                             */
    /***************************************************************/
    // Get latest odometry data
    // Get velocities
    //double vx,vy,vw;
    //this->PR2Copy->GetBaseCartesianSpeedActual(&vx,&vy,&vw);
    // Translate into ROS message format and publish
    //this->odomMsg.vel.x  = vx;
    //this->odomMsg.vel.y  = vy;
    //this->odomMsg.vel.th = vw;

    // Get position
    //double x,y,z,roll,pitch,yaw;
    //this->PR2Copy->GetBasePositionActual(&x,&y,&z,&roll,&pitch,&yaw);
    //this->odomMsg.pos.x  = x;
    //this->odomMsg.pos.y  = y;
    //this->odomMsg.pos.th = yaw;

    // TODO: get the frame ID from somewhere
    this->odomMsg.header.frame_id = "FRAMEID_ODOM";
    this->odomMsg.header.stamp.sec = (unsigned long)floor(hw_.current_time_);
    this->odomMsg.header.stamp.nsec = (unsigned long)floor(  1e9 * (  hw_.current_time_ - this->odomMsg.header.stamp.sec) );

    // This publish call resets odomMsg.header.stamp.sec and 
    // odomMsg.header.stamp.nsec to zero.  Thus, it must be called *after*
    // those values are reused in the sendInverseEuler() call above.
    //rosnode_->publish("odom",this->odomMsg);

    /***************************************************************/
    /*                                                             */
    /*   object position                                           */
    /*                                                             */
    /***************************************************************/
    //this->PR2Copy->GetObjectPositionActual(&x,&y,&z,&roll,&pitch,&yaw);
    //this->objectPosMsg.x  = x;
    //this->objectPosMsg.y  = y;
    //this->objectPosMsg.z  = z;
    //rosnode_->publish("object_position", this->objectPosMsg);



    std_msgs::PR2Arm larm,rarm;
    /* get left arm position */
    larm.turretAngle       = mc_.model_.getJoint("shoulder_pan_left_joint")->position_;
    larm.shoulderLiftAngle = mc_.model_.getJoint("shoulder_pitch_left_joint")->position_;
    larm.upperarmRollAngle = mc_.model_.getJoint("upperarm_roll_left_joint")->position_;
    larm.elbowAngle        = mc_.model_.getJoint("elbow_flex_left_joint")->position_;
    larm.forearmRollAngle  = mc_.model_.getJoint("forearm_roll_left_joint")->position_;
    larm.wristPitchAngle   = mc_.model_.getJoint("wrist_flex_left_joint")->position_;
    larm.wristRollAngle    = mc_.model_.getJoint("gripper_roll_left_joint")->position_;
    //larm.gripperForceCmd   = mc_.model_.getJoint("gripper_left_joint")->applied_effort_;
    //larm.gripperGapCmd     = mc_.model_.getJoint("gripper_left_joint")->position_;
    rosnode_->publish("left_pr2arm_pos", larm);
    /* get right arm position */
    rarm.turretAngle       = mc_.model_.getJoint("shoulder_pan_right_joint")->position_;
    rarm.shoulderLiftAngle = mc_.model_.getJoint("shoulder_pitch_right_joint")->position_;
    rarm.upperarmRollAngle = mc_.model_.getJoint("upperarm_roll_right_joint")->position_;
    rarm.elbowAngle        = mc_.model_.getJoint("elbow_flex_right_joint")->position_;
    rarm.forearmRollAngle  = mc_.model_.getJoint("forearm_roll_right_joint")->position_;
    rarm.wristPitchAngle   = mc_.model_.getJoint("wrist_flex_right_joint")->position_;
    rarm.wristRollAngle    = mc_.model_.getJoint("gripper_roll_right_joint")->position_;
    //rarm.gripperForceCmd   = mc_.model_.getJoint("gripper_right_joint")->applied_effort_;
    //rarm.gripperGapCmd     = mc_.model_.getJoint("gripper_right_joint")->position_;
    rosnode_->publish("right_pr2arm_pos", rarm);

    //PublishFrameTransforms();

    this->lock.unlock();

    //---------------------------------------------------------------------
    // Real time update calls to mechanism control
    // this is what the hard real time loop does,
    // minus the tick() call to etherCAT
    //---------------------------------------------------------------------
    //
    // step through all controllers in the Robot_controller

    // update joint status from hardware
    for (std::vector<Gazebo_joint_*>::iterator gji = gazebo_joints_.begin(); gji != gazebo_joints_.end() ; gji++)
    {
      // gripper joint, is an ugly special case for now
      if ((*gji)->isGripper)
      {
        for (std::vector<gazebo::Joint*>::iterator ggji = (*gji)->gaz_joints_.begin(); ggji != (*gji)->gaz_joints_.end() ; ggji++)
        {

        }
        gazebo::HingeJoint* gj_f_l     = dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[0]);
        gazebo::HingeJoint* gj_f_r     = dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[1]);
        gazebo::HingeJoint* gj_f_tip_l = dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[2]);
        gazebo::HingeJoint* gj_f_tip_r = dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[3]);

        (*gji)->rmc_joint_->position_       = gj_f_l->GetAngle();
        (*gji)->rmc_joint_->velocity_       = gj_f_l->GetAngleRate();
        (*gji)->rmc_joint_->applied_effort_ = (*gji)->rmc_joint_->commanded_effort_;

      }
      else
      {
        // normal joints
        switch((*gji)->gaz_joints_[0]->GetType())
        {
          case gazebo::Joint::SLIDER:
          {
            gazebo::SliderJoint* gjs  = dynamic_cast<gazebo::SliderJoint*>((*gji)->gaz_joints_[0]);
            (*gji)->rmc_joint_->position_       = gjs->GetPosition();
            (*gji)->rmc_joint_->velocity_       = gjs->GetPositionRate();
            (*gji)->rmc_joint_->applied_effort_ = (*gji)->rmc_joint_->commanded_effort_;
            break;
          }
          case gazebo::Joint::HINGE:
          {
            gazebo::HingeJoint* gjh  = dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[0]);
            (*gji)->rmc_joint_->position_       = gjh->GetAngle();
            (*gji)->rmc_joint_->velocity_       = gjh->GetAngleRate();
            (*gji)->rmc_joint_->applied_effort_ = (*gji)->rmc_joint_->commanded_effort_;
            break;
          }
          case gazebo::Joint::HINGE2:
          case gazebo::Joint::BALL:
          case gazebo::Joint::UNIVERSAL:
            break;
        }
      }

    }
    // push reverse_mech_joint_ stuff back toward actuators
    for (unsigned int i=0; i < rmc_.model_.transmissions_.size(); i++)
    {
      rmc_.model_.transmissions_[i]->propagatePositionBackwards();
      rmc_.model_.transmissions_[i]->propagateEffort();
      // std::cout << " applying reverse transmisison : "
      //           <<  dynamic_cast<mechanism::SimpleTransmission*>(rmc_.model_.transmissions_[i])->name_
      //           << " " <<  std::endl;
    }


    // -------------------------------------------------------------------------------------------------
    // -                                                                                               -
    // -   test some controllers set points by hardcode for debug                                      -
    // -                                                                                               -
    // -------------------------------------------------------------------------------------------------
    // set through ros?
    // artifically set command
    // controller::Controller* mcc = mc_.getControllerByName( "shoulder_pitch_right_controller" );
    // dynamic_cast<controller::JointPositionController*>(mcc)->setCommand(-0.2);
    // // sample read back angle
    // controller::Controller* mc2 = mc_.getControllerByName( "shoulder_pitch_left_controller" );
    // std::cout << " angle = " << dynamic_cast<controller::JointPositionController*>(mcc)->getActual() << std::endl;

    // controller::Controller* mc5 = mc_.getControllerByName( "shoulder_pitch_left_controller" );
    // dynamic_cast<controller::JointPositionController*>(mc5)->setCommand(-0.5);
    // controller::Controller* mc3 = mc_.getControllerByName( "gripper_left_controller" );
    // dynamic_cast<controller::JointPositionController*>(mc3)->setCommand(0.2);

    // libTF::Pose3D::Vector cmd_vel;
    // cmd_vel.x = 1.0;
    // cmd_vel.y = 0.1;
    // cmd_vel.z = 0.1;
    // controller::Controller* bc = mc_.getControllerByName( "base_controller" );
    // dynamic_cast<controller::BaseController*>(bc)->setCommand(cmd_vel);

    // -------------------------------------------------------------------------------------------------
    // -                                                                                               -
    // -  update each controller, this updates the joint that the controller was initialized with      -
    // -                                                                                               -
    // -  update mc given the actuator states are filled from above                                    -
    // -                                                                                               -
    // -  update actuators from robot joints via forward transmission propagation                      -
    // -                                                                                               -
    // -------------------------------------------------------------------------------------------------
    mcn_.update();
    //mc_.update();


    //============================================================================================
    // below is when the actuator stuff goes to the hardware
    //============================================================================================

    // -------------------------------------------------------------------------------------------------
    // -                                                                                               -
    // -    reverse transmission, get joint data from actuators                                        -
    // -                                                                                               -
    // -------------------------------------------------------------------------------------------------
    // propagate actuator data back to reverse-joints
    for (unsigned int i=0; i < rmc_.model_.transmissions_.size(); i++)
    {
      // assign reverse joint states from actuator states
      rmc_.model_.transmissions_[i]->propagatePosition();
      // assign joint effort
      rmc_.model_.transmissions_[i]->propagateEffortBackwards();
    }

    // -------------------------------------------------------------------------------------------------
    // -                                                                                               -
    // -     udpate gazebo joint for this controller joint                                             -
    // -                                                                                               -
    // -------------------------------------------------------------------------------------------------
    for (std::vector<Gazebo_joint_*>::iterator gji = gazebo_joints_.begin(); gji != gazebo_joints_.end() ; gji++)
    {
      // gripper joint, is an ugly special case for now
      if ((*gji)->isGripper)
      {
        double gripperCmd  , currentError, currentCmd  ;
        // FIXME: this restricts gripper to a position controller... not ideal
        std::string  jn = *((*gji)->gripper_controller_name_ );
        controller::Controller* jc = mc_.getControllerByName(jn); // from actual mechanism control, not rmc_
        controller::JointPositionControllerNode* jpc = dynamic_cast<controller::JointPositionControllerNode*>(jc);
        gripperCmd   = jpc->getCommand();

        currentError = math_utils::shortest_angular_distance(dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[0])->GetAngle(),gripperCmd);
        currentCmd   = (*gji)->gaz_gripper_pids_[0]->updatePid(currentError,currentTime-lastTime);
        dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[0])->SetParam( dParamVel, currentCmd );
        dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[0])->SetParam( dParamFMax, (*gji)->saturationTorque );

        currentError = math_utils::shortest_angular_distance(dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[1])->GetAngle(),-gripperCmd);
        currentCmd   = (*gji)->gaz_gripper_pids_[1]->updatePid(currentError,currentTime-lastTime);
        dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[1])->SetParam( dParamVel, currentCmd );
        dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[1])->SetParam( dParamFMax, (*gji)->saturationTorque );

        currentError = math_utils::shortest_angular_distance(dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[2])->GetAngle(),-gripperCmd);
        currentCmd   = (*gji)->gaz_gripper_pids_[2]->updatePid(currentError,currentTime-lastTime);
        dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[2])->SetParam( dParamVel, currentCmd );
        dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[2])->SetParam( dParamFMax, (*gji)->saturationTorque );

        currentError = math_utils::shortest_angular_distance(dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[3])->GetAngle(),gripperCmd);
        currentCmd   = (*gji)->gaz_gripper_pids_[3]->updatePid(currentError,currentTime-lastTime);
        dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[3])->SetParam( dParamVel, currentCmd );
        dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[3])->SetParam( dParamFMax, (*gji)->saturationTorque );

      }
      else
      {
        // normal joints
        switch ((*gji)->gaz_joints_[0]->GetType())
        {
          case gazebo::Joint::SLIDER:
          {
            gazebo::SliderJoint* gjs  = dynamic_cast<gazebo::SliderJoint*>((*gji)->gaz_joints_[0]);
            gjs->SetSliderForce( (*gji)->rmc_joint_->commanded_effort_);
            break;
          }
          case gazebo::Joint::HINGE:
          {
            gazebo::HingeJoint* gjh  = dynamic_cast<gazebo::HingeJoint*>((*gji)->gaz_joints_[0]);
            gjh->SetTorque( (*gji)->rmc_joint_->commanded_effort_);
            break;
          }
          case gazebo::Joint::HINGE2:
          case gazebo::Joint::BALL:
          case gazebo::Joint::UNIVERSAL:
            break;
        }
      }

    }

    lastTime = currentTime;

  }








  void
  TestActuators::LoadFrameTransformOffsets()
  {
    // get all links in pr2.xml
    robot_desc::URDF::Link* link;

    // get all the parameters needed for frame transforms
    link = pr2Description.getLink("base");
    base_center_offset_z = link->collision->xyz[2];
    link = pr2Description.getLink("torso");
    base_torso_offset_x  = link->xyz[0];
    base_torso_offset_y  = link->xyz[1];
    base_torso_offset_z  = link->xyz[2];
    link = pr2Description.getLink("shoulder_pan_left");
    sh_pan_left_torso_offset_x =  link->xyz[0];
    sh_pan_left_torso_offset_y =  link->xyz[1];
    sh_pan_left_torso_offset_z =  link->xyz[2];
    link = pr2Description.getLink("shoulder_pitch_left");
    shoulder_pitch_left_offset_x = link->xyz[0];
    shoulder_pitch_left_offset_y = link->xyz[1];
    shoulder_pitch_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("upperarm_roll_left");
    upperarm_roll_left_offset_x = link->xyz[0];
    upperarm_roll_left_offset_y = link->xyz[1];
    upperarm_roll_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("elbow_flex_left");
    elbow_flex_left_offset_x = link->xyz[0];
    elbow_flex_left_offset_y = link->xyz[1];
    elbow_flex_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("finger_l_left");
    finger_l_left_offset_x = link->xyz[0];
    finger_l_left_offset_y = link->xyz[1];
    finger_l_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("forearm_roll_left");
    forearm_roll_left_offset_x = link->xyz[0];
    forearm_roll_left_offset_y = link->xyz[1];
    forearm_roll_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("wrist_flex_left");
    wrist_flex_left_offset_x = link->xyz[0];
    wrist_flex_left_offset_y = link->xyz[1];
    wrist_flex_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("gripper_roll_left");
    gripper_roll_left_offset_x = link->xyz[0];
    gripper_roll_left_offset_y = link->xyz[1];
    gripper_roll_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("finger_r_left");
    finger_r_left_offset_x = link->xyz[0];
    finger_r_left_offset_y = link->xyz[1];
    finger_r_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("finger_tip_l_left");
    finger_tip_l_left_offset_x = link->xyz[0];
    finger_tip_l_left_offset_y = link->xyz[1];
    finger_tip_l_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("finger_tip_r_left");
    finger_tip_r_left_offset_x = link->xyz[0];
    finger_tip_r_left_offset_y = link->xyz[1];
    finger_tip_r_left_offset_z = link->xyz[2];


    link = pr2Description.getLink("shoulder_pan_right");
    shoulder_pan_right_offset_x = link->xyz[0];
    shoulder_pan_right_offset_y = link->xyz[1];
    shoulder_pan_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("shoulder_pitch_right");
    shoulder_pitch_right_offset_x = link->xyz[0];
    shoulder_pitch_right_offset_y = link->xyz[1];
    shoulder_pitch_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("upperarm_roll_right");
    upperarm_roll_right_offset_x = link->xyz[0];
    upperarm_roll_right_offset_y = link->xyz[1];
    upperarm_roll_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("elbow_flex_right");
    elbow_flex_right_offset_x = link->xyz[0];
    elbow_flex_right_offset_y = link->xyz[1];
    elbow_flex_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("forearm_roll_right");
    forearm_roll_right_offset_x = link->xyz[0];
    forearm_roll_right_offset_y = link->xyz[1];
    forearm_roll_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("wrist_flex_right");
    wrist_flex_right_offset_x = link->xyz[0];
    wrist_flex_right_offset_y = link->xyz[1];
    wrist_flex_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("gripper_roll_right");
    gripper_roll_right_offset_x = link->xyz[0];
    gripper_roll_right_offset_y = link->xyz[1];
    gripper_roll_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("finger_l_right");
    finger_l_right_offset_x = link->xyz[0];
    finger_l_right_offset_y = link->xyz[1];
    finger_l_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("finger_r_right");
    finger_r_right_offset_x = link->xyz[0];
    finger_r_right_offset_y = link->xyz[1];
    finger_r_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("finger_tip_l_right");
    finger_tip_l_right_offset_x = link->xyz[0];
    finger_tip_l_right_offset_y = link->xyz[1];
    finger_tip_l_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("finger_tip_r_right");
    finger_tip_r_right_offset_x = link->xyz[0];
    finger_tip_r_right_offset_y = link->xyz[1];
    finger_tip_r_right_offset_z = link->xyz[2];

    link = pr2Description.getLink("forearm_camera_left");
    forearm_camera_left_offset_x = link->xyz[0];
    forearm_camera_left_offset_y = link->xyz[1];
    forearm_camera_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("forearm_camera_right");
    forearm_camera_right_offset_x = link->xyz[0];
    forearm_camera_right_offset_y = link->xyz[1];
    forearm_camera_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("wrist_camera_left");
    wrist_camera_left_offset_x = link->xyz[0];
    wrist_camera_left_offset_y = link->xyz[1];
    wrist_camera_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("wrist_camera_right");
    wrist_camera_right_offset_x = link->xyz[0];
    wrist_camera_right_offset_y = link->xyz[1];
    wrist_camera_right_offset_z = link->xyz[2];


    link = pr2Description.getLink("head_pan");
    head_pan_offset_x = link->xyz[0];
    head_pan_offset_y = link->xyz[1];
    head_pan_offset_z = link->xyz[2];
    link = pr2Description.getLink("head_tilt");
    head_tilt_offset_x = link->xyz[0];
    head_tilt_offset_y = link->xyz[1];
    head_tilt_offset_z = link->xyz[2];
    link = pr2Description.getLink("base_laser");
    base_laser_offset_x = link->xyz[0];
    base_laser_offset_y = link->xyz[1];
    base_laser_offset_z = link->xyz[2];
    link = pr2Description.getLink("tilt_laser");
    tilt_laser_offset_x = link->xyz[0];
    tilt_laser_offset_y = link->xyz[1];
    tilt_laser_offset_z = link->xyz[2],
    link = pr2Description.getLink("caster_front_left");
    caster_front_left_offset_x = link->xyz[0];
    caster_front_left_offset_y = link->xyz[1];
    caster_front_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("wheel_front_left_l");
    wheel_front_left_l_offset_x = link->xyz[0];
    wheel_front_left_l_offset_y = link->xyz[1];
    wheel_front_left_l_offset_z = link->xyz[2];
    link = pr2Description.getLink("wheel_front_left_r");
    wheel_front_left_r_offset_x = link->xyz[0];
    wheel_front_left_r_offset_y = link->xyz[1];
    wheel_front_left_r_offset_z = link->xyz[2];
    link = pr2Description.getLink("caster_front_right");
    caster_front_right_offset_x = link->xyz[0];
    caster_front_right_offset_y = link->xyz[1];
    caster_front_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("wheel_front_right_l");
    wheel_front_right_l_offset_x = link->xyz[0];
    wheel_front_right_l_offset_y = link->xyz[1];
    wheel_front_right_l_offset_z = link->xyz[2];
    link = pr2Description.getLink("wheel_front_right_r");
    wheel_front_right_r_offset_x = link->xyz[0];
    wheel_front_right_r_offset_y = link->xyz[1];
    wheel_front_right_r_offset_z = link->xyz[2];
    link = pr2Description.getLink("caster_rear_left");
    caster_rear_left_offset_x = link->xyz[0];
    caster_rear_left_offset_y = link->xyz[1];
    caster_rear_left_offset_z = link->xyz[2];
    link = pr2Description.getLink("wheel_rear_left_l");
    wheel_rear_left_l_offset_x = link->xyz[0];
    wheel_rear_left_l_offset_y = link->xyz[1];
    wheel_rear_left_l_offset_z = link->xyz[2];
    link = pr2Description.getLink("wheel_rear_left_r");
    wheel_rear_left_r_offset_x = link->xyz[0];
    wheel_rear_left_r_offset_y = link->xyz[1];
    wheel_rear_left_r_offset_z = link->xyz[2];
    link = pr2Description.getLink("caster_rear_right");
    caster_rear_right_offset_x = link->xyz[0];
    caster_rear_right_offset_y = link->xyz[1];
    caster_rear_right_offset_z = link->xyz[2];
    link = pr2Description.getLink("wheel_rear_right_l");
    wheel_rear_right_l_offset_x = link->xyz[0];
    wheel_rear_right_l_offset_y = link->xyz[1];
    wheel_rear_right_l_offset_z = link->xyz[2];
    link = pr2Description.getLink("wheel_rear_right_r");
    wheel_rear_right_r_offset_x = link->xyz[0];
    wheel_rear_right_r_offset_y = link->xyz[1];
    wheel_rear_right_r_offset_z = link->xyz[2];

  }




  int
  TestActuators::AdvertiseSubscribeMessages()
  {
    //rosnode_->advertise<std_msgs::RobotBase2DOdom>("odom");
    rosnode_->advertise<std_msgs::PR2Arm>("left_pr2arm_pos");
    rosnode_->advertise<std_msgs::PR2Arm>("right_pr2arm_pos");
    rosnode_->advertise<rostools::Time>("time");
    rosnode_->advertise<std_msgs::Empty>("transform");
    //rosnode_->advertise<std_msgs::Point3DFloat32>("object_position");

    //rosnode_->subscribe("cmd_vel", velMsg, &TestActuators::CmdBaseVelReceived);
    rosnode_->subscribe("cmd_leftarmconfig", leftarmMsg, &TestActuators::CmdLeftarmconfigReceived);
    rosnode_->subscribe("cmd_rightarmconfig", rightarmMsg, &TestActuators::CmdRightarmconfigReceived);
    //rosnode_->subscribe("cmd_leftarm_cartesian", leftarmcartesianMsg, &TestActuators::CmdLeftarmcartesianReceived);
    //rosnode_->subscribe("cmd_rightarm_cartesian", rightarmcartesianMsg, &TestActuators::CmdRightarmcartesianReceived);
    
    return(0);
  }

  void
  TestActuators::CmdLeftarmconfigReceived()
  {
    this->lock.lock();
    printf("hoo!\n");
    controller::Controller* j1 = mc_.getControllerByName( "shoulder_pan_left_controller" );
    controller::Controller* j2 = mc_.getControllerByName( "shoulder_pitch_left_controller" );
    controller::Controller* j3 = mc_.getControllerByName( "upperarm_roll_left_controller" );
    controller::Controller* j4 = mc_.getControllerByName( "elbow_flex_left_controller" );
    controller::Controller* j5 = mc_.getControllerByName( "forearm_roll_left_controller" );
    controller::Controller* j6 = mc_.getControllerByName( "wrist_flex_left_controller" );
    controller::Controller* j7 = mc_.getControllerByName( "gripper_roll_left_controller" );
    controller::Controller* j8 = mc_.getControllerByName( "gripper_left_controller" );
    dynamic_cast<controller::JointPositionController*>(j1)->setCommand(leftarmMsg.turretAngle       );
    dynamic_cast<controller::JointPositionController*>(j2)->setCommand(leftarmMsg.shoulderLiftAngle );
    dynamic_cast<controller::JointPositionController*>(j3)->setCommand(leftarmMsg.upperarmRollAngle );
    dynamic_cast<controller::JointPositionController*>(j4)->setCommand(leftarmMsg.elbowAngle        );
    dynamic_cast<controller::JointPositionController*>(j5)->setCommand(leftarmMsg.forearmRollAngle  );
    dynamic_cast<controller::JointPositionController*>(j6)->setCommand(leftarmMsg.wristPitchAngle   );
    dynamic_cast<controller::JointPositionController*>(j7)->setCommand(leftarmMsg.wristRollAngle    );
    dynamic_cast<controller::JointPositionController*>(j8)->setCommand(leftarmMsg.gripperGapCmd     );
    this->lock.unlock();
  }
  void
  TestActuators::CmdRightarmconfigReceived()
  {
    this->lock.lock();
    printf("hoo!\n");
    controller::Controller* j1 = mc_.getControllerByName( "shoulder_pan_right_controller" );
    controller::Controller* j2 = mc_.getControllerByName( "shoulder_pitch_right_controller" );
    controller::Controller* j3 = mc_.getControllerByName( "upperarm_roll_right_controller" );
    controller::Controller* j4 = mc_.getControllerByName( "elbow_flex_right_controller" );
    controller::Controller* j5 = mc_.getControllerByName( "forearm_roll_right_controller" );
    controller::Controller* j6 = mc_.getControllerByName( "wrist_flex_right_controller" );
    controller::Controller* j7 = mc_.getControllerByName( "gripper_roll_right_controller" );
    controller::Controller* j8 = mc_.getControllerByName( "gripper_right_controller" );
    dynamic_cast<controller::JointPositionController*>(j1)->setCommand(rightarmMsg.turretAngle       );
    dynamic_cast<controller::JointPositionController*>(j2)->setCommand(rightarmMsg.shoulderLiftAngle );
    dynamic_cast<controller::JointPositionController*>(j3)->setCommand(rightarmMsg.upperarmRollAngle );
    dynamic_cast<controller::JointPositionController*>(j4)->setCommand(rightarmMsg.elbowAngle        );
    dynamic_cast<controller::JointPositionController*>(j5)->setCommand(rightarmMsg.forearmRollAngle  );
    dynamic_cast<controller::JointPositionController*>(j6)->setCommand(rightarmMsg.wristPitchAngle   );
    dynamic_cast<controller::JointPositionController*>(j7)->setCommand(rightarmMsg.wristRollAngle    );
    dynamic_cast<controller::JointPositionController*>(j8)->setCommand(rightarmMsg.gripperGapCmd     );
    this->lock.unlock();
  }

  void
  TestActuators::CmdLeftarmcartesianReceived()
  {
    this->lock.lock();
    this->lock.unlock();
  }
  void
  TestActuators::CmdRightarmcartesianReceived()
  {
    this->lock.lock();
    this->lock.unlock();
  }


  void
  TestActuators::CmdBaseVelReceived()
  {
    this->lock.lock();
    this->lock.unlock();
  }


  void
  TestActuators::PublishFrameTransforms()
  {

    /***************************************************************/
    /*                                                             */
    /*  frame transforms                                           */
    /*                                                             */
    /*  TODO: should we send z, roll, pitch, yaw? seems to confuse */
    /*        localization                                         */
    /*                                                             */
    /***************************************************************/
    double x=0,y=0,z=0,roll=0,pitch=0,yaw=0;
    //this->PR2Copy->GetBasePositionActual(&x,&y,&z,&roll,&pitch,&yaw); // actual CoM of base

    tfs->sendInverseEuler("FRAMEID_ODOM",
                 "base",
                 x,
                 y,
                 z - base_center_offset_z, /* get infor from xml: half height of base box */
                 yaw,
                 pitch,
                 roll,
                 odomMsg.header.stamp);

    /***************************************************************/
    /*                                                             */
    /*  frame transforms                                           */
    /*                                                             */
    /*  x,y,z,yaw,pitch,roll                                       */
    /*                                                             */
    /***************************************************************/
    tfs->sendEuler("base",
                 "FRAMEID_ROBOT",
                 0,
                 0,
                 0, 
                 0,
                 0,
                 0,
                 odomMsg.header.stamp);

    //std::cout << "base y p r " << yaw << " " << pitch << " " << roll << std::endl;

    // base = center of the bottom of the base box
    // torso = midpoint of bottom of turrets

    tfs->sendEuler("torso",
                 "base",
                 base_torso_offset_x,
                 base_torso_offset_y,
                 base_torso_offset_z, /* FIXME: spine elevator not accounted for */
                 0.0,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // arm_l_turret = bottom of left turret
    tfs->sendEuler("shoulder_pan_left",
                 "torso",
                 sh_pan_left_torso_offset_x,
                 sh_pan_left_torso_offset_y,
                 sh_pan_left_torso_offset_z,
                 0.0, //larm.turretAngle,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);
    //std::cout << "left pan angle " << larm.turretAngle << std::endl;

    // arm_l_shoulder = center of left shoulder pitch bracket
    tfs->sendEuler("shoulder_pitch_left",
                 "shoulder_pan_left",
                 shoulder_pitch_left_offset_x,
                 shoulder_pitch_left_offset_y,
                 shoulder_pitch_left_offset_z,
                 0.0,
                 0.0, //larm.shoulderLiftAngle,
                 0.0,
                 odomMsg.header.stamp);

    // arm_l_upperarm = upper arm with roll DOF, at shoulder pitch center
    tfs->sendEuler("upperarm_roll_left",
                 "shoulder_pitch_left",
                 upperarm_roll_left_offset_x,
                 upperarm_roll_left_offset_y,
                 upperarm_roll_left_offset_z,
                 0.0,
                 0.0,
                 0.0, //larm.upperarmRollAngle,
                 odomMsg.header.stamp);

    //frameid_arm_l_elbow = elbow pitch bracket center of rotation
    tfs->sendEuler("elbow_flex_left",
                 "upperarm_roll_left",
                 elbow_flex_left_offset_x,
                 elbow_flex_left_offset_y,
                 elbow_flex_left_offset_z,
                 0.0,
                 0.0, //larm.elbowAngle,
                 0.0,
                 odomMsg.header.stamp);

    //frameid_arm_l_forearm = forearm roll DOR, at elbow pitch center
    tfs->sendEuler("forearm_roll_left",
                 "elbow_flex_left",
                 forearm_roll_left_offset_x,
                 forearm_roll_left_offset_y,
                 forearm_roll_left_offset_z,
                 0.0,
                 0.0,
                 0.0, //larm.forearmRollAngle,
                 odomMsg.header.stamp);

    // arm_l_wrist = wrist pitch DOF.
    tfs->sendEuler("wrist_flex_left",
                 "forearm_roll_left",
                 wrist_flex_left_offset_x,
                 wrist_flex_left_offset_y,
                 wrist_flex_left_offset_z,
                 0.0,
                 0.0, //larm.wristPitchAngle,
                 0.0,
                 odomMsg.header.stamp);

    // arm_l_hand = hand roll DOF, center at wrist pitch center
    tfs->sendEuler("gripper_roll_left",
                 "wrist_flex_left",
                 gripper_roll_left_offset_x,
                 gripper_roll_left_offset_y,
                 gripper_roll_left_offset_z,
                 0.0,
                 0.0,
                 0.0, //larm.wristRollAngle,
                 odomMsg.header.stamp);

    // proximal digit, left
    tfs->sendEuler("finger_l_left",
                 "gripper_roll_left",
                 finger_l_left_offset_x,
                 finger_l_left_offset_y,
                 finger_l_left_offset_z,
                 0.0,  //FIXME: get angle of finger...
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // proximal digit, right
    tfs->sendEuler("finger_r_left",
                 "gripper_roll_left",
                 finger_r_left_offset_x,
                 finger_r_left_offset_y,
                 finger_r_left_offset_z,
                 0.0,  //FIXME: get angle of finger...
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // distal digit, left
    tfs->sendEuler("finger_tip_l_left",
                 "finger_l_left",
                 finger_tip_l_left_offset_x,
                 finger_tip_l_left_offset_y,
                 finger_tip_l_left_offset_z,
                 0.0,  //FIXME: get angle of finger tip...
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // distal digit, left
    tfs->sendEuler("finger_tip_r_left",
                 "finger_r_left",
                 finger_tip_r_left_offset_x,
                 finger_tip_r_left_offset_y,
                 finger_tip_r_left_offset_z,
                 0.0,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);




    // arm_r_turret = bottom of right turret
    tfs->sendEuler("shoulder_pan_right",
                 "torso",
                 shoulder_pan_right_offset_x,
                 shoulder_pan_right_offset_y,
                 shoulder_pan_right_offset_z,
                 0.0, //rarm.turretAngle,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);
    //std::cout << "right pan angle " << larm.turretAngle << std::endl;

    // arm_r_shoulder = center of right shoulder pitch bracket
    tfs->sendEuler("shoulder_pitch_right",
                 "shoulder_pan_right",
                 shoulder_pitch_right_offset_x,
                 shoulder_pitch_right_offset_y,
                 shoulder_pitch_right_offset_z,
                 0.0,
                 0.0, //rarm.shoulderLiftAngle,
                 0.0,
                 odomMsg.header.stamp);

    // arm_r_upperarm = upper arm with roll DOF, at shoulder pitch center
    tfs->sendEuler("upperarm_roll_right",
                 "shoulder_pitch_right",
                 upperarm_roll_right_offset_x,
                 upperarm_roll_right_offset_y,
                 upperarm_roll_right_offset_z,
                 0.0,
                 0.0,
                 0.0, //rarm.upperarmRollAngle,
                 odomMsg.header.stamp);

    //frameid_arm_r_elbow = elbow pitch bracket center of rotation
    tfs->sendEuler("elbow_flex_right",
                 "upperarm_roll_right",
                 elbow_flex_right_offset_x,
                 elbow_flex_right_offset_y,
                 elbow_flex_right_offset_z,
                 0.0,
                 0.0, //rarm.elbowAngle,
                 0.0,
                 odomMsg.header.stamp);

    //frameid_arm_r_forearm = forearm roll DOR, at elbow pitch center
    tfs->sendEuler("forearm_roll_right",
                 "elbow_flex_right",
                 forearm_roll_right_offset_x,
                 forearm_roll_right_offset_y,
                 forearm_roll_right_offset_z,
                 0.0,
                 0.0,
                 0.0, //rarm.forearmRollAngle,
                 odomMsg.header.stamp);

    // arm_r_wrist = wrist pitch DOF.
    tfs->sendEuler("wrist_flex_right",
                 "forearm_roll_right",
                 wrist_flex_right_offset_x,
                 wrist_flex_right_offset_y,
                 wrist_flex_right_offset_z,
                 0.0,
                 0.0, //rarm.wristPitchAngle,
                 0.0,
                 odomMsg.header.stamp);

    // arm_r_hand = hand roll DOF, center at wrist pitch center
    tfs->sendEuler("gripper_roll_right",
                 "wrist_flex_right",
                 gripper_roll_right_offset_x,
                 gripper_roll_right_offset_y,
                 gripper_roll_right_offset_z,
                 0.0,
                 0.0,
                 0.0, //rarm.wristRollAngle,
                 odomMsg.header.stamp);

    // proximal digit, right
    tfs->sendEuler("finger_l_right",
                 "gripper_roll_right",
                 finger_l_right_offset_x,
                 finger_l_right_offset_y,
                 finger_l_right_offset_z,
                 0.0,  //FIXME: get angle of finger...
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // proximal digit, right
    tfs->sendEuler("finger_r_right",
                 "gripper_roll_right",
                 finger_r_right_offset_x,
                 finger_r_right_offset_y,
                 finger_r_right_offset_z,
                 0.0,  //FIXME: get angle of finger...
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // distal digit, right
    tfs->sendEuler("finger_tip_l_right",
                 "finger_l_right",
                 finger_tip_l_right_offset_x,
                 finger_tip_l_right_offset_y,
                 finger_tip_l_right_offset_z,
                 0.0,  //FIXME: get angle of finger tip...
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // distal digit, right
    tfs->sendEuler("finger_tip_r_right",
                 "finger_r_right",
                 finger_tip_r_right_offset_x,
                 finger_tip_r_right_offset_y,
                 finger_tip_r_right_offset_z,
                 0.0,  //FIXME: get angle of finger tip...
                 0.0,
                 0.0,
                 odomMsg.header.stamp);






    // forearm camera left
    tfs->sendEuler("forearm_camera_left",
                 "forearm_roll_left",
                 forearm_camera_left_offset_x,
                 forearm_camera_left_offset_y,
                 forearm_camera_left_offset_z,
                 0.0,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // forearm camera right
    tfs->sendEuler("forearm_camera_right",
                 "forearm_roll_right",
                 forearm_camera_right_offset_x,
                 forearm_camera_right_offset_y,
                 forearm_camera_right_offset_z,
                 0.0,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // wrist camera left
    tfs->sendEuler("wrist_camera_left",
                 "gripper_roll_left",
                 wrist_camera_left_offset_x,
                 wrist_camera_left_offset_y,
                 wrist_camera_left_offset_z,
                 0.0,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // wrist camera right
    tfs->sendEuler("wrist_camera_right",
                 "gripper_roll_right",
                 wrist_camera_right_offset_x,
                 wrist_camera_right_offset_y,
                 wrist_camera_right_offset_z,
                 0.0,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);







    // head pan angle
    tfs->sendEuler("head_pan",
                 "torso",
                 head_pan_offset_x,
                 head_pan_offset_y,
                 head_pan_offset_z,
                 0.0, //FIXME: get pan angle
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // head tilt angle
    tfs->sendEuler("head_tilt",
                 "head_pan",
                 head_tilt_offset_x,
                 head_tilt_offset_y,
                 head_tilt_offset_z,
                 0.0, //FIXME: get tilt angle
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // FIXME: not implemented
    tfs->sendEuler("stereo",
                 "head_pan",
                 0.0,
                 0.0,
                 1.10,
                 0.0,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // base laser location
    tfs->sendEuler("base_laser",
                 "base",
                 base_laser_offset_x,
                 base_laser_offset_y,
                 base_laser_offset_z,
                 0.0,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);

    // tilt laser location
    double tmpPitch, tmpPitchRate;
    //this->PR2Copy->hw.GetJointServoCmd(PR2::HEAD_LASER_PITCH, &tmpPitch, &tmpPitchRate );
    tfs->sendEuler("tilt_laser",
                 "torso",
                 tilt_laser_offset_x,
                 tilt_laser_offset_y,
                 tilt_laser_offset_z,
                 0.0,
                 tmpPitch, //FIXME: verify laser tilt angle
                 0.0,
                 odomMsg.header.stamp);


    /***************************************************************/
    // for the casters
    double tmpSteerFL, tmpVelFL;
    double tmpSteerFR, tmpVelFR;
    double tmpSteerRL, tmpVelRL;
    double tmpSteerRR, tmpVelRR;
    //this->PR2Copy->hw.GetJointServoCmd(PR2::CASTER_FL_STEER, &tmpSteerFL, &tmpVelFL );
    //this->PR2Copy->hw.GetJointServoCmd(PR2::CASTER_FR_STEER, &tmpSteerFR, &tmpVelFR );
    //this->PR2Copy->hw.GetJointServoCmd(PR2::CASTER_RL_STEER, &tmpSteerRL, &tmpVelRL );
    //this->PR2Copy->hw.GetJointServoCmd(PR2::CASTER_RR_STEER, &tmpSteerRR, &tmpVelRR );
    tfs->sendEuler("caster_front_left",
                 "base",
                 caster_front_left_offset_x,
                 caster_front_left_offset_y,
                 caster_front_left_offset_z,
                 tmpSteerFL,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);
    tfs->sendEuler("wheel_front_left_l",
                 "caster_front_left",
                 wheel_front_left_l_offset_x,
                 wheel_front_left_l_offset_y,
                 wheel_front_left_l_offset_z,
                 0.0,
                 0.0, //FIXME: get wheel rotation
                 0.0,
                 odomMsg.header.stamp);
    tfs->sendEuler("wheel_front_left_r",
                 "caster_front_left",
                 wheel_front_left_r_offset_x,
                 wheel_front_left_r_offset_y,
                 wheel_front_left_r_offset_z,
                 0.0,
                 0.0, //FIXME: get wheel rotation
                 0.0,
                 odomMsg.header.stamp);

    tfs->sendEuler("caster_front_right",
                 "base",
                 caster_front_right_offset_x,
                 caster_front_right_offset_y,
                 caster_front_right_offset_z,
                 tmpSteerFR,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);
    tfs->sendEuler("wheel_front_right_l",
                 "caster_front_right",
                 wheel_front_right_l_offset_x,
                 wheel_front_right_l_offset_y,
                 wheel_front_right_l_offset_z,
                 0.0,
                 0.0, //FIXME: get wheel rotation
                 0.0,
                 odomMsg.header.stamp);
    tfs->sendEuler("wheel_front_right_r",
                 "caster_front_right",
                 wheel_front_right_r_offset_x,
                 wheel_front_right_r_offset_y,
                 wheel_front_right_r_offset_z,
                 0.0,
                 0.0, //FIXME: get wheel rotation
                 0.0,
                 odomMsg.header.stamp);

    tfs->sendEuler("caster_rear_left",
                 "base",
                 caster_rear_left_offset_x,
                 caster_rear_left_offset_y,
                 caster_rear_left_offset_z,
                 tmpSteerRL,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);
    tfs->sendEuler("wheel_rear_left_l",
                 "caster_rear_left",
                 wheel_rear_left_l_offset_x,
                 wheel_rear_left_l_offset_y,
                 wheel_rear_left_l_offset_z,
                 0.0,
                 0.0, //FIXME: get wheel rotation
                 0.0,
                 odomMsg.header.stamp);
    tfs->sendEuler("wheel_rear_left_r",
                 "caster_rear_left",
                 wheel_rear_left_r_offset_x,
                 wheel_rear_left_r_offset_y,
                 wheel_rear_left_r_offset_z,
                 0.0,
                 0.0, //FIXME: get wheel rotation
                 0.0,
                 odomMsg.header.stamp);

    tfs->sendEuler("caster_rear_right",
                 "base",
                 caster_rear_right_offset_x,
                 caster_rear_right_offset_y,
                 caster_rear_right_offset_z,
                 tmpSteerRR,
                 0.0,
                 0.0,
                 odomMsg.header.stamp);
    tfs->sendEuler("wheel_rear_right_l",
                 "caster_rear_right",
                 wheel_rear_right_l_offset_x,
                 wheel_rear_right_l_offset_y,
                 wheel_rear_right_l_offset_z,
                 0.0,
                 0.0, //FIXME: get wheel rotation
                 0.0,
                 odomMsg.header.stamp);
    tfs->sendEuler("wheel_rear_right_r",
                 "caster_rear_right",
                 wheel_rear_right_r_offset_x,
                 wheel_rear_right_r_offset_y,
                 wheel_rear_right_r_offset_z,
                 0.0,
                 0.0, //FIXME: get wheel rotation
                 0.0,
                 odomMsg.header.stamp);

    rosnode_->publish("transform",this->shutterMsg);
   

  }


} // namespace gazebo
