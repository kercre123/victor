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

#include "whiskeyToF/tof.h"

#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"

#include <webots/Supervisor.hpp>
#include <webots/RangeFinder.hpp>

#ifndef SIMULATOR
#error SIMULATOR should be defined by any target using tof_mac.cpp
#endif

namespace Anki {
namespace Vector {

namespace {
  bool _engineSupervisorSet = false;
  webots::Supervisor* _engineSupervisor = nullptr;
  
  webots::RangeFinder* leftSensor_;
  webots::RangeFinder* rightSensor_;
}

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

void ToFSensor::SetSupervisor(webots::Supervisor* sup)
{
  _engineSupervisor = sup;
  _engineSupervisorSet = true;
}

ToFSensor::ToFSensor()
{
  if(nullptr != _engineSupervisor)
  {
    DEV_ASSERT(ROBOT_TIME_STEP_MS >= _engineSupervisor->getBasicTimeStep(), "tof_mac.UnexpectedTimeStep");

    leftSensor_ = _engineSupervisor->getRangeFinder("leftRangeSensor");
    rightSensor_ = _engineSupervisor->getRangeFinder("rightRangeSensor");

    leftSensor_->enable(ROBOT_TIME_STEP_MS);
    rightSensor_->enable(ROBOT_TIME_STEP_MS);
  }
}

ToFSensor::~ToFSensor()
{

}

Result ToFSensor::Update()
{
  if (nullptr != _engineSupervisor) {
    if (_engineSupervisor->step(ROBOT_TIME_STEP_MS) == -1) {
      return RESULT_FAIL;
    }
  }
  return RESULT_OK;
}

RangeDataRaw ToFSensor::GetData(bool& dataUpdated)
{
  dataUpdated = true;
  
  RangeDataRaw rangeData;

  const float* rightImage = rightSensor_->getRangeImage();
  const float* leftImage = leftSensor_->getRangeImage();

  for(int i = 0; i < leftSensor_->getWidth(); i++)
  {
    for(int j = 0; j < 4; j++)
    {
      int index = i*8 + j;
      RangingData& lData = rangeData.data[index];
      RangingData& rData = rangeData.data[index + 4];

      lData.numObjects = 1;
      lData.roiStatus = 0;
      lData.spadCount = 90.f;
      rData = lData;
      
      lData.roi = index;
      rData.roi = index + 4;
      
      lData.processedRange_mm = leftImage[i*4 + j];
      rData.processedRange_mm = rightImage[i*4 + j];

      RangeReading reading;
      reading.signalRate_mcps = 25.f;
      reading.ambientRate_mcps = 0.25f;
      reading.sigma_mm = 0.f;
      reading.status = 0;

      reading.rawRange_mm = leftImage[i*4 + j] * 1000;
      lData.readings.push_back(reading);

      reading.rawRange_mm = rightImage[i*4 + j] * 1000;
      rData.readings.push_back(reading);
    }
  }
  
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
