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

namespace Anki {
class Pose3d;
namespace Cozmo {

class ProxSensorComponent : public ISensorComponent
{
public:
  ProxSensorComponent(Robot& robot);
  ~ProxSensorComponent() = default;

protected:
  virtual void UpdateInternal(const RobotState& msg) override;
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override;
  
public:
  u16 GetLatestDistance_mm() const { return _latestData.distance_mm; }
  
  // Returns the current pose of the prox sensor w.r.t. robot. Computed on-the-fly
  // since it depends on the robot's pose.
  Pose3d GetPose() const;
  
  // Outputs true if the given pose falls within the sensor's field of view
  Result IsInFOV(const Pose3d&, bool& isInFOV) const;
  
  // Outputs true if any part of the lift falls within the sensor's field of view
  Result IsLiftInFOV(bool& isInFOV) const;
  
  void ActivateTheremin(const bool b=true);
  
private:

  void UpdateNavMap();
  
  void UpdateTheremin();
  bool _thereminActive = false;
  
  ProxSensorData _latestData;
  
  // The timestamp of the RobotState message with the
  // latest distance sensor data
  uint32_t _lastMsgTimestamp = 0;
  
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_ProxSensorComponent_H__
