/**
 * File: imuComponent.cpp
 *
 * Author: Michael Willett
 * Created: 9/18/2018
 *
 * Description: Component for managing imu data received from robot process
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/components/sensors/imuComponent.h"

#include "engine/robot.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "util/console/consoleInterface.h"

namespace Anki {
namespace Vector {
  
namespace {
  const std::string kLogDirName = "imu";
  const f32         kTimeBetweenFrames_ms = 65.0;
  const int         kMaxSizeOfHistory = 80;

} // end anonymous namespace

ImuComponent::ImuComponent() 
: ISensorComponent(kLogDirName)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::ImuSensor)
{
}

void ImuComponent::AddData(RobotInterface::IMUDataFrame&& frame) {
  if(_imuHistory.size() == kMaxSizeOfHistory) { _imuHistory.pop_front(); }
  _imuHistory.emplace_back(frame);
}

bool ImuComponent::GetImuDataBeforeAndAfter(RobotTimeStamp_t t,
                                            RobotInterface::IMUDataFrame& before,
                                            RobotInterface::IMUDataFrame& after) const
{
  if(_imuHistory.size() < 2)
  {
    return false;
  }

  // Start at beginning + 1 because there is no data before the first element
  for(auto iter = _imuHistory.begin() + 1; iter != _imuHistory.end(); ++iter)
  {
    // If we get to the imu data after the timestamp
    // or We have gotten to the imu data that has yet to be given a timestamp and
    // our last known timestamped imu data is fairly close the time we are looking for
    // so use it and the data after it that doesn't have a timestamp
    const int64_t tDiff = ((int64_t)(TimeStamp_t)(iter - 1)->timestamp - (int64_t)(TimeStamp_t)t);
    if(iter->timestamp > t ||
        (iter->timestamp == 0 &&
        ABS(tDiff) < kTimeBetweenFrames_ms))
    {
      after = *iter;
      before = *(iter - 1);
      return true;
    }
  }
  return false;
}

bool ImuComponent::IsImuDataBeforeTimeGreaterThan(const RobotTimeStamp_t t,
                                                    const int numToLookBack,
                                                    const f32 rateX, const f32 rateY, const f32 rateZ) const
{
  if(_imuHistory.empty())
  {
    return false;
  }
  
  auto iter = _imuHistory.begin();
  // Separate check for the first element due to there not being anything before it
  if(iter->timestamp > t)
  {
    if(ABS(iter->rateX) > rateX && ABS(iter->rateY) > rateY && ABS(iter->rateZ) > rateZ)
    {
      return true;
    }
  }
  // If the first element has a timestamp of zero then nothing else will have valid timestamps
  // due to how ImuData comes in during an image so the data that we get for an image that we are in the process of receiving will not
  // have timestamps
  else if(iter->timestamp == 0)
  {
    return false;
  }
  
  for(iter = _imuHistory.begin() + 1; iter != _imuHistory.end(); ++iter)
  {
    // If we get to the imu data after the timestamp
    // or We have gotten to the imu data that has yet to be given a timestamp and
    // our last known timestamped imu data is fairly close the time we are looking for
    const int64_t tDiff = ((int64_t)(TimeStamp_t)(iter - 1)->timestamp - (int64_t)(TimeStamp_t)t);
    if(iter->timestamp > t ||
        (iter->timestamp == 0 &&
        ABS(tDiff) < kTimeBetweenFrames_ms))
    {          
      // Once we get to the imu data after the timestamp look at the numToLookBack imu data before it
      for(int i = 0; i < numToLookBack; i++)
      {
        if(ABS(iter->rateX) > rateX && ABS(iter->rateY) > rateY && ABS(iter->rateZ) > rateZ)
        {
          return true;
        }
        if(iter-- == _imuHistory.begin())
        {
          return false;
        }
      }
      return false;
    }
  }
  return false;
}

std::string ImuComponent::GetLogHeader()
{
  return std::string("timestamp_ms, rateX, rateY, rateZ");
}


std::string ImuComponent::GetLogRow()
{
  std::stringstream ss;
  if (!_imuHistory.empty()) {
    const auto& d = _imuHistory.back();
    ss << d.timestamp << ", ";
    ss << d.rateX     << ", ";
    ss << d.rateY     << ", ";
    ss << d.rateZ;
  }

  return ss.str();
}




} // Cozmo namespace
} // Anki namespace
