/**
 * File: behaviorOnboardingShowCubes.h
 *
 * Author: Molly Jameson
 * Created: 2016-09-01
 *
 * Description: SuperSpecificOnboarding behavior since it requires a lot of cube information, needs to be in engine
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorOnboardingShowCube_H__
#define __Cozmo_Basestation_Behaviors_BehaviorOnboardingShowCube_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/externalInterface/messageEngineToGame.h" // OnboardingStateEnum are in here.
#include "coretech/common/engine/objectIDs.h"

namespace Anki {
namespace Cozmo {
  
// Forward declarations:
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
  struct RobotObservedObject;
}

class BehaviorOnboardingShowCube : public ICozmoBehavior
{
  
protected:

  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorOnboardingShowCube(const Json::Value& config);

public:
  virtual bool WantsToBeActivatedBehavior() const override;

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
    modifiers.behaviorAlwaysDelegates = false;
  }
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual void AlwaysHandleInScope(const EngineToGameEvent& event) override;
  virtual void HandleWhileActivated(const EngineToGameEvent& event) override;
  virtual void HandleWhileActivated(const GameToEngineEvent& event) override;

  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using BaseClass = ICozmoBehavior;
  using State = ExternalInterface::OnboardingStateEnum;
  State _state = State::Inactive;
  uint8_t     _numErrors = 0;
  uint8_t     _numErrorsPickup = 0;
  uint8_t     _timesPickedUpCube = 0;
  ObjectID    _targetBlock;
    
  void HandleObjectObserved(const ExternalInterface::RobotObservedObject& msg);
  
  void SetState_internal(State state, const std::string& stateName);
  void TransitionToWaitToInspectCube();
  void TransitionToErrorState(State state);
  
  void TransitionToNextState();
  
  void StartSubStatePickUpBlock();
  void StartSubStateCelebratePickup();
  
  bool IsSequenceComplete();
  
  uint8_t _maxErrorsTotal = 4;
  uint8_t _maxErrorsPickup = 5;
  float  _maxTimeBeforeTimeout_Sec = 5.0f * 60.0f;
};

}
}


#endif
