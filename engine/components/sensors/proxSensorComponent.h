/**
 * File: proxSensorComponent.h
 *
 * Author: Matt Michini
 * Created: 8/30/2017
 *
 * Description: Component for managing forward distance sensor
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_Components_ProxSensorComponent_H__
#define __Engine_Components_ProxSensorComponent_H__

#include "engine/components/sensors/iSensorComponent.h"

#include "clad/types/proxMessages.h"

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "util/entityComponent/entity.h"
#include "coretech/common/engine/math/pose.h"

namespace Anki {
namespace Vector {

// Storage for processed prox sensor reading with useful metadata
struct ProxSensorData
{
  u16  distance_mm;
  f32  signalQuality;

  bool unobstructed;          // The sensor has not detected anything up to its max range
  bool foundObject;           // The sensor detected an object in the valid operating range
  bool isLiftInFOV;           // Lift (or object on lift) is occluding the sensor

  // TODO: the following should probably not be included in this object in favor of checking the above conditions
  bool isInValidRange;        // Distance is within valid range
  bool isValidSignalQuality;  // Signal quality is sufficiently strong to trust that something was detected
  bool isTooPitched;          // Robot is too far pitched up or down
  bool hasValidRangeStatus;   // RangeStatus reported internally by sensor is valid
};


class ProxSensorComponent :public ISensorComponent, public IDependencyManagedComponent<RobotComponentID>
{
public:
  ProxSensorComponent();
  ~ProxSensorComponent() = default;

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

  // Populates distance_mm with the latest distance value.
  // Returns true if the sensor reading is considered valid (see UpdateReadingValidity()). 
  // Returns false if not valid.
  bool GetLatestDistance_mm(u16& distance_mm) const;
  
  // Returns true if the latest distance sensor reading is valid,
  // same as what GetLatestDistance_mm() would return only you don't get distance also.
  bool IsLatestReadingValid() const { return _latestData.foundObject; }
  
  // Note: If you just need distance data, prefer to use GetLatestDistance_mm() and
  // check its return value rather than calling this method.
  const ProxSensorData& GetLatestProxData() const { return _latestData; }
  
  // Returns true if any part of the lift (or object that it's carrying)
  // falls within the sensor's field of view
  bool IsLiftInFOV() const { return _latestData.isLiftInFOV; }

  // enable or disable this entire component's ability to update the nav map
  void SetNavMapUpdateEnabled(bool enabled) { _enabled = enabled; }
  
  // loads raw prox sensor data to a string for different logging systems
  std::string GetDebugString(const std::string& delimeter = "\n");

protected:
  virtual void NotifyOfRobotStateInternal(const RobotState& msg) override;
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override { return GetDebugString(", "); }
  

private:

  void UpdateNavMap();
  
  // Checks the raw sensor data and updates current component state
  void ProcessRawSensorData();
  
  ProxSensorDataRaw _latestDataRaw;
  ProxSensorData    _latestData;
  Pose3d            _previousRobotPose;
  Pose3d            _currentRobotPose;
  float             _previousMeasurement;
  u8                _measurementsAtPose;

  // The timestamp of the RobotState message with the
  // latest distance sensor data
  uint32_t _lastMsgTimestamp = 0;

  // The pose frame ID of the RobotState message with the
  // latest distance sensor data
  uint32_t _lastMsgPoseFrameID = 0;

  uint32_t _numTicsLiftOutOfFOV = 0;

  // Whether or not navmap updates are enabled
  bool _enabled = true;
  
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_ProxSensorComponent_H__
