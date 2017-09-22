/**
* File: behaviorSearchForFace.h
*
* Author: Kevin M. Karol
* Created: 7/21/17
*
* Description: A behavior which uses an animation in order to search around for a face
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorSearchForFace_H__
#define __Cozmo_Basestation_Behaviors_BehaviorSearchForFace_H__

#include "engine/aiComponent/behaviorSystem/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {
  
class BehaviorSearchForFace : public IBehavior
{
protected:
  using base = IBehavior;
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorSearchForFace(const Json::Value& config);
  
public:
  virtual ~BehaviorSearchForFace() override {}
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;

  virtual bool CarryingObjectHandledInternally() const override{ return false;}
  
protected:
  virtual Result OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  
private:
  enum class State {
    SearchingForFace,
    FoundFace
  };
  
  State    _behaviorState;
  
  void TransitionToSearchingAnimation(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToFoundFace(BehaviorExternalInterface& behaviorExternalInterface);
  
  void SetState_internal(State state, const std::string& stateName);

};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorSearchForFace_H__
