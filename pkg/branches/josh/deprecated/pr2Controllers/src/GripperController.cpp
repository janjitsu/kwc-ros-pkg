#include <pr2Controllers/GripperController.h>

using namespace controller;

GripperController::GripperController()
{
}
    
GripperController::~GripperController( )
{
}

void
GripperController::Update( )
{

}

PR2::PR2_ERROR_CODE
GripperController::setGap(double distance)
{
  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE
GripperController::setForce(double force)
{
  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE
GripperController::close()
{
  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE
GripperController::open()
{
  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE
GripperController::setParam(std::string label,double value)
{
  return PR2::PR2_ALL_OK;
}

PR2::PR2_ERROR_CODE
GripperController::setParam(std::string label,std::string value)
{
  return PR2::PR2_ALL_OK;
}




