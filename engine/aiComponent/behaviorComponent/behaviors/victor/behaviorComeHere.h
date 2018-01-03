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

namespace Anki {
namespace Cozmo {

class BehaviorComeHere : public ICozmoBehavior
{
protected:
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorComeHere(const Json::Value& config);

public:
  //
  // Abstract methods to be overloaded:
  //
  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override { return true; }
  virtual bool CarryingObjectHandledInternally() const override { return false;}
  virtual bool ShouldRunWhileOffTreads() const override { return true;}
  virtual bool ShouldRunWhileOnCharger() const override { return true;}

protected:
  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;

private:
  using FaceSelectionPenaltyMultiplier = FaceSelectionComponent::FaceSelectionPenaltyMultiplier;
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

  std::vector<FaceSelectionComponent::FaceSelectionPenaltyMultiplier> _facePriorities;
  SmartFaceID _currentFaceID;

  void TurnTowardsMicDirection(BehaviorExternalInterface& behaviorExternalInterface);
  void TurnTowardsFace(BehaviorExternalInterface& behaviorExternalInterface);
  void AlreadyHere(BehaviorExternalInterface& behaviorExternalInterface);
  void DriveTowardsFace(BehaviorExternalInterface& behaviorExternalInterface);
  void NoFaceFound(BehaviorExternalInterface& behaviorExternalInterface);

};

} // namespace Cozmo
} // namespace Anki


#endif //__Engine_AiComponent_BehaviorComponent_Behaviors_Victor_BehaviorComeHere_H__
