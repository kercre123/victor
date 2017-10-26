/**
 * File: feedingCubeController.h
 *
 * Author: Kevin M. Karol
 * Created: 2017-03-28
 *
 * Description: Controls the selection and setting of cube lights and cozmo's
 * lights during the feeding process.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_FeedingComponents_FeedingCubeController_H__
#define __Cozmo_Basestation_BehaviorSystem_FeedingComponents_FeedingCubeController_H__

#include "engine/aiComponent/feedingSoundEffectManager.h"
#include "util/helpers/noncopyable.h"

#include <memory>

namespace Anki {
class ObjectID;
namespace Cozmo {

class ActiveObject;
class BehaviorExternalInterface;
class Robot;
struct CubeStateTracker;
namespace CubeAccelListeners {
class ShakeListener;
}
  
class FeedingCubeController : private Util::noncopyable{
public:
  enum class ControllerState{
    Activated,
    DrainCube,
    Deactivated
  };
  
  FeedingCubeController(BehaviorExternalInterface& behaviorExternalInterface, const ObjectID& objectControlling);
  virtual ~FeedingCubeController();
  
  void Update(BehaviorExternalInterface& behaviorExternalInterface);
  // Set what state the controller should be in
  void SetControllerState(BehaviorExternalInterface& behaviorExternalInterface, ControllerState newState, float eatingDuration_s = -1.0);
  ControllerState GetControllerState() const{ return _currentState;}
  
  // Functions which can be queried to find out the state of the cube being
  // controlled
  bool IsCubeCharging() const;
  bool IsCubeCharged() const;

private:
  using ChargeStateChange = FeedingSoundEffectManager::ChargeStateChange;
  
  ControllerState  _currentState;
  std::unique_ptr<CubeStateTracker> _cubeStateTracker;
  
  // Functions which start/stop listening for shakes and
  // set cube lights
  void InitializeController(BehaviorExternalInterface& behaviorExternalInterface);
  void ReInitializeController(BehaviorExternalInterface& behaviorExternalInterface);
  void ClearController(BehaviorExternalInterface& behaviorExternalInterface);
  
  void CheckForChargeStateChanges(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Functions which manage fading the cube lights up/down during charging and
  // draining process
  void StartCubeDrain(BehaviorExternalInterface& behaviorExternalInterface, float eatingDuration_s);
  void UpdateCubeDrain(BehaviorExternalInterface& behaviorExternalInterface);
  void UpdateChargeLights(BehaviorExternalInterface& behaviorExternalInterface);
  
  // Functions which manage listening for and responding to cube shakes
  void StartListeningForShake(BehaviorExternalInterface& behaviorExternalInterface);
  void ShakeDetected(BehaviorExternalInterface& behaviorExternalInterface, const float shakeScore);
  
  // Uniform function through which updated cube lights are set at the end of
  // the update tick
  void SetCubeLights(BehaviorExternalInterface& behaviorExternalInterface);

  // Sends a message to unity to let it know how audio for the cube shake
  // sound effect should be updated
  void UpdateChargeAudioRound(BehaviorExternalInterface& behaviorExternalInterface, ChargeStateChange changeEnumVal);
  
  const char* ChargeStateChangeToString(ChargeStateChange state);
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_FeedingComponents_FeedingCubeController_H__

