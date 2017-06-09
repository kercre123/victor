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

#include <memory>

namespace Anki {
class ObjectID;
namespace Cozmo {

class Robot;
class ActiveObject;
class Robot;
struct CubeStateTracker;
namespace CubeAccelListeners {
class ShakeListener;
}
  
class FeedingCubeController{
public:
  enum class ControllerState{
    Activated,
    DrainCube,
    Deactivated
  };
  
  FeedingCubeController(Robot& robot, const ObjectID& objectControlling);
  virtual ~FeedingCubeController();
  
  void Update(Robot& robot);
  // Set what state the controller should be in
  void SetControllerState(Robot& robot, ControllerState newState, float eatingDuration_s = -1.0);
  
  // Functions which can be queried to find out the state of the cube being
  // controlled
  bool IsCubeCharging() const;
  bool IsCubeCharged() const;
  bool IsCubeDrained() const;

private:
  // As defined in messageEngineToGame.clad
  enum class ChargeStateChange{
    Charge_Start = 0,
    Charge_Up = 1,
    Charge_Down = 2,
    Charge_Complete = 3,
    Charge_Stop = 4
  };
  
  ControllerState  _currentStage;
  std::unique_ptr<CubeStateTracker> _cubeStateTracker;
  
  // Functions which start/stop listening for shakes and
  // set cube lights
  void InitializeController(Robot& robot);
  void ClearController(Robot& robot);
  
  void CheckForChargeStateChanges(Robot& robot);
  
  // Functions which manage fading the cube lights up/down during charging and
  // draining process
  void StartCubeDrain(Robot& robot, float eatingDuration_s);
  void UpdateCubeDrain(Robot& robot);
  void UpdateChargeLights(Robot& robot);
  
  // Functions which manage listening for and responding to cube shakes
  void StartListeningForShake(Robot& robot);
  void ShakeDetected(Robot& robot, const float shakeScore);
  
  // Uniform function through which updated cube lights are set at the end of
  // the update tick
  void SetCubeLights(Robot& robot);

  // Sends a message to unity to let it know how audio for the cube shake
  // sound effect should be updated
  void UpdateChargeAudioRound(Robot& robot, ChargeStateChange changeEnumVal);
  
  const char* ChargeStateChangeToString(ChargeStateChange state);
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_FeedingComponents_FeedingCubeController_H__

