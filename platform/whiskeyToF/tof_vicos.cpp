/**
 * File: tof_vicos.cpp
 *
 * Author: Al Chaussee
 * Created: 10/18/2018
 *
 * Description: Defines interface to a some number(2) of tof sensors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "whiskeyToF/tof.h"

#include "util/logging/logging.h"

#ifdef SIMULATOR
#error SIMULATOR should be defined by any target using tof_vicos.cpp
#endif

namespace Anki {
namespace Vector {

ToFSensor* ToFSensor::_instance = nullptr;

ToFSensor* ToFSensor::getInstance()
{
  DEV_ASSERT(_engineSupervisorSet, "tof_mac.NoSupervisorSet");
  if(nullptr == _instance)
  {
    _instance = new ToFSensor();
  }
  return _instance;
}

void ToFSensor::removeInstance()
{
  Util::SafeDelete(_instance);
}

ToFSensor::ToFSensor()
{

}

ToFSensor::~ToFSensor()
{

}

Result ToFSensor::Update()
{
  return RESULT_OK;
}

RangeDataRaw ToFSensor::GetData()
{
  RangeDataRaw rangeData;
  return rangeData;
}
  
}
}
