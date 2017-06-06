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

#include "anki/cozmo/basestation/behaviorSystem/behaviors/iBehavior.h"
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

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorOnboardingShowCube(Robot& robot, const Json::Value& config);

public:

  virtual bool IsRunnableInternal(const BehaviorPreReqNone& preReqData) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  virtual bool ShouldRunWhileOnCharger() const override { return true;}

protected:

  virtual Result InitInternal(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual void   StopInternal(Robot& robot) override;
  
  virtual void AlwaysHandle(const EngineToGameEvent& event, const Robot& robot) override;
  virtual void HandleWhileRunning(const EngineToGameEvent& event, Robot& robot) override;
  virtual void HandleWhileRunning(const GameToEngineEvent& event, Robot& robot) override;

  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Types
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  using BaseClass = IBehavior;
  using State = ExternalInterface::OnboardingStateEnum;
  State _state = State::Inactive;
  uint8_t     _numErrors = 0;
  uint8_t     _timesPickedUpCube = 0;
  ObjectID    _targetBlock;
    
  void HandleObjectObserved(Robot& robot, const ExternalInterface::RobotObservedObject& msg);
  
  void SetState_internal(State state, const std::string& stateName, const Robot& robot);
  void TransitionToWaitToInspectCube(Robot& robot);
  void TransitionToErrorState(State state, const Robot& robot);
  
  void TransitionToNextState(Robot& robot);
  
  void StartSubStatePickUpBlock(Robot& robot);
  void StartSubStateCelebratePickup(Robot& robot);
  
  bool IsSequenceComplete();
  
  uint8_t _maxErrorsTotal = 4;
  uint8_t _maxErrorsPickup = 5;
  float  _maxTimeBeforeTimeout_Sec = 5.0f * 60.0f;
};

}
}


#endif
