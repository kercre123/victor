/**
 * File: tof_mac.cpp
 *
 * Author: Al Chaussee
 * Created: 10/18/2018
 *
 * Description: Defines interface to a some number(2) of tof sensors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef WEBOTS

#include "whiskeyToF/tof.h"

#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/factory/emrHelper.h"

namespace Anki {
namespace Vector {

ToFSensor* ToFSensor::_instance = nullptr;

ToFSensor* ToFSensor::getInstance()
{
  if(!IsWhiskey())
  {
    return nullptr;
  }
  
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

RangeDataRaw ToFSensor::GetData(bool& dataUpdated)
{
  RangeDataRaw rangeData;
  return rangeData;
}

int ToFSensor::SetupSensors(const CommandCallback& callback)
{
  if(callback != nullptr)
  {
    callback(CommandResult::Success);
  }
  return 0;
}

int ToFSensor::StartRanging(const CommandCallback& callback)
{
  if(callback != nullptr)
  {
    callback(CommandResult::Success);
  }
  return 0;
}

int ToFSensor::StopRanging(const CommandCallback& callback)
{
  if(callback != nullptr)
  {
    callback(CommandResult::Success);
  }
  return 0;
}

bool ToFSensor::IsRanging() const
{
  return true;
}

bool ToFSensor::IsValidRoiStatus(uint8_t status) const
{
  return true;
}
  
int ToFSensor::PerformCalibration(uint32_t distanceToTarget_mm,
                                  float targetReflectance,
                                  const CommandCallback& callback)
{
  if(callback != nullptr)
  {
    callback(CommandResult::Success);
  }
  return 0;
}

bool ToFSensor::IsCalibrating() const
{
  return false;
}

void ToFSensor::SetLogPath(const std::string& path)
{
  return;
}
  
}
}

#endif // ndef WEBOTS
