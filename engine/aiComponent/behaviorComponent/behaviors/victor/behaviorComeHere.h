/**
* File: behaviorComeHere.h
*
* Author: Kevin M. Karol
* Created: 2017-12-05
*
* Description: Behavior that turns towards a face, confirms it through a search, and then drives towards it
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorComeHere_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorComeHere_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/aiComponent/faceSelectionComponent.h"
#include "engine/smartFaceId.h"

#include "clad/types/faceSelectionTypes.h"


namespace Anki {
namespace Vector {

class BehaviorComeHere : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorComeHere(const Json::Value& config);

public:
  //
  // Abstract methods to be overloaded:
  //
  virtual bool WantsToBeActivatedBehavior() const override { return true; }

protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override{
    modifiers.wantsToBeActivatedWhenOffTreads = true;
    modifiers.wantsToBeActivatedWhenOnCharger = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void OnBehaviorActivated() override;

private:
  enum class ComeHereState{
    TurnTowardsMicDirection,
    TurnTowardsFace,
    AlreadyHere,
    DriveTowardsFace,
    NoFaceFound
  };
  
  struct ComeHereParams{
    float  distAlreadyHere_mm_sqr = 0.f;
    bool   preferMicData = false;
  } _params;

  std::vector<FaceSelectionPenaltyMultiplier> _facePriorities;
  SmartFaceID _currentFaceID;

  void TurnTowardsMicDirection();
  void TurnTowardsFace();
  void AlreadyHere();
  void DriveTowardsFace();
  void NoFaceFound();

};

} // namespace Vector
} // namespace Anki


#endif //__Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorComeHere_H__
