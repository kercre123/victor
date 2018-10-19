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
    if (_engineSupervisor->step(Vector::ROBOT_TIME_STEP_MS) == -1) {
      return RESULT_FAIL;
    }
  }
  return RESULT_OK;
}

RangeDataRaw ToFSensor::GetData()
{
  RangeDataRaw rangeData;

  const float* rightImage = rightSensor_->getRangeImage();
  const float* leftImage = leftSensor_->getRangeImage();

  for(int i = 0; i < leftSensor_->getWidth(); i++)
  {
    for(int j = 0; j < 4; j++)
    {
      int index = i*8 + j;
      rangeData.data[index] = leftImage[i*4 + j];
      rangeData.data[index + 4] = rightImage[i*4 + j];
    }
  }
  return rangeData;
}
  
}
}
