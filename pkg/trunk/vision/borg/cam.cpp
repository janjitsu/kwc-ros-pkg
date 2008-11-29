#include <cstdio>
#include <cstdlib>
#include "cam.h"

using namespace borg;

bool Cam::init()
{
  if (config_status != CAM_NOINIT)
    return false;
  config_status = _init() ? CAM_OK : CAM_ERROR;
  return config_status == CAM_OK;
}

bool Cam::startImageStream()
{
  if (config_status != CAM_OK)
    return false;
  config_status = CAM_STREAMING;
  return _startImageStream();
}

bool Cam::stopImageStream()
{
  if (config_status != CAM_STREAMING)
    return false;
  if (!_stopImageStream())
  {
    config_status = CAM_ERROR;
    return false;
  }
  config_status = CAM_OK;
  return true;
}

bool Cam::savePhoto(uint8_t *photo)
{
  if (config_status != CAM_STREAMING)
    return false;
  return _savePhoto(photo);
}

bool Cam::shutdown()
{
  if (config_status == CAM_STREAMING)
    _stopImageStream();
  if (config_status == CAM_OK || config_status == CAM_ERROR)
    return _shutdown();
  return false;
}

