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

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"
#include "clad/externalInterface/messageEngineToGame.h" // OnboardingStateEnum are in here.
#include "anki/common/basestation/objectIDs.h"

namespace Anki {
namespace Cozmo {
  
// Forward declarations:
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
  struct RobotObservedObject;
}

class BehaviorOnboardingShowCube : public IBehavior
{
  
protected:

  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorOnboardingShowCube(const Json::Value& config);

public:

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return true;}
  virtual bool ShouldRunWhileOnCharger() const override { return true;}

protected:

  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void AlwaysHandle(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void HandleWhileRunning(const EngineToGameEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void HandleWhileRunning(const GameToEngineEvent& event, BehaviorExternalInterface& behaviorExternalInterface) override;

  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using BaseClass = IBehavior;
  using State = ExternalInterface::OnboardingStateEnum;
  State _state = State::Inactive;
  uint8_t     _numErrors = 0;
  uint8_t     _numErrorsPickup = 0;
  uint8_t     _timesPickedUpCube = 0;
  ObjectID    _targetBlock;
    
  void HandleObjectObserved(BehaviorExternalInterface& behaviorExternalInterface, const ExternalInterface::RobotObservedObject& msg);
  
  void SetState_internal(State state, const std::string& stateName, BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToWaitToInspectCube(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToErrorState(State state, BehaviorExternalInterface& behaviorExternalInterface);
  
  void TransitionToNextState(BehaviorExternalInterface& behaviorExternalInterface);
  
  void StartSubStatePickUpBlock(BehaviorExternalInterface& behaviorExternalInterface);
  void StartSubStateCelebratePickup(BehaviorExternalInterface& behaviorExternalInterface);
  
  bool IsSequenceComplete();
  
  uint8_t _maxErrorsTotal = 4;
  uint8_t _maxErrorsPickup = 5;
  float  _maxTimeBeforeTimeout_Sec = 5.0f * 60.0f;
};

}
}


#endif
