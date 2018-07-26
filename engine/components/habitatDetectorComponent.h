/**
 * File: habitatDetectorComponenet.h
 *
 * Author: Arjun Menon
 * Created: 05/24/2018
 *
 * Description: Passive logic component to determine if we are in a habitat
 * after we have been moved. Uses a combination of sensors to reach "certainty"
 * with some heuristics.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_Cozmo_Basestation_Components_HabitatDetectorComponent_H__
#define __Anki_Cozmo_Basestation_Components_HabitatDetectorComponent_H__

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/robotComponents_fwd.h"
#include "util/signals/simpleSignal_fwd.h"
#include "coretech/common/shared/types.h"
#include "engine/components/sensors/cliffSensorComponent.h"

#include "clad/types/habitatDetectionTypes.h"
#include "coretech/common/engine/math/pose.h"

#include <array>

namespace Anki {
namespace Cozmo {

class Robot;
class ObservableObject;
class BlockWorldFilter;
  
class HabitatDetectorComponent : public IDependencyManagedComponent<RobotComponentID>
{
public:
  
  static constexpr int kNumCliffSensors = CliffSensorComponent::kNumCliffSensors;

  HabitatDetectorComponent();
  
  ~HabitatDetectorComponent();

  //////
  // IDependencyManagedComponent functions
  //////
  
  virtual void InitDependent(Robot* robot, const RobotCompMap& dependentComponents) override;
  
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override
  {
    dependencies.insert(RobotComponentID::CliffSensor);
    dependencies.insert(RobotComponentID::Map);
    dependencies.insert(RobotComponentID::ProxSensor);
    dependencies.insert(RobotComponentID::BlockWorld);
    dependencies.insert(RobotComponentID::FullRobotPose);
    dependencies.insert(RobotComponentID::Vision);
  }
  
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override
  {
    dependencies.insert(RobotComponentID::CliffSensor);
    dependencies.insert(RobotComponentID::Map);
    dependencies.insert(RobotComponentID::ProxSensor);
    dependencies.insert(RobotComponentID::BlockWorld);
    dependencies.insert(RobotComponentID::FullRobotPose);
    dependencies.insert(RobotComponentID::Vision);
  }

  virtual void UpdateDependent(const DependencyManagedEntity<RobotComponentID>& dependentComps) override;
  
  //////
  // end IDependencyManagedComponent functions
  //////

  template<typename T>
  void HandleMessage(const T& msg);
  
  // query if the robot is in a known sub-state (config)
  // callers of this code may read the sub-state and take special actions to make the state
  // converge to the best state available to for grabbing prox sensor data when in the habitat
  HabitatLineRelPose GetHabitatLineRelPose() const { return _habitatLineRelPose; }
  
  HabitatBeliefState GetHabitatBeliefState() const { return _habitatBelief; }
  
  bool IsProxObservingHabitatWall() const;
  
  void ForceSetHabitatBeliefState(HabitatBeliefState belief);
  
protected:
  
  void SendDataToWebViz() const;
  
  // adds valid prox readings to a buffer
  // - used to average readings to counteract noise
  // - returns true when enough readings are collected
  bool UpdateProxObservations();

private:
  
  enum HabitatCliffReadingType
  {
    None  = 0x00,
    Grey  = 0x01,
    White = 0x02
  };
  
  std::vector<Signal::SmartHandle> _signalHandles;
  
  Robot* _robot = nullptr;

  // current belief state of whether we are in the habitat, or not, or undecided
  HabitatBeliefState _habitatBelief;
  
  // current possible configurations of the robot with respect to habitat white-line feature
  HabitatLineRelPose _habitatLineRelPose;
  
  // whether we've seen any white cliff data since being putdown
  bool _detectedWhiteFromCliffs = false;
  
  f32 _nextSendWebVizDataTime_sec = 0.0f;
  
  // readings aggregated from the prox sensor while the front two cliffs are positioned
  // on top of the white line of the habitat, used to compute an average distance
  std::vector<u16> _proxReadingBuffer = {};
  
  // counter for the number of ticks the robot has been collecting readings from prox
  u16 _numTicksWaitingForProx = 0;
};

} // end Cozmo
} // end Anki

#endif // __Anki_Cozmo_Basestation_Components_HabitatDetectorComponent_H__
