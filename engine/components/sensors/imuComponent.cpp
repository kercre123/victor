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
#include "util/logging/logging.h"

#define LOG_CHANNEL "ImuComponent"

namespace Anki {
namespace Vector {
  
namespace {
  const std::string kLogDirName = "imu";
  const int         kMaxSizeOfHistory = 80;
} // end anonymous namespace

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImuComponent::ImuComponent() 
: ISensorComponent(kLogDirName)
, IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::ImuSensor)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuComponent::AddData(IMUDataFrame&& frame)
{
  _imuHistory.AddData(std::move(frame));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImuHistory::AddData(IMUDataFrame&& frame)
{
  while (_history.size() >= kMaxSizeOfHistory) {
    _history.pop_front();
  }
  
  DEV_ASSERT_MSG(_history.empty() || (frame.timestamp >= _history.back().timestamp),
                 "ImuHistory.AddData.TimeMovingBackwards",
                 "history size %zu, frame timestamp %d, last timestamp %d",
                 _history.size(), frame.timestamp, _history.back().timestamp);
  
  _history.emplace_back(frame);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ImuHistory::GetImuDataBeforeAndAfter(RobotTimeStamp_t t,
                                          IMUDataFrame& before,
                                          IMUDataFrame& after) const
{
  if(_history.size() < 2)
  {
    return false;
  }

  // Start at beginning + 1 because there is no data before the first element
  for(auto iter = _history.begin() + 1; iter != _history.end(); ++iter)
  {
    // If we get to the imu data after or equal to the timestamp
    if(iter->timestamp >= t)
    {
      after = *iter;
      before = *(iter - 1);
      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ImuHistory::IsImuDataBeforeTimeGreaterThan(const RobotTimeStamp_t t,
                                                const int numToLookBack,
                                                const f32 rateX, const f32 rateY, const f32 rateZ) const
{
  if(_history.empty())
  {
    return false;
  }
  
  auto iter = _history.begin();
  // Separate check for the first element due to there not being anything before it
  if(iter->timestamp > t)
  {
    if(ABS(iter->gyroRobotFrame.x) > rateX && ABS(iter->gyroRobotFrame.y) > rateY && ABS(iter->gyroRobotFrame.z) > rateZ)
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
  
  for(iter = _history.begin() + 1; iter != _history.end(); ++iter)
  {
    // If we get to the imu data after or equal to the timestamp
    if(iter->timestamp >= t)
    {          
      // Once we get to the imu data after the timestamp look at the numToLookBack imu data before it
      for(int i = 0; i < numToLookBack; i++)
      {
        if(ABS(iter->gyroRobotFrame.x) > rateX && ABS(iter->gyroRobotFrame.y) > rateY && ABS(iter->gyroRobotFrame.z) > rateZ)
        {
          return true;
        }
        if(iter-- == _history.begin())
        {
          return false;
        }
      }
      return false;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string ImuComponent::GetLogHeader()
{
  return std::string("timestamp_ms, rateX, rateY, rateZ");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string ImuComponent::GetLogRow()
{
  std::stringstream ss;
  if (!_imuHistory.empty()) {
    const auto& d = _imuHistory.back();
    ss << d.timestamp        << ", ";
    ss << d.gyroRobotFrame.x << ", ";
    ss << d.gyroRobotFrame.y << ", ";
    ss << d.gyroRobotFrame.z;
  }

  return ss.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ImuHistory::WasHeadRotatingTooFast(RobotTimeStamp_t t,
                                        const f32 headTurnSpeedLimit_radPerSec,
                                        const int numImuDataToLookBack) const
{
  if(numImuDataToLookBack > 0)
  {
    return (IsImuDataBeforeTimeGreaterThan(t,
                                           numImuDataToLookBack,
                                           0, headTurnSpeedLimit_radPerSec, 0));
  }
  
  IMUDataFrame prev, next;
  if(!GetImuDataBeforeAndAfter(t, prev, next))
  {
    const u32 newestTimestamp = _history.empty() ? 0 : (u32) _history.back().timestamp;
    LOG_INFO("ImuComponent.WasHeadRotatingTooFast.NoIMUData",
             "Could not get next/previous imu data for timestamp %u (newest timestamp in history %u - diff %u)",
             (TimeStamp_t)t, newestTimestamp, (u32) t - newestTimestamp);
    return true;
  }
  
  if(ABS(prev.gyroRobotFrame.y) > headTurnSpeedLimit_radPerSec ||
     ABS(next.gyroRobotFrame.y) > headTurnSpeedLimit_radPerSec)
  {
    return true;
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ImuHistory::WasBodyRotatingTooFast(RobotTimeStamp_t t,
                                        const f32 bodyTurnSpeedLimit_radPerSec,
                                        const int numImuDataToLookBack) const
{
  if(numImuDataToLookBack > 0)
  {
    return (IsImuDataBeforeTimeGreaterThan(t,
                                           numImuDataToLookBack,
                                           0, 0, bodyTurnSpeedLimit_radPerSec));
  }
  
  IMUDataFrame prev, next;
  if(!GetImuDataBeforeAndAfter(t, prev, next))
  {
    LOG_INFO("ImuComponent..WasBodyRotatingTooFast",
             "Could not get next/previous imu data for timestamp %u", (TimeStamp_t)t);
    return true;
  }
  
  if(ABS(prev.gyroRobotFrame.z) > bodyTurnSpeedLimit_radPerSec ||
     ABS(next.gyroRobotFrame.z) > bodyTurnSpeedLimit_radPerSec)
  {
    return true;
  }
  
  return false;
}

} // Cozmo namespace
} // Anki namespace
