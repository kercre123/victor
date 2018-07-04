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

namespace Anki {
class Pose3d;
namespace Cozmo {

// Storage for processed prox sensor reading with useful metadata
struct ProxSensorData
{
  // Convenience function to see if all validity conditions are met
  bool IsValid() const { 
    return isInValidRange && isValidSignalQuality && !isLiftInFOV && !isTooPitched;
  }

  u16  distance_mm;
  f32  signalQuality;

  bool isInValidRange;        // Distance is within valid range
  bool isValidSignalQuality;  // Signal quality is sufficiently strong to trust that something was detected
  bool isLiftInFOV;           // Lift (or object on lift) is occluding the sensor
  bool isTooPitched;          // Robot is too far pitched up or down
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

protected:
  virtual void NotifyOfRobotStateInternal(const RobotState& msg) override;
  
  virtual std::string GetLogHeader() override;
  virtual std::string GetLogRow() override;
  
public:
  // Populates distance_mm with the latest distance value.
  // Returns true if the sensor reading is considered valid (see UpdateReadingValidity()). 
  // Returns false if not valid.
  bool GetLatestDistance_mm(u16& distance_mm) const;
  
  // Returns true if the latest distance sensor reading is valid,
  // same as what GetLatestDistance_mm() would return only you don't get distance also.
  bool IsLatestReadingValid() const { return _latestData.IsValid(); }
  
  // Note: If you just need distance data, prefer to use GetLatestDistance_mm() and
  // check its return value rather than calling this method.
  const ProxSensorDataRaw& GetLatestProxDataRaw() const { return _latestDataRaw; }

  const ProxSensorData& GetLatestProxData() const { return _latestData; }
  
  // Returns true if the latest sensor reading was of sufficiently
  // high signal strength as to be trusted that something was 
  // actually detected. Distance may not be accurate, but it
  // should be in the ballpark.
  bool IsValidSignalQuality() const { return _latestData.isValidSignalQuality; }

  // Returns the current pose of the prox sensor w.r.t. robot. Computed on-the-fly
  // since it depends on the robot's pose.
  Pose3d GetPose() const;
  
  // Outputs true if the given pose falls within the sensor's field of view
  Result IsInFOV(const Pose3d&, bool& isInFOV) const;
  
  // Returns true if any part of the lift (or object that it's carrying)
  // falls within the sensor's field of view
  bool IsLiftInFOV() const { return _latestData.isLiftInFOV; }

  // Returns true if the robot is too pitched for the reading to be considered valid
  bool IsTooPitched() const {return _latestData.isTooPitched; }

  // calculate the pose directly in front of the robot where the prox sensor is indicating an object
  // returns false if sensor reading isn't valid
  bool CalculateSensedObjectPose(Pose3d& sensedObjectPose) const;

  // enable or disable this entire component's ability to update the nav map
  void SetEnabled(bool enabled) { _enabled = enabled; }

private:

  void UpdateNavMap();
  
  // Updates the flags indicating whether or not the 
  // latest sensor reading is valid. 
  //   1) Is within a reliable obstacle detection range
  //   2) Is not being obstructed by the lift
  //   3) Has a reasonable signal quality
  void UpdateReadingValidity();
  
  // Returns a unitless metric of "signal quality", which is computed as the
  // signal intensity (which is the total signal intensity of the reading)
  // divided by the number of active SPADs (which are the actual imaging sensors)
  static float GetSignalQuality(const ProxSensorDataRaw& proxData) {
    return proxData.signalIntensity / proxData.spadCount;
  }
  
  ProxSensorDataRaw _latestDataRaw;
  ProxSensorData    _latestData;

  // The timestamp of the RobotState message with the
  // latest distance sensor data
  uint32_t _lastMsgTimestamp = 0;

  uint32_t _numTicsLiftOutOfFOV = 0;

  bool _enabled = true;
  
};


} // Cozmo namespace
} // Anki namespace

#endif // __Engine_Components_ProxSensorComponent_H__
