/**
 * File: behaviorObservingLookAtFaces.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-24
 *
 * Description: Behavior to search for and interact with faces in a "quiet" way during the observing state
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorObservingLookAtFaces_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorObservingLookAtFaces_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/common/engine/robotTimeStamp.h"
#include "engine/smartFaceId.h"

namespace Anki {
namespace Vector {

class BehaviorObservingLookAtFaces : public ICozmoBehavior
{
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;  
  BehaviorObservingLookAtFaces(const Json::Value& config);

public:

  virtual bool WantsToBeActivatedBehavior() const override {
    // can always either search or interact
    return true;
  }

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Standard });
    modifiers.visionModesForActiveScope->insert({ VisionMode::CroppedDetectingFaces, EVisionUpdateFrequency::Standard });
    
    // Avoid marker detection to improve performance
    // TODO: Remove with VIC-6838
    modifiers.visionModesForActiveScope->insert({ VisionMode::DisableMarkerDetection, EVisionUpdateFrequency::Standard });

  }

  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override {}

  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;

  virtual void InitBehavior() override;
  
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual bool CanBeGentlyInterruptedNow() const override;

private:

  enum class State {
    FindFaces,
    TurnTowardsFace,
    StareAtFace
  };

  RobotTimeStamp_t GetRecentFaceTime();
  SmartFaceID GetFaceToStareAt();
  bool ShouldStareAtFace(const SmartFaceID& face) const;
  
  void TransitionToFindFaces();
  void TransitionToTurnTowardsAFace();
  void TransitionToStareAtFace(SmartFaceID face);

  void SetState_internal(State state, const std::string& stateName);

  State _state = State::FindFaces;

  ICozmoBehaviorPtr _searchBehavior;
  
  // which faces we've already looked at during this activation of the behavior. 
  std::vector<SmartFaceID> _faceIdsLookedAt;

};

}
}



#endif
