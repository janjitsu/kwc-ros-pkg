/*
 *  rosgazebo
 *  Copyright (c) 2008, Willow Garage, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <pthread.h>

// gazebo
#include <gazebo/gazebo.h>
#include <gazebo/GazeboError.hh>
#include <libpr2API/pr2API.h>

#include <genericControllers/Controller.h>
#include <pr2Controllers/ArmController.h>
#include <pr2Controllers/HeadController.h>
#include <pr2Controllers/SpineController.h>
#include <pr2Controllers/BaseController.h>
#include <pr2Controllers/LaserScannerController.h>
#include <pr2Controllers/GripperController.h>

#include "ringbuffer.h"

// roscpp
#include <ros/node.h>
// roscpp - laser
#include <std_msgs/LaserScan.h>
// roscpp - laser image (point cloud)
#include <std_msgs/PointCloudFloat32.h>
#include <std_msgs/Point3DFloat32.h>
#include <std_msgs/ChannelFloat32.h>
// roscpp - used for shutter message right now
#include <std_msgs/Empty.h>
// roscpp - used for broadcasting time over ros
#include <rostools/Time.h>
// roscpp - base
#include <std_msgs/RobotBase2DOdom.h>
#include <std_msgs/BaseVel.h>
// roscpp - arm
#include <std_msgs/PR2Arm.h>
// roscpp - camera
#include <std_msgs/Image.h>

// for frame transforms
#include <rosTF/rosTF.h>

#include <time.h>
#include <signal.h>

// Our node
class RosGazeboNode : public ros::node
{
  private:
    // Messages that we'll send or receive
    std_msgs::BaseVel velMsg;
    std_msgs::LaserScan laserMsg;
    std_msgs::PointCloudFloat32 cloudMsg;
    std_msgs::PointCloudFloat32 full_cloudMsg;
    std_msgs::Empty shutterMsg;  // marks end of a cloud message
    std_msgs::RobotBase2DOdom odomMsg;
    rostools::Time timeMsg;

    // A mutex to lock access to fields that are used in message callbacks
    ros::thread::mutex lock;

    // for frame transforms, publish frame transforms
    rosTFServer tf;

    // time step calculation
    double lastTime, simTime;

    // smooth vx, vw commands
    double vxSmooth, vwSmooth;

    // used to generate Gaussian noise (for PCD)
    double GaussianKernel(double mu,double sigma);

    // used to generate Gaussian noise (for PCD)
    PR2::PR2Robot *PR2Copy;
    CONTROLLER::ArmController          *armCopy;
    CONTROLLER::HeadController         *headCopy;
    CONTROLLER::SpineController        *spineCopy;
    CONTROLLER::BaseController         *baseCopy;
    CONTROLLER::LaserScannerController *laserScannerCopy;
    CONTROLLER::GripperController      *gripperCopy;

  public:
    // Constructor; stage itself needs argc/argv.  fname is the .world file
    // that stage should load.
    RosGazeboNode(int argc, char** argv, const char* fname,
         PR2::PR2Robot          *myPR2,
         CONTROLLER::ArmController          *myArm,
         CONTROLLER::HeadController         *myHead,
         CONTROLLER::SpineController        *mySpine,
         CONTROLLER::BaseController         *myBase,
         CONTROLLER::LaserScannerController *myLaserScanner,
         CONTROLLER::GripperController      *myGripper
         );
    ~RosGazeboNode();

    // advertise / subscribe models
    int AdvertiseSubscribeMessages();

    // Do one update of the simulator.  May pause if the next update time
    // has not yet arrived.
    void Update();

    // Message callback for a std_msgs::BaseVel message, which set velocities.
    void cmdvelReceived();

    // Message callback for a std_msgs::PR2Arm message, which sets arm configuration.
    void cmd_leftarmconfigReceived();
    void cmd_rightarmconfigReceived();

    // laser range data
    float    ranges[GZ_LASER_MAX_RANGES];
    uint8_t  intensities[GZ_LASER_MAX_RANGES];

    // camera data
    std_msgs::Image img;
    
    // camera data
    std_msgs::PR2Arm leftarm;
    std_msgs::PR2Arm rightarm;

    // for the point cloud data
    ringBuffer<std_msgs::Point3DFloat32> *cloud_pts;
    ringBuffer<float>                    *cloud_ch1;

    // keep count for full cloud
    int max_cloud_pts;

    // clean up on interrupt
    static void finalize(int);
};

void
RosGazeboNode::cmd_rightarmconfigReceived()
{
  this->lock.lock();
  /*
  printf("turret angle: %.3f\n", this->rightarm.turretAngle);
  printf("shoulder pitch : %.3f\n", this->rightarm.shoulderLiftAngle);
  printf("shoulder roll: %.3f\n", this->rightarm.upperarmRollAngle);
  printf("elbow pitch: %.3f\n", this->rightarm.elbowAngle);
  printf("elbow roll: %.3f\n", this->rightarm.forearmRollAngle);
  printf("wrist pitch angle: %.3f\n", this->rightarm.wristPitchAngle);
  printf("wrist roll: %.3f\n", this->rightarm.wristRollAngle);
  printf("gripper gap: %.3f\n", this->rightarm.gripperGapCmd);
  
  double jointPosition[] = {this->rightarm.turretAngle,
                            this->rightarm.shoulderLiftAngle,
                            this->rightarm.upperarmRollAngle,
                            this->rightarm.elbowAngle,
                            this->rightarm.forearmRollAngle,
                            this->rightarm.wristPitchAngle,
                            this->rightarm.wristRollAngle,
                            this->rightarm.gripperGapCmd};
  double jointSpeed[] = {0,0,0,0,0,0,0,0};

  //  this->PR2Copy->SetArmJointPosition(PR2::PR2_LEFT_ARM, jointPosition, jointSpeed);
  */
  //*
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_R_PAN           , this->rightarm.turretAngle,       0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_R_SHOULDER_PITCH, this->rightarm.shoulderLiftAngle, 0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_R_SHOULDER_ROLL , this->rightarm.upperarmRollAngle, 0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_R_ELBOW_PITCH   , this->rightarm.elbowAngle,        0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_R_ELBOW_ROLL    , this->rightarm.forearmRollAngle,  0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_R_WRIST_PITCH   , this->rightarm.wristPitchAngle,   0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_R_WRIST_ROLL    , this->rightarm.wristRollAngle,    0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_R_GRIPPER       , this->rightarm.gripperGapCmd,     0);
  this->PR2Copy->hw.CloseGripper(PR2::PR2_RIGHT_GRIPPER, this->rightarm.gripperGapCmd, this->rightarm.gripperForceCmd);
  //*/
  this->lock.unlock();
}


void
RosGazeboNode::cmd_leftarmconfigReceived()
{
  this->lock.lock();
  /*
  double jointPosition[] = {this->leftarm.turretAngle,
                            this->leftarm.shoulderLiftAngle,
                            this->leftarm.upperarmRollAngle,
                            this->leftarm.elbowAngle,
                            this->leftarm.forearmRollAngle,
                            this->leftarm.wristPitchAngle,
                            this->leftarm.wristRollAngle,
                            this->leftarm.gripperGapCmd};
  double jointSpeed[] = {0,0,0,0,0,0,0,0};
  this->PR2Copy->SetArmJointPosition(PR2::PR2_LEFT_ARM, jointPosition, jointSpeed);
  */

  //*
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_L_PAN           , this->leftarm.turretAngle,       0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_L_SHOULDER_PITCH, this->leftarm.shoulderLiftAngle, 0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_L_SHOULDER_ROLL , this->leftarm.upperarmRollAngle, 0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_L_ELBOW_PITCH   , this->leftarm.elbowAngle,        0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_L_ELBOW_ROLL    , this->leftarm.forearmRollAngle,  0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_L_WRIST_PITCH   , this->leftarm.wristPitchAngle,   0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::ARM_L_WRIST_ROLL    , this->leftarm.wristRollAngle,    0);
  // this->PR2Copy->SetJointServoCmd(PR2::ARM_L_GRIPPER       , this->leftarm.gripperGapCmd,     0);
  this->PR2Copy->hw.CloseGripper(PR2::PR2_LEFT_GRIPPER, this->leftarm.gripperGapCmd, this->leftarm.gripperForceCmd);
  //*/
  this->lock.unlock();
}

void
RosGazeboNode::cmdvelReceived()
{
  this->lock.lock();
  double dt;
  double w11, w21, w12, w22;

  // smooth out the commands by time decay
  // with w1,w2=1, this means equal weighting for new command every second
  this->PR2Copy->GetTime(&(this->simTime));
  dt = simTime - lastTime;

  // smooth if dt is larger than zero
  if (dt > 0.0)
  {
    w11 =  1.0;
    w21 =  1.0;
    w12 =  1.0;
    w22 =  1.0;
    this->vxSmooth = (w11 * this->vxSmooth + w21*dt *this->velMsg.vx)/( w11 + w21*dt);
    this->vwSmooth = (w12 * this->vwSmooth + w22*dt *this->velMsg.vw)/( w12 + w22*dt);
  }

  // when running with the 2dnav stack, we need to refrain from moving when steering angles are off.
  // when operating with the keyboard, we need instantaneous setting of both velocity and angular velocity.

  // std::cout << "received cmd: vx " << this->velMsg.vx << " vw " <<  this->velMsg.vw
  //           << " vxsmoo " << this->vxSmooth << " vxsmoo " <<  this->vwSmooth
  //           << " | steer erros: " << this->PR2Copy->BaseSteeringAngleError() << " - " <<  M_PI/100.0
  //           << std::endl;

  // 2dnav: if steering angle is wrong, don't move or move slowly
  if (this->PR2Copy->BaseSteeringAngleError() > M_PI/100.0)
  {
    // set steering angle only
    this->PR2Copy->SetBaseSteeringAngle    (this->vxSmooth,0.0,this->vwSmooth);
  }
  else
  {
    // set base velocity
    this->PR2Copy->SetBaseCartesianSpeedCmd(this->vxSmooth, 0.0, this->vwSmooth);
  }

  // TODO: this is a hack, need to rewrite
  //       if we are trying to stop, send the command through
  if (this->velMsg.vx == 0.0)
  {
    // set base velocity
    this->PR2Copy->SetBaseCartesianSpeedCmd(this->vxSmooth, 0.0, this->vwSmooth);
  }

  this->lastTime = this->simTime;

  this->lock.unlock();
}

RosGazeboNode::RosGazeboNode(int argc, char** argv, const char* fname,
         PR2::PR2Robot          *myPR2,
         CONTROLLER::ArmController          *myArm,
         CONTROLLER::HeadController         *myHead,
         CONTROLLER::SpineController        *mySpine,
         CONTROLLER::BaseController         *myBase,
         CONTROLLER::LaserScannerController *myLaserScanner,
         CONTROLLER::GripperController      *myGripper) :
        ros::node("rosgazebo"),tf(*this)
{
  // accept passed in robot
  this->PR2Copy = myPR2;

  // initialize random seed
  srand(time(NULL));

  // Initialize ring buffer for point cloud data
  this->cloud_pts = new ringBuffer<std_msgs::Point3DFloat32>();
  this->cloud_ch1 = new ringBuffer<float>();

  // FIXME:  move this to Subscribe Models
  param("tilting_laser/max_cloud_pts",max_cloud_pts, 10000);
  this->cloud_pts->allocate(this->max_cloud_pts);
  this->cloud_ch1->allocate(this->max_cloud_pts);

  // initialize times
  this->PR2Copy->GetTime(&(this->lastTime));
  this->PR2Copy->GetTime(&(this->simTime));

}

void RosGazeboNode::finalize(int)
{
  fprintf(stderr,"Caught sig, clean-up and exit\n");
  sleep(1);
  exit(-1);
}


int
RosGazeboNode::AdvertiseSubscribeMessages()
{
  //advertise<std_msgs::LaserScan>("laser");
  advertise<std_msgs::LaserScan>("scan");
  advertise<std_msgs::RobotBase2DOdom>("odom");
  advertise<std_msgs::Image>("image");
  advertise<std_msgs::PointCloudFloat32>("cloud");
  advertise<std_msgs::PointCloudFloat32>("full_cloud");
  advertise<std_msgs::Empty>("shutter");
  advertise<std_msgs::PR2Arm>("left_pr2arm_pos");
  advertise<std_msgs::PR2Arm>("right_pr2arm_pos");
  advertise<rostools::Time>("time");

  subscribe("cmd_vel", velMsg, &RosGazeboNode::cmdvelReceived);
  subscribe("cmd_leftarmconfig", leftarm, &RosGazeboNode::cmd_leftarmconfigReceived);
  subscribe("cmd_rightarmconfig", rightarm, &RosGazeboNode::cmd_rightarmconfigReceived);

  return(0);
}

RosGazeboNode::~RosGazeboNode()
{
}

double
RosGazeboNode::GaussianKernel(double mu,double sigma)
{
  // using Box-Muller transform to generate two independent standard normally disbributed normal variables
  // see wikipedia
  double U = (double)rand()/(double)RAND_MAX; // normalized uniform random variable
  double V = (double)rand()/(double)RAND_MAX; // normalized uniform random variable
  double X = sqrt(-2.0 * ::log(U)) * cos( 2.0*M_PI * V);
  //double Y = sqrt(-2.0 * ::log(U)) * sin( 2.0*M_PI * V); // the other indep. normal variable
  // we'll just use X
  // scale to our mu and sigma
  X = sigma * X + mu;
  return X;
}

void
RosGazeboNode::Update()
{
  this->lock.lock();

  float    angle_min;
  float    angle_max;
  float    angle_increment;
  float    range_max;
  uint32_t ranges_size;
  uint32_t ranges_alloc_size;
  uint32_t intensities_size;
  uint32_t intensities_alloc_size;
  std_msgs::Point3DFloat32 tmp_cloud_pt;

  /***************************************************************/
  /*                                                             */
  /*  laser - pitching                                           */
  /*                                                             */
  /***************************************************************/
  if (this->PR2Copy->hw.GetLaserRanges(PR2::LASER_HEAD,
                &angle_min, &angle_max, &angle_increment,
                &range_max, &ranges_size     , &ranges_alloc_size,
                &intensities_size, &intensities_alloc_size,
                this->ranges     , this->intensities) == PR2::PR2_ALL_OK)
  {
    for(unsigned int i=0;i<ranges_size;i++)
    {
      // get laser pitch angle
      double laser_yaw, laser_pitch, laser_pitch_rate;
      this->PR2Copy->hw.GetJointServoActual(PR2::HEAD_LASER_PITCH , &laser_pitch,  &laser_pitch_rate);
      // get laser yaw angle
      laser_yaw = angle_min + (double)i * angle_increment;
      //std::cout << " pit " << laser_pitch << "yaw " << laser_yaw
      //          << " amin " <<  angle_min << " inc " << angle_increment << std::endl;
      // populating cloud data by range
      double tmp_range = this->ranges[i];
      // transform from range to x,y,z
      tmp_cloud_pt.x                = tmp_range * cos(laser_yaw) * cos(laser_pitch);
      tmp_cloud_pt.y                = tmp_range * sin(laser_yaw) ; //* cos(laser_pitch);
      tmp_cloud_pt.z                = tmp_range * cos(laser_yaw) * sin(laser_pitch);

      // add gaussian noise
      const double sigma = 0.02;  // 2 centimeter sigma
      tmp_cloud_pt.x                = tmp_cloud_pt.x + GaussianKernel(0,sigma);
      tmp_cloud_pt.y                = tmp_cloud_pt.y + GaussianKernel(0,sigma);
      tmp_cloud_pt.z                = tmp_cloud_pt.z + GaussianKernel(0,sigma);

      // add mixed pixel noise
      // if this point is some threshold away from last, add mixing model

      // push pcd point into structure
      this->cloud_pts->add((std_msgs::Point3DFloat32)tmp_cloud_pt);
      this->cloud_ch1->add(this->intensities[i]);
    }
    /***************************************************************/
    /*                                                             */
    /*  point cloud from laser image                               */
    /*                                                             */
    /***************************************************************/
    //std::cout << " pcd num " << this->cloud_pts->length << std::endl;
    int    num_channels = 1;
    this->cloudMsg.set_pts_size(this->cloud_pts->length);
    this->cloudMsg.set_chan_size(num_channels);
    this->cloudMsg.chan[0].name = "intensities";
    this->cloudMsg.chan[0].set_vals_size(this->cloud_ch1->length);

    this->full_cloudMsg.set_pts_size(this->cloud_pts->length);
    this->full_cloudMsg.set_chan_size(num_channels);
    this->full_cloudMsg.chan[0].name = "intensities";
    this->full_cloudMsg.chan[0].set_vals_size(this->cloud_ch1->length);

    for(int i=0;i< this->cloud_pts->length ;i++)
    {
      this->cloudMsg.pts[i].x        = this->cloud_pts->buffer[i].x;
      this->cloudMsg.pts[i].y        = this->cloud_pts->buffer[i].y;
      this->cloudMsg.pts[i].z        = this->cloud_pts->buffer[i].z;
      this->cloudMsg.chan[0].vals[i] = this->cloud_ch1->buffer[i];

      this->full_cloudMsg.pts[i].x        = this->cloud_pts->buffer[i].x;
      this->full_cloudMsg.pts[i].y        = this->cloud_pts->buffer[i].y;
      this->full_cloudMsg.pts[i].z        = this->cloud_pts->buffer[i].z;
      this->full_cloudMsg.chan[0].vals[i] = this->cloud_ch1->buffer[i];
    }
    publish("cloud",this->cloudMsg);
    publish("full_cloud",this->full_cloudMsg);
    //publish("shutter",this->shutterMsg);
  }


  /***************************************************************/
  /*                                                             */
  /*  publish time                                               */
  /*                                                             */
  /***************************************************************/
  this->PR2Copy->GetTime(&(this->simTime));
  timeMsg.rostime.sec  = (unsigned long)floor(this->simTime);
  timeMsg.rostime.nsec = (unsigned long)floor(  1e9 * (  this->simTime - this->laserMsg.header.stamp.sec) );
  publish("time",timeMsg);

  /***************************************************************/
  /*                                                             */
  /*  laser - base                                               */
  /*                                                             */
  /***************************************************************/
  if (this->PR2Copy->hw.GetLaserRanges(PR2::LASER_BASE,
                &angle_min, &angle_max, &angle_increment,
                &range_max, &ranges_size     , &ranges_alloc_size,
                &intensities_size, &intensities_alloc_size,
                this->ranges     , this->intensities) == PR2::PR2_ALL_OK)
  {
    // Get latest laser data
    this->laserMsg.angle_min       = angle_min;
    this->laserMsg.angle_max       = angle_max;
    this->laserMsg.angle_increment = angle_increment;
    this->laserMsg.range_max       = range_max;
    this->laserMsg.set_ranges_size(ranges_size);
    this->laserMsg.set_intensities_size(intensities_size);
    for(unsigned int i=0;i<ranges_size;i++)
    {
      double tmp_range = this->ranges[i];
      this->laserMsg.ranges[i]      =tmp_range;
      this->laserMsg.intensities[i] = this->intensities[i];
    }

    this->laserMsg.header.frame_id = FRAMEID_LASER;
    this->laserMsg.header.stamp.sec = (unsigned long)floor(this->simTime);
    this->laserMsg.header.stamp.nsec = (unsigned long)floor(  1e9 * (  this->simTime - this->laserMsg.header.stamp.sec) );

    //publish("laser",this->laserMsg); // for laser_view FIXME: can alias this at the commandline or launch script
    publish("scan",this->laserMsg);  // for rosstage
  }



  /***************************************************************/
  /*                                                             */
  /*  odometry                                                   */
  /*                                                             */
  /***************************************************************/
  // Get latest odometry data
  // Get velocities
  double vx,vy,vw;
  this->PR2Copy->GetBaseCartesianSpeedActual(&vx,&vy,&vw);
  // Translate into ROS message format and publish
  this->odomMsg.vel.x  = vx;
  this->odomMsg.vel.y  = vy;
  this->odomMsg.vel.th = vw;

  // Get position
  double x,y,z,roll,pitch,yaw;
  this->PR2Copy->GetBasePositionActual(&x,&y,&z,&roll,&pitch,&yaw);
  this->odomMsg.pos.x  = x;
  this->odomMsg.pos.y  = y;
  this->odomMsg.pos.th = yaw;
  // this->odomMsg.stall = this->positionmodel->Stall();

  // TODO: get the frame ID from somewhere
  this->odomMsg.header.frame_id = FRAMEID_ODOM;

  this->odomMsg.header.stamp.sec = (unsigned long)floor(this->simTime);
  this->odomMsg.header.stamp.nsec = (unsigned long)floor(  1e9 * (  this->simTime - this->odomMsg.header.stamp.sec) );

  /***************************************************************/
  /*                                                             */
  /*  frame transforms                                           */
  /*                                                             */
  /***************************************************************/
  tf.sendInverseEuler(FRAMEID_ODOM,
                      FRAMEID_ROBOT,
                      odomMsg.pos.x,
                      odomMsg.pos.y,
                      0.0,
                      odomMsg.pos.th,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // This publish call resets odomMsg.header.stamp.sec and 
  // odomMsg.header.stamp.nsec to zero.  Thus, it must be called *after*
  // those values are reused in the sendInverseEuler() call above.
  publish("odom",this->odomMsg);

  /***************************************************************/
  /*                                                             */
  /*  camera                                                     */
  /*                                                             */
  /***************************************************************/
  uint32_t              width, height, depth;
  std::string           compression, colorspace;
  uint32_t              buf_size;
  static unsigned char  buf[GAZEBO_CAMERA_MAX_IMAGE_SIZE];

  // get image
  //this->PR2Copy->hw.GetCameraImage(PR2::CAMERA_GLOBAL,
  this->PR2Copy->hw.GetCameraImage(PR2::CAMERA_HEAD_RIGHT,
          &width           ,         &height               ,
          &depth           ,
          &compression     ,         &colorspace           ,
          &buf_size        ,         buf                   );
  this->img.width       = width;
  this->img.height      = height;
  this->img.compression = compression;
  this->img.colorspace  = colorspace;

  if(buf_size >0)
  {
    this->img.set_data_size(buf_size);

    this->img.data        = buf;
    //memcpy(this->img.data,buf,data_size);

    publish("image",this->img);
  }

  /***************************************************************/
  /*                                                             */
  /*  pitching Hokuyo joint                                      */
  /*                                                             */
  /***************************************************************/
  static double dAngle = -1;
  double simPitchFreq,simPitchAngle,simPitchRate,simPitchTimeScale,simPitchAmp,simPitchOffset;
  simPitchFreq      = 1.0/10.0;
  simPitchTimeScale = 2.0*M_PI*simPitchFreq;
  simPitchAmp    =  M_PI / 8.0;
  simPitchOffset = -M_PI / 8.0;
  simPitchAngle = simPitchOffset + simPitchAmp * sin(this->simTime * simPitchTimeScale);
  simPitchRate  =  simPitchAmp * simPitchTimeScale * cos(this->simTime * simPitchTimeScale); // TODO: check rate correctness
  this->PR2Copy->GetTime(&this->simTime);
  //std::cout << "sim time: " << this->simTime << std::endl;
  //std::cout << "ang: " << simPitchAngle*180.0/M_PI << "rate: " << simPitchRate*180.0/M_PI << std::endl;
  this->PR2Copy->hw.SetJointTorque(PR2::HEAD_LASER_PITCH , 1000.0);
  this->PR2Copy->hw.SetJointGains(PR2::HEAD_LASER_PITCH, 10.0, 0.0, 0.0);
  this->PR2Copy->hw.SetJointServoCmd(PR2::HEAD_LASER_PITCH , simPitchAngle, simPitchRate);

  if (dAngle * simPitchRate < 0.0)
  {
    dAngle = -dAngle;
    publish("shutter",this->shutterMsg);
  }

  // should send shutter when changing direction, or wait for Tully to implement ring buffer in viewer


  /***************************************************************/
  /*                                                             */
  /*  arm                                                        */
  /*  gripper                                                    */
  /*                                                             */
  /***************************************************************/

  double position, velocity;
  std_msgs::PR2Arm larm, rarm;
  
  /* get left arm position */
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_L_PAN,            &position, &velocity);
  larm.turretAngle       = position;
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_L_SHOULDER_PITCH, &position, &velocity);
  larm.shoulderLiftAngle = position;
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_L_SHOULDER_ROLL,  &position, &velocity);
  larm.upperarmRollAngle = position;
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_L_ELBOW_PITCH,    &position, &velocity);
  larm.elbowAngle        = position; 
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_L_ELBOW_ROLL,     &position, &velocity);
  larm.forearmRollAngle  = position;
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_L_WRIST_PITCH,    &position, &velocity);
  larm.wristPitchAngle   = position;
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_L_WRIST_ROLL,     &position, &velocity);
  larm.wristRollAngle    = position;
  this->PR2Copy->GetLeftGripperActual  (                              &position, &velocity);
  larm.gripperForceCmd   = velocity;
  larm.gripperGapCmd     = position;
  publish("left_pr2arm_pos", larm);
  
  /* get left arm position */
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_R_PAN,            &position, &velocity);
  rarm.turretAngle       = position;
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_R_SHOULDER_PITCH, &position, &velocity);
  rarm.shoulderLiftAngle = position;
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_R_SHOULDER_ROLL,  &position, &velocity);
  rarm.upperarmRollAngle = position;
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_R_ELBOW_PITCH,    &position, &velocity);
  rarm.elbowAngle        = position; 
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_R_ELBOW_ROLL,     &position, &velocity);
  rarm.forearmRollAngle  = position;
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_R_WRIST_PITCH,    &position, &velocity);
  rarm.wristPitchAngle   = position;
  this->PR2Copy->hw.GetJointPositionActual(PR2::ARM_R_WRIST_ROLL,     &position, &velocity);
  rarm.wristRollAngle    = position;
  this->PR2Copy->GetRightGripperActual  (                             &position, &velocity);
  rarm.gripperForceCmd   = velocity;
  rarm.gripperGapCmd     = position;
  publish("right_pr2arm_pos", rarm);
  

  //  this->arm.turretAngle          = 0.0;
  //  this->arm.shoulderLiftAngle    = 0.0;
  //  this->arm.upperarmRollAngle    = 0.0;
  //  this->arm.elbowAngle           = 0.0;
  //  this->arm.forearmRollAngle     = 0.0;
  //  this->arm.wristPitchAngle      = 0.0;
  //  this->arm.wristRollAngle       = 0.0;
  //  this->arm.gripperForceCmd      = 1000.0;
  //  this->arm.gripperGapCmd        = 0.0;
  //
  //  // gripper test
  //  this->PR2Copy->SetGripperGains(PR2::PR2_LEFT_GRIPPER  ,10.0,0.0,0.0);
  //  this->PR2Copy->SetGripperGains(PR2::PR2_RIGHT_GRIPPER ,10.0,0.0,0.0);
  //  this->PR2Copy->OpenGripper(PR2::PR2_LEFT_GRIPPER ,this->arm.gripperGapCmd,this->arm.gripperForceCmd);
  //  this->PR2Copy->CloseGripper(PR2::PR2_RIGHT_GRIPPER,this->arm.gripperGapCmd,this->arm.gripperForceCmd);

  /***************************************************************/
  /*                                                             */
  /*  frame transforms                                           */
  /*                                                             */
  /*  x,y,z,yaw,pitch,roll                                       */
  /*                                                             */
  /***************************************************************/
  //this->PR2Copy->GetBasePositionActual(&x,&y,&z,&roll,&pitch,&yaw); // actual CoM of base
  tf.sendInverseEuler(PR2::FRAMEID_BASE,
                      FRAMEID_ODOM,
                      x,
                      y,
                      z-0.13, /* half height of base box */
                      yaw,
                      pitch,
                      roll,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // base = center of the bottom of the base box
  // torso = midpoint of bottom of turrets
  tf.sendInverseEuler(PR2::FRAMEID_TORSO,
                      PR2::FRAMEID_BASE,
                      PR2::BASE_LEFT_ARM_OFFSET.x,
                      0.0,
                      PR2::BASE_LEFT_ARM_OFFSET.z, /* FIXME: spine elevator not accounted for */
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // arm_l_turret = bottom of left turret
  tf.sendInverseEuler(PR2::FRAMEID_ARM_L_TURRET,
                      PR2::FRAMEID_TORSO,
                      0.0,
                      PR2::BASE_LEFT_ARM_OFFSET.y,
                      0.0,
                      larm.turretAngle,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // arm_l_shoulder = center of left shoulder pitch bracket
  tf.sendInverseEuler(PR2::FRAMEID_ARM_L_SHOULDER,
                      PR2::FRAMEID_ARM_L_TURRET,
                      PR2::ARM_PAN_SHOULDER_PITCH_OFFSET.x,
                      PR2::ARM_PAN_SHOULDER_PITCH_OFFSET.y,
                      PR2::ARM_PAN_SHOULDER_PITCH_OFFSET.z,
                      0.0,
                      larm.shoulderLiftAngle,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // arm_l_upperarm = upper arm with roll DOF, at shoulder pitch center
  tf.sendInverseEuler(PR2::FRAMEID_ARM_L_UPPERARM,
                      PR2::FRAMEID_ARM_L_SHOULDER,
                      PR2::ARM_SHOULDER_PITCH_ROLL_OFFSET.x,
                      PR2::ARM_SHOULDER_PITCH_ROLL_OFFSET.y,
                      PR2::ARM_SHOULDER_PITCH_ROLL_OFFSET.z,
                      0.0,
                      0.0,
                      larm.upperarmRollAngle,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  //frameid_arm_l_elbow = elbow pitch bracket center of rotation
  tf.sendInverseEuler(PR2::FRAMEID_ARM_L_ELBOW,
                      PR2::FRAMEID_ARM_L_UPPERARM,
                      PR2::ARM_SHOULDER_ROLL_ELBOW_PITCH_OFFSET.x,
                      PR2::ARM_SHOULDER_ROLL_ELBOW_PITCH_OFFSET.y,
                      PR2::ARM_SHOULDER_ROLL_ELBOW_PITCH_OFFSET.z,
                      0.0,
                      larm.elbowAngle,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  //frameid_arm_l_forearm = forearm roll DOR, at elbow pitch center
  tf.sendInverseEuler(PR2::FRAMEID_ARM_L_FOREARM,
                      PR2::FRAMEID_ARM_L_ELBOW,
                      PR2::ELBOW_PITCH_ELBOW_ROLL_OFFSET.x,
                      PR2::ELBOW_PITCH_ELBOW_ROLL_OFFSET.y,
                      PR2::ELBOW_PITCH_ELBOW_ROLL_OFFSET.z,
                      0.0,
                      0.0,
                      larm.forearmRollAngle,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // arm_l_wrist = wrist pitch DOF.
  tf.sendInverseEuler(PR2::FRAMEID_ARM_L_WRIST,
                      PR2::FRAMEID_ARM_L_FOREARM,
                      PR2::ELBOW_ROLL_WRIST_PITCH_OFFSET.x,
                      PR2::ELBOW_ROLL_WRIST_PITCH_OFFSET.y,
                      PR2::ELBOW_ROLL_WRIST_PITCH_OFFSET.z,
                      0.0,
                      larm.wristPitchAngle,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // arm_l_hand = hand roll DOF, center at wrist pitch center
  tf.sendInverseEuler(PR2::FRAMEID_ARM_L_HAND,
                      PR2::FRAMEID_ARM_L_WRIST,
                      PR2::WRIST_PITCH_WRIST_ROLL_OFFSET.x,
                      PR2::WRIST_PITCH_WRIST_ROLL_OFFSET.y,
                      PR2::WRIST_PITCH_WRIST_ROLL_OFFSET.z,
                      0.0,
                      0.0,
                      larm.wristRollAngle,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // FIXME: not implemented
  tf.sendInverseEuler(PR2::FRAMEID_ARM_L_FINGER_1,
                      PR2::FRAMEID_ARM_L_HAND,
                      0.05,
                      0.025,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // FIXME: not implemented
  tf.sendInverseEuler(PR2::FRAMEID_ARM_L_FINGER_2,
                      PR2::FRAMEID_ARM_L_HAND,
                      0.05,
                      -0.025,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);


  // arm_r_turret = bottom of right turret
  tf.sendInverseEuler(PR2::FRAMEID_ARM_R_TURRET,
                      PR2::FRAMEID_TORSO,
                      0.0,
                      PR2::BASE_RIGHT_ARM_OFFSET.y,
                      0.0,
                      rarm.turretAngle,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // arm_r_shoulder = center of right shoulder pitch bracket
  tf.sendInverseEuler(PR2::FRAMEID_ARM_R_SHOULDER,
                      PR2::FRAMEID_ARM_R_TURRET,
                      PR2::ARM_PAN_SHOULDER_PITCH_OFFSET.x,
                      PR2::ARM_PAN_SHOULDER_PITCH_OFFSET.y,
                      PR2::ARM_PAN_SHOULDER_PITCH_OFFSET.z,
                      0.0,
                      rarm.shoulderLiftAngle,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // arm_r_upperarm = upper arm with roll DOF, at shoulder pitch center
  tf.sendInverseEuler(PR2::FRAMEID_ARM_R_UPPERARM,
                      PR2::FRAMEID_ARM_R_SHOULDER,
                      PR2::ARM_SHOULDER_PITCH_ROLL_OFFSET.x,
                      PR2::ARM_SHOULDER_PITCH_ROLL_OFFSET.y,
                      PR2::ARM_SHOULDER_PITCH_ROLL_OFFSET.z,
                      0.0,
                      0.0,
                      rarm.upperarmRollAngle,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  //frameid_arm_r_elbow = elbow pitch bracket center of rotation
  tf.sendInverseEuler(PR2::FRAMEID_ARM_R_ELBOW,
                      PR2::FRAMEID_ARM_R_UPPERARM,
                      PR2::ARM_SHOULDER_ROLL_ELBOW_PITCH_OFFSET.x,
                      PR2::ARM_SHOULDER_ROLL_ELBOW_PITCH_OFFSET.y,
                      PR2::ARM_SHOULDER_ROLL_ELBOW_PITCH_OFFSET.z,
                      0.0,
                      rarm.elbowAngle,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  //frameid_arm_r_forearm = forearm roll DOR, at elbow pitch center
  tf.sendInverseEuler(PR2::FRAMEID_ARM_R_FOREARM,
                      PR2::FRAMEID_ARM_R_ELBOW,
                      PR2::ELBOW_PITCH_ELBOW_ROLL_OFFSET.x,
                      PR2::ELBOW_PITCH_ELBOW_ROLL_OFFSET.y,
                      PR2::ELBOW_PITCH_ELBOW_ROLL_OFFSET.z,
                      0.0,
                      0.0,
                      rarm.forearmRollAngle,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // arm_r_wrist = wrist pitch DOF.
  tf.sendInverseEuler(PR2::FRAMEID_ARM_R_WRIST,
                      PR2::FRAMEID_ARM_R_FOREARM,
                      PR2::ELBOW_ROLL_WRIST_PITCH_OFFSET.x,
                      PR2::ELBOW_ROLL_WRIST_PITCH_OFFSET.y,
                      PR2::ELBOW_ROLL_WRIST_PITCH_OFFSET.z,
                      0.0,
                      rarm.wristPitchAngle,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // arm_r_hand = hand roll DOF, center at wrist pitch center
  tf.sendInverseEuler(PR2::FRAMEID_ARM_R_HAND,
                      PR2::FRAMEID_ARM_R_WRIST,
                      PR2::WRIST_PITCH_WRIST_ROLL_OFFSET.x,
                      PR2::WRIST_PITCH_WRIST_ROLL_OFFSET.y,
                      PR2::WRIST_PITCH_WRIST_ROLL_OFFSET.z,
                      0.0,
                      0.0,
                      rarm.wristRollAngle,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // FIXME: not implemented
  tf.sendInverseEuler(PR2::FRAMEID_ARM_R_FINGER_1,
                      PR2::FRAMEID_ARM_R_HAND,
                      0.05,
                      0.025,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // FIXME: not implemented
  tf.sendInverseEuler(PR2::FRAMEID_ARM_R_FINGER_2,
                      PR2::FRAMEID_ARM_R_HAND,
                      0.05,
                      -0.025,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // FIXME: not implemented
  tf.sendInverseEuler(PR2::FRAMEID_HEAD_BASE,
                      PR2::FRAMEID_TORSO,
                      0.0,
                      0.0,
                      1.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // FIXME: not implemented
  tf.sendInverseEuler(PR2::FRAMEID_LASER_BLOCK,
                      PR2::FRAMEID_TORSO,
                      0.0,
                      0.0,
                      1.05,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // FIXME: not implemented
  tf.sendInverseEuler(PR2::FRAMEID_STEREO_BLOCK,
                      PR2::FRAMEID_TORSO,
                      0.0,
                      0.0,
                      1.10,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  // FIXME: not implemented
  tf.sendInverseEuler(PR2::FRAMEID_LASERBLOCK,
                      PR2::FRAMEID_BASE,
                      0.035,
                      0.0,
                      0.26,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  /***************************************************************/
  // for the casters
  double tmpSteerFL, tmpVelFL;
  double tmpSteerFR, tmpVelFR;
  double tmpSteerRL, tmpVelRL;
  double tmpSteerRR, tmpVelRR;
  this->PR2Copy->hw.GetJointServoCmd(PR2::CASTER_FL_STEER, &tmpSteerFL, &tmpVelFL );
  this->PR2Copy->hw.GetJointServoCmd(PR2::CASTER_FR_STEER, &tmpSteerFR, &tmpVelFR );
  this->PR2Copy->hw.GetJointServoCmd(PR2::CASTER_RL_STEER, &tmpSteerRL, &tmpVelRL );
  this->PR2Copy->hw.GetJointServoCmd(PR2::CASTER_RR_STEER, &tmpSteerRR, &tmpVelRR );
  tf.sendInverseEuler(PR2::FRAMEID_CASTER_FL_BODY,
                      PR2::FRAMEID_BASE,
                      PR2::BASE_BODY_OFFSETS[0].x,
                      PR2::BASE_BODY_OFFSETS[0].y,
                      PR2::BASE_BODY_OFFSETS[0].z,
                      0.0,
                      0.0,
                      tmpSteerFL,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);
  tf.sendInverseEuler(PR2::FRAMEID_CASTER_FL_WHEEL_L,
                      PR2::FRAMEID_CASTER_FL_BODY,
                      0.0,
                      PR2::CASTER_DRIVE_OFFSET[0].y,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);
  tf.sendInverseEuler(PR2::FRAMEID_CASTER_FL_WHEEL_R,
                      PR2::FRAMEID_CASTER_FL_BODY,
                      0.0,
                      PR2::CASTER_DRIVE_OFFSET[1].y,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  tf.sendInverseEuler(PR2::FRAMEID_CASTER_FR_BODY,
                      PR2::FRAMEID_BASE,
                      PR2::BASE_BODY_OFFSETS[3].x,
                      PR2::BASE_BODY_OFFSETS[3].y,
                      PR2::BASE_BODY_OFFSETS[3].z,
                      0.0,
                      0.0,
                      tmpSteerFR,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);
  tf.sendInverseEuler(PR2::FRAMEID_CASTER_FR_WHEEL_L,
                      PR2::FRAMEID_CASTER_FR_BODY,
                      0.0,
                      PR2::CASTER_DRIVE_OFFSET[2].y,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);
  tf.sendInverseEuler(PR2::FRAMEID_CASTER_FR_WHEEL_R,
                      PR2::FRAMEID_CASTER_FR_BODY,
                      0.0,
                      PR2::CASTER_DRIVE_OFFSET[3].y,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  tf.sendInverseEuler(PR2::FRAMEID_CASTER_RL_BODY,
                      PR2::FRAMEID_BASE,
                      PR2::BASE_BODY_OFFSETS[6].x,
                      PR2::BASE_BODY_OFFSETS[6].y,
                      PR2::BASE_BODY_OFFSETS[6].z,
                      0.0,
                      0.0,
                      tmpSteerRL,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);
  tf.sendInverseEuler(PR2::FRAMEID_CASTER_RL_WHEEL_L,
                      PR2::FRAMEID_CASTER_RL_BODY,
                      0.0,
                      PR2::CASTER_DRIVE_OFFSET[4].y,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);
  tf.sendInverseEuler(PR2::FRAMEID_CASTER_RL_WHEEL_R,
                      PR2::FRAMEID_CASTER_RL_BODY,
                      0.0,
                      PR2::CASTER_DRIVE_OFFSET[5].y,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);

  tf.sendInverseEuler(PR2::FRAMEID_CASTER_RR_BODY,
                      PR2::FRAMEID_BASE,
                      PR2::BASE_BODY_OFFSETS[9].x,
                      PR2::BASE_BODY_OFFSETS[9].y,
                      PR2::BASE_BODY_OFFSETS[9].z,
                      0.0,
                      0.0,
                      tmpSteerRR,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);
  tf.sendInverseEuler(PR2::FRAMEID_CASTER_RR_WHEEL_L,
                      PR2::FRAMEID_CASTER_RR_BODY,
                      0.0,
                      PR2::CASTER_DRIVE_OFFSET[6].y,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);
  tf.sendInverseEuler(PR2::FRAMEID_CASTER_RR_WHEEL_R,
                      PR2::FRAMEID_CASTER_RR_BODY,
                      0.0,
                      PR2::CASTER_DRIVE_OFFSET[7].y,
                      0.0,
                      0.0,
                      0.0,
                      0.0,
                      odomMsg.header.stamp.sec,
                      odomMsg.header.stamp.nsec);



  this->lock.unlock();
}

void *nonRealtimeLoop(void *rgn)
{
  while (1)
  {
    ((RosGazeboNode*)rgn)->Update();
    // some time out for publishing ros info
    usleep(10000);
  }

}

int 
main(int argc, char** argv)
{ 
  // we need 2 threads, one for RT and one for nonRT
  pthread_t threads[2];

  ros::init(argc,argv);

  /***************************************************************************************/
  /*                                                                                     */
  /*                           The main simulator object                                 */
  /*                                                                                     */
  /***************************************************************************************/
  PR2::PR2Robot* myPR2;
  // Initialize robot object
  myPR2 = new PR2::PR2Robot();
  // Initialize connections
  myPR2->InitializeRobot();
  // Set control mode for the base
  myPR2->SetBaseControlMode(PR2::PR2_CARTESIAN_CONTROL);
  // myPR2->SetJointControlMode(PR2::CASTER_FL_STEER, PR2::TORQUE_CONTROL);
  // myPR2->SetJointControlMode(PR2::CASTER_FR_STEER, PR2::TORQUE_CONTROL);
  // myPR2->SetJointControlMode(PR2::CASTER_RL_STEER, PR2::TORQUE_CONTROL);
  // myPR2->SetJointControlMode(PR2::CASTER_RR_STEER, PR2::TORQUE_CONTROL);

  myPR2->EnableGripperLeft();
  myPR2->EnableGripperRight();

  // Set control mode for the arms
  // FIXME: right now this just sets default to pd control
  //myPR2->SetArmControlMode(PR2::PR2_RIGHT_ARM, PR2::PR2_JOINT_CONTROL);
  //myPR2->SetArmControlMode(PR2::PR2_LEFT_ARM, PR2::PR2_JOINT_CONTROL);
  //------------------------------------------------------------

  // set torques for driving the robot wheels
  // myPR2->hw.SetJointTorque(PR2::CASTER_FL_DRIVE_L, 1000.0 );
  // myPR2->hw.SetJointTorque(PR2::CASTER_FR_DRIVE_L, 1000.0 );
  // myPR2->hw.SetJointTorque(PR2::CASTER_RL_DRIVE_L, 1000.0 );
  // myPR2->hw.SetJointTorque(PR2::CASTER_RR_DRIVE_L, 1000.0 );
  // myPR2->hw.SetJointTorque(PR2::CASTER_FL_DRIVE_R, 1000.0 );
  // myPR2->hw.SetJointTorque(PR2::CASTER_FR_DRIVE_R, 1000.0 );
  // myPR2->hw.SetJointTorque(PR2::CASTER_RL_DRIVE_R, 1000.0 );
  // myPR2->hw.SetJointTorque(PR2::CASTER_RR_DRIVE_R, 1000.0 );

  /***************************************************************************************/
  /*                                                                                     */
  /*                        build actuators from pr2Actuators.xml                        */
  /*                                                                                     */
  /***************************************************************************************/
  //Actuators myActuators(myPR2);
  
  /***************************************************************************************/
  /*                                                                                     */
  /*                            initialize controllers                                   */
  /*                                                                                     */
  /***************************************************************************************/
  CONTROLLER::ArmController          myArm;
  CONTROLLER::HeadController         myHead;
  CONTROLLER::SpineController        mySpine;
  CONTROLLER::BaseController         myBase;
  CONTROLLER::LaserScannerController myLaserScanner;
  CONTROLLER::GripperController      myGripper;

  /***************************************************************************************/
  /*                                                                                     */
  /*                            initialize ROS Gazebo Nodes                              */
  /*                                                                                     */
  /***************************************************************************************/
  RosGazeboNode rgn(argc,argv,argv[1],myPR2,&myArm,&myHead,&mySpine,&myBase,&myLaserScanner,&myGripper);

  /***************************************************************************************/
  /*                                                                                     */
  /*                                on termination...                                    */
  /*                                                                                     */
  /***************************************************************************************/
  signal(SIGINT,  (&rgn.finalize));
  signal(SIGQUIT, (&rgn.finalize));
  signal(SIGTERM, (&rgn.finalize));

  // see if we can subscribe models needed
  if (rgn.AdvertiseSubscribeMessages() != 0)
    exit(-1);

  /***************************************************************************************/
  /*                                                                                     */
  /* Update ROS Gazebo Node                                                              */
  /*   contains controller pointers for the non-RT setpoints                             */
  /*                                                                                     */
  /***************************************************************************************/
  int rgnt = pthread_create(&threads[0],NULL, nonRealtimeLoop, (void *) (&rgn));
  if (rgnt)
  {
    printf("Could not start ROSGazeboNode (code=%d)\n",rgnt);
    exit(-1);
  }

  /***************************************************************************************/
  /*                                                                                     */
  /*      RealTime loop using Gazebo ClientWait function call                            */
  /*        this is updated once every gazebo timestep (world time step size)            */
  /*                                                                                     */
  /***************************************************************************************/
  while(1)
  {

    // Update Controllers
    myArm.Update();
    myHead.Update();
    mySpine.Update();
    myBase.Update();
    myLaserScanner.Update();
    myGripper.Update();

    // Send updated controller commands to hardware
    // myPR2->hw.UpdateHW();

    // wait for Gazebo time step
    myPR2->hw.ClientWait();
  }
  
  /***************************************************************************************/
  /*                                                                                     */
  /* have to call this explicitly for some reason.  probably interference                */
  /* from signal handling in Stage / FLTK?                                               */
  /*                                                                                     */
  /***************************************************************************************/
  ros::msg_destruct();

  exit(0);

}
