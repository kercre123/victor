/**
 * File: imuComponent.h
 *
 * Author: Michael Willett
 * Created: 9/18/2018
 *
 * Description: Component for managing imu data received from robot process
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_Components_ImuComponent_H__
#define __Engine_Components_ImuComponent_H__

#include "engine/components/sensors/iSensorComponent.h"

#include "clad/robotInterface/messageRobotToEngine.h"
#include "coretech/common/engine/robotTimeStamp.h"

#include "util/entityComponent/entity.h"
#include "util/math/math.h"

#include <deque>

namespace Anki {
namespace Vector {

//
// Storage for recent history of IMU data
//  Why is this a separate class from ImuComponent below, you ask?
//  Because ImuComponent inherits a bunch of stuff as a component, including being non-copyable.
//  This gives us a thin wrapper around the actual historical info, which can be copied to, and
//  queried by, the vision thread, as part of its VisionPoseData input.
//
class ImuHistory
{
public:
  
  ImuHistory() = default;
  
  // process a gyro frame and add it to history
  void AddData(RobotInterface::IMUDataFrame&& frame);
  
  // Checks if IMU data at or near timestamp t indicates the head was rotating
  // faster than the given limit
  bool WasHeadRotatingTooFast(RobotTimeStamp_t t,
                              const f32 headTurnSpeedLimit_radPerSec,
                              const int numImuDataToLookBack = 0) const;
  
  // Checks if IMU data at or near timestamp t indicates the body was rotating
  // faster than the given limit
  bool WasBodyRotatingTooFast(RobotTimeStamp_t t,
                              const f32 bodyTurnSpeedLimit_radPerSec,
                              const int numImuDataToLookBack = 0) const;
  
  // Check if the head OR body was rotating too fast at or near timestamp t
  bool WasRotatingTooFast(RobotTimeStamp_t t,
                          const f32 bodyTurnSpeedLimit_radPerSec = DEG_TO_RAD(10),
                          const f32 headTurnSpeedLimit_radPerSec = DEG_TO_RAD(10),
                          const int numImuDataToLookBack = 0) const;
  
  // Provide STL-style (const) accessors to underlying history data
  const RobotInterface::IMUDataFrame& front() const { return _history.front(); }
  const RobotInterface::IMUDataFrame& back() const { return _history.back(); }
  bool empty() const { return _history.empty(); }
  
  using const_iterator = std::deque<RobotInterface::IMUDataFrame>::const_iterator;
  const_iterator begin() const { return _history.begin(); }
  const_iterator end()   const { return _history.end();   }
  
private:
  
  std::deque<RobotInterface::IMUDataFrame> _history;
  
  // Gets the imu data before and after the timestamp
  bool GetImuDataBeforeAndAfter(RobotTimeStamp_t t,
                                RobotInterface::IMUDataFrame& before,
                                RobotInterface::IMUDataFrame& after) const;
  
  // Returns true if the any of the numToLookBack imu data before timestamp t have rates that are greater than
  // the given rates
  bool IsImuDataBeforeTimeGreaterThan(const RobotTimeStamp_t t,
                                      const int numToLookBack,
                                      const f32 rateX, const f32 rateY, const f32 rateZ) const;
  
}; // class ImuHistory

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Component wrapper around the underlying ImuHistory data
//  See note above for ImuHistory as to why we have a separate component wrapper.
//
class ImuComponent :public ISensorComponent, public IDependencyManagedComponent<RobotComponentID>
{
public:
  ImuComponent();
  virtual ~ImuComponent() override = default;

  // process a gyro frame and add it to history
  void AddData(RobotInterface::IMUDataFrame&& frame);
  
  //////
  // IDependencyManagedComponent functions
  //////
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {};
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComps) override {
    InitBase(robot);
  };

  //////
  // end IDependencyManagedComponent functions
  //////

  const ImuHistory& GetImuHistory() const { return _imuHistory; }

protected:
  virtual void NotifyOfRobotStateInternal(const RobotState& msg) override {};
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override;

private:
  ImuHistory _imuHistory;

};
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool ImuHistory::WasRotatingTooFast(RobotTimeStamp_t t,
                                           const f32 bodyTurnSpeedLimit_radPerSec,
                                           const f32 headTurnSpeedLimit_radPerSec,
                                           const int numImuDataToLookBack) const
{
  return (WasHeadRotatingTooFast(t, headTurnSpeedLimit_radPerSec, numImuDataToLookBack) ||
          WasBodyRotatingTooFast(t, bodyTurnSpeedLimit_radPerSec, numImuDataToLookBack));
}

} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_ProxSensorComponent_H__
