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
#include "engine/entity.h"

namespace Anki {
class Pose3d;
namespace Cozmo {

class ProxSensorComponent :public ISensorComponent
{
public:
  ProxSensorComponent();
  ~ProxSensorComponent() = default;

  //////
  // IDependencyManagedComponent functions
  //////
  // Maintain the chain of initializations currently in robot - it might be possible to
  // change the order of initialization down the line, but be sure to check for ripple effects
  // when changing this function
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CliffSensor);
  };
  //////
  // end IDependencyManagedComponent functions
  //////

protected:
  virtual void UpdateInternal(const RobotState& msg) override;
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override;
  
public:
  u16 GetLatestDistance_mm() const { return _latestData.distance_mm; }
  
  const ProxSensorData& GetLatestProxData() const { return _latestData; }

  // Returns the current pose of the prox sensor w.r.t. robot. Computed on-the-fly
  // since it depends on the robot's pose.
  Pose3d GetPose() const;
  
  // Outputs true if the given pose falls within the sensor's field of view
  Result IsInFOV(const Pose3d&, bool& isInFOV) const;
  
  // Outputs true if any part of the lift falls within the sensor's field of view
  Result IsLiftInFOV(bool& isInFOV) const;
  
  // Indicates when the sensor reading is both 
  //   1) Within a reliable obstacle detection range
  //   2) Is not being obstructed by the lift
  bool IsSensorReadingValid();

  // calculate the pose directly in front of the robot where the prox sensor is indicating an object
  // returns false if sensor reading isn't valid
  bool CalculateSensedObjectPose(Pose3d& sensedObjectPose);


private:

  void UpdateNavMap();
  
  ProxSensorData _latestData;
  
  // The timestamp of the RobotState message with the
  // latest distance sensor data
  uint32_t _lastMsgTimestamp = 0;
  
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_ProxSensorComponent_H__
