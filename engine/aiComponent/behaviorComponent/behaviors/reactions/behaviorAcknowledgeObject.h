/**
 * File: behaviorAcknowledgeObject.h
 *
 * Author:  Andrew Stein
 * Created: 2016-06-14
 *
 * Description:  Simple quick reaction to a "new" object, just to show Cozmo has noticed it.
 *               Cozmo just turns towards the object and then plays a reaction animation.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeObject_H__
#define __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeObject_H__

#include "anki/common/basestation/objectIDs.h"
#include "anki/common/basestation/math/pose.h"
#include "anki/common/shared/radians.h"
#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/objectFamilies.h"
#include "anki/common/constantsAndMacros.h"

#include <set>

namespace Anki {
namespace Cozmo {

// Forward declarations:
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
struct RobotObservedObject;
}
class ObservableObject;
  
class BehaviorAcknowledgeObject : public ICozmoBehavior
{
private:
  using super = ICozmoBehavior;

public:
  virtual bool CarryingObjectHandledInternally() const override { return true;}
  
  void SetObjectsToAcknowledge(const std::set<s32>& targets){_targets = targets;}
  
protected:
  // Default configuration parameters which can be overridden by JSON config
  struct AcknowledgeConfigParams {
    AnimationTrigger reactionAnimTrigger   = AnimationTrigger::Count;
    Radians     maxTurnAngle_rad           = DEG_TO_RAD_F32(45.f);
    Radians     panTolerance_rad           = DEG_TO_RAD_F32(5.f);
    Radians     tiltTolerance_rad          = DEG_TO_RAD_F32(5.f);
    s32         numImagesToWaitFor         = 2; // After turning before playing animation, 0 means don't verify
  } _params;
  
  void LoadConfig(const Json::Value& config);
  
  // Enforce creation through BehaviorContainer
  friend class BehaviorContainer;
  BehaviorAcknowledgeObject(const Json::Value& config);

  void InitBehavior(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual Status UpdateInternal_WhileRunning(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual void   OnBehaviorDeactivated(BehaviorExternalInterface& behaviorExternalInterface) override;

  virtual bool WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const override;
  
  virtual void AddListener(IReactToObjectListener* listener) override;

  
private:
  void BeginIteration(BehaviorExternalInterface& behaviorExternalInterface);
  void LookForStackedCubes(BehaviorExternalInterface& behaviorExternalInterface);
  void FinishIteration(BehaviorExternalInterface& behaviorExternalInterface);
  
  // helper functions to set the ghost object's pose
  void SetGhostBlockPoseRelObject(BehaviorExternalInterface& behaviorExternalInterface, const ObservableObject* obj, float zOffset);
  
  // helper function to check stack visibility, with optional output argument for whether
  // the ghost cube is outside the current FOV
  bool CheckIfGhostBlockVisible(BehaviorExternalInterface& behaviorExternalInterface, const ObservableObject* obj, float zOffset);
  bool CheckIfGhostBlockVisible(BehaviorExternalInterface& behaviorExternalInterface, const ObservableObject* obj, float zOffset, bool& outsideFOV);
  
  // helper function for looking at ghost block location
  template<typename T>
  void LookAtGhostBlock(BehaviorExternalInterface& behaviorExternalInterface, bool backupFirst, void(T::*callback)(BehaviorExternalInterface&));
  
  // NOTE: uses s32 instead of ObjectID to match ICozmoBehaviorPoseBasedAcknowledgement's generic ids
  ObjectID _currTarget;

  // fake object to try to look at to check for possible stacks
  std::unique_ptr<ObservableObject> _ghostStackedObject;

  bool _shouldStart = false;
  
  bool _shouldCheckBelowTarget;
  bool _forceBackupWhenCheckingForStack = false;
  
  std::set<s32> _targets;
  
  std::set<IReactToObjectListener*> _objectListeners;
  
}; // class BehaviorAcknowledgeObject

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorAcknowledgeObject_H__
