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

  // check its return value rather than calling this method.
  const ProxSensorData& GetLatestProxData() const { return _latestData; }
  
  // enable or disable this entire component's ability to update the nav map
  void SetNavMapUpdateEnabled(bool enabled) { _mapEnabled = enabled; }
  
  // loads raw prox sensor data to a string for different logging systems
  std::string GetDebugString(const std::string& delimeter = "\n");

protected:
  virtual void NotifyOfRobotStateInternal(const RobotState& msg) override;
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override { return GetDebugString(", "); }
  

private:

  // Pushes processed prox data into the NavMap
  void UpdateNavMap(uint32_t timestamp);
  
  // Checks if the lift is currently blocking any sensor regions
  bool CheckLiftOcclusion();
  
  ProxSensorDataRaw _latestDataRaw;           // raw data sent from robot process - should only be used for debugging purposes
  ProxSensorData    _latestData;              // processed data that has additional cleanup and state
  Pose3d            _currentRobotPose;        // robot pose at current sensor reading
  uint32_t          _measurementsAtPose = 0;  // counter to limit number of map updates while not moving
  uint32_t          _numTicsLiftOutOfFOV = 0; // counter for lift FOV checks to account for sensor delay
  bool              _mapEnabled = true;       // disable map updates entirely (currently for low-power mode)
  
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_ProxSensorComponent_H__
