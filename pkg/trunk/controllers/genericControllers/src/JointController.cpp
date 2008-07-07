#include <genericControllers/JointController.h>

JointController::JointController()
{
}
    
JointController::~JointController( )
{
}

CONTROLLER::CONTROLLER_ERROR_CODE
JointController::setPos(double pos)
{
  return CONTROLLER::CONTROLLER_ALL_OK;
}

CONTROLLER::CONTROLLER_ERROR_CODE
JointController::getPosCmd(double *pos)
{
  return CONTROLLER::CONTROLLER_ALL_OK;
}

JointController::getPosAct(double *pos)
{
  return CONTROLLER::CONTROLLER_ALL_OK;
}

CONTROLLER::CONTROLLER_ERROR_CODE
JointController::setTorque(double torque)
{
  return CONTROLLER::CONTROLLER_ALL_OK;
}

CONTROLLER::CONTROLLER_ERROR_CODE
JointController::getTorqueCmd(double *torque)
{
  return CONTROLLER::CONTROLLER_ALL_OK;
}

CONTROLLER::CONTROLLER_ERROR_CODE
JointController::getTorqueAct(double *torque)
{
  return CONTROLLER::CONTROLLER_ALL_OK;
}

CONTROLLER::CONTROLLER_ERROR_CODE
JointController::setVel(double vel)
{
  return CONTROLLER::CONTROLLER_ALL_OK;
}

CONTROLLER::CONTROLLER_ERROR_CODE
JointController::setParam(string label,double value)
{
  return CONTROLLER::CONTROLLER_ALL_OK;
}

CONTROLLER::CONTROLLER_ERROR_CODE
JointController::setParam(string label,string value)
{
  return CONTROLLER::CONTROLLER_ALL_OK;
}



