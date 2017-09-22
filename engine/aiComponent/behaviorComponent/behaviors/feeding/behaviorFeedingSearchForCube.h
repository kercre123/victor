/**
* File: behaviorFeedingSearchForCube.h
*
* Author: Kevin M. Karol
* Created: 2017-3-28
*
* Description: Behavior for cozmo to anticipate energy being loaded
* into a cube as the user prepares it
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingSearchForCube_H__
#define __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingSearchForCube_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorFeedingSearchForCube : public IBehavior
{
protected:
  using Base = IBehavior;
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorFeedingSearchForCube(const Json::Value& config);
  
public:
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
    
protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;

  
private:
  enum class State {
    FirstSearchForCube,
    MakeFoodRequest,
    SecondSearchForCube,
    FailedToFindCubeReaction
  };
  
  State _currentState;
  float _timeEndSearch_s;
  
  void TransitionToSearchForFoodBase(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToFirstSearchForFood(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToSecondSearchForFood(BehaviorExternalInterface& behaviorExternalInterface);

  
  void TransitionToMakeFoodRequest(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToFailedToFindCubeReaction(BehaviorExternalInterface& behaviorExternalInterface);

  
  void SetState_internal(State state, const std::string& stateName);

  
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_Behaviors_Feeding_BehaviorFeedingSearchForCube_H__

