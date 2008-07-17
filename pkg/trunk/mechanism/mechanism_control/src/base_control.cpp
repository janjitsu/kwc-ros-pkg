
///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2008, Eric Berger
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright 
//     notice, this list of conditions and the following disclaimer in the 
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of Stanford University nor the names of its 
//     contributors may be used to endorse or promote products derived from 
//     this software without specific prior written permission.
//   
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////////////////

#include "mechanism_control/base_control.h"
#include <signal.h>
#include <etherdrive_hardware/etherdrive_hardware.h>
#include <sys/time.h>

#define BASE_NUM_JOINTS 12

/*const double PGain = 10.0;
const double IGain = 0; 
const double DGain = 0;

const double pGain = 0.1;
const double iGain = 0.1;
const double dGain = 0;

//const double maxPositiveTorque = 0.0528; 
//const double maxNegativeTorque = -0.0528; 
//const double maxEffort = 0.0528;
const double maxEffort = 0.75;
const double maxPositiveTorque = maxEffort; 
const double maxNegativeTorque = -maxEffort; 
const double PGain_Pos = 10.0; 
const double IGain_Pos = 0; 
const double DGain_Pos = 0.1; 

const double IMax  = 10;
const double IMin  = -10;
*/

const double maxPositiveTorque = 0.75; 

MechanismControl::MechanismControl(HardwareInterface *hw){
  this->hw = hw;
}

int notDone = 1;

void finalize(int dummy){
  notDone = 0;
}

double GetTime()
{
  struct timeval t;
  gettimeofday( &t, 0);
  return (double) (t.tv_usec *1e-6 + t.tv_sec);
}


void MechanismControl::Init(){
  r = new Robot("robot"); 

  r->numJoints = BASE_NUM_JOINTS;
  r->numTransmissions = BASE_NUM_JOINTS;
  r->numLinks = BASE_NUM_JOINTS;

  r->transmission = new SimpleTransmission[BASE_NUM_JOINTS];
  r->joint = new Joint[BASE_NUM_JOINTS];
  r->link = new Link[BASE_NUM_JOINTS];

  for(int ii=0; ii<BASE_NUM_JOINTS; ii++){
     r->transmission[ii].actuator = &(hw->actuator[ii]);
     r->transmission[ii].joint = &(r->joint[ii]);
     r->transmission[ii].mechanicalReduction = 1.0;
     r->transmission[ii].motorTorqueConstant = 1.0;
     r->transmission[ii].pulsesPerRevolution = 90000;
     hw->actuator[ii].command.enable = true;
     r->joint[ii].effortLimit = maxPositiveTorque;
  }
  //  SimpleTransmission *sc = new SimpleTransmission(joint,&hi->actuator[0],1,1,14000);
  controller = new BaseController(r);
  controller->Init();
  //controller = new JointController();
  //controller->Init(pGain, iGain, dGain, IMax, IMin, ETHERDRIVE_SPEED, GetTime(), maxPositiveTorque, maxNegativeTorque, maxEffort, &(r->joint[0]));
  //controller->EnableController();
}

void MechanismControl::update()//This function is called only from the realtime loop.  Everything it calls must also be realtime safe.
{
  //Clear actuator commands

  //Process robot model transmissions
  for(int i = 0; i < r->numTransmissions; i++){
    r->transmission[i].propagatePosition();
  }

  //update KDL model with new joint position/velocities
  controller->Update();
#ifdef DEBUG
  for(int i = 0; i < r->numJoints; i++){
    printf("base_control:: cmd:: %d, %f\n",i,r->joint[i].commandedEffort);
  }
#endif
  /*  for(int i = 0; i < r->numJoints; i++){
    r->joint[i].enforceLimits();
    }*/

  for(int i = 0; i < r->numTransmissions; i++){
    r->transmission[i].propagateEffort();
  }
}


int main(int argc, char *argv[]){

 
  signal(SIGINT,  (&finalize));
  signal(SIGQUIT, (&finalize));
  signal(SIGTERM, (&finalize));

  int numBoards = 2;
  int numActuators = 12;
  //  int boardLookUp[] ={0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1}; 
  // int portLookUp[] = {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5};

  int boardLookUp[] = {0,0,0,1,1,1,0,0,0,1,1,1};
  int portLookUp[]  = {2,0,1,2,0,1,5,3,4,5,3,4};

  //int jointId[]={1,2, 0, 7, 8, 6, 4, 5, 3, 10, 11, 9};
  int jointId[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

  string etherIP[] = {"10.12.0.103", "10.11.0.102"};
  string hostIP[] = {"10.12.0.2", "10.11.0.3"};
  
  /*
  int numBoards = 1;
  int numActuators = 1;
  string etherIP[] = {"10.12.0.103"};
  string hostIP[] = {"10.12.0.2"};
  int boardLookUp[] ={0}; 
  int portLookUp[] = {0};
  int jointId[] = {0};
  */
  HardwareInterface *hi = new HardwareInterface(numActuators);
  EtherdriveHardware *h = new EtherdriveHardware(numBoards, numActuators, boardLookUp, portLookUp, jointId, etherIP, hostIP,hi);
  MechanismControl *mc = new MechanismControl(hi);

  h->init();

  mc->Init();
  mc->controller->setVelocity(1,0,0);
  //  mc->controller->SetVelCmd(5);
  while(notDone) {

    h->updateState();
    mc->update();
    h->sendCommand();
    h->tick();
    //        nanosleep(&req,&rem);
    usleep(1000);
    // notDone = 0;
  }  

  delete(mc);
  delete(h);
  delete(hi);
}
