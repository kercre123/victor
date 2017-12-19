/**
 * File: behaviorVictorDemoObservingFaceInteraction.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-24
 *
 * Description: Behavior to search for and interact with faces in a "quiet" way during the observing state
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorVictorDemoObservingFaceInteraction_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorVictorDemoObservingFaceInteraction_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

#include "engine/smartFaceId.h"

namespace Anki {
namespace Cozmo {

class BehaviorVictorDemoObservingFaceInteraction : public ICozmoBehavior
{
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;  
  BehaviorVictorDemoObservingFaceInteraction(const Json::Value& config);

public:

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override {
    // can always either search or interact
    return true;
  }

  virtual bool CarryingObjectHandledInternally() const override {return false;}

protected:

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;
  
  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool CanBeGentlyInterruptedNow(BehaviorExternalInterface& behaviorExternalInterface) const override;

private:

  enum class State {
    FindFaces,
    TurnTowardsFace,
    StareAtFace
  };

  TimeStamp_t GetRecentFaceTime(BehaviorExternalInterface& behaviorExternalInterface);
  SmartFaceID GetFaceToStareAt(BehaviorExternalInterface& behaviorExternalInterface);
  bool ShouldStareAtFace(BehaviorExternalInterface& behaviorExternalInterface, const SmartFaceID& face) const;
  
  void TransitionToFindFaces(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToTurnTowardsAFace(BehaviorExternalInterface& behaviorExternalInterface);
  void TransitionToStareAtFace(BehaviorExternalInterface& behaviorExternalInterface, SmartFaceID face);

  void SetState_internal(State state, const std::string& stateName);

  State _state = State::FindFaces;

  ICozmoBehaviorPtr _searchBehavior;
  
  // which faces we've already looked at during this activation of the behavior. 
  std::vector<SmartFaceID> _faceIdsLookedAt;

};

}
}



#endif
