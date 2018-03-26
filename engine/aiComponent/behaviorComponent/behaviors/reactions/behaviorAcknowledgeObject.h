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

#include "coretech/common/engine/objectIDs.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/shared/radians.h"
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
  
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorAcknowledgeObject(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenCarryingObject = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;
  virtual void OnBehaviorDeactivated() override;

  virtual bool WantsToBeActivatedBehavior() const override;
  
  virtual void AddListener(IReactToObjectListener* listener) override;

  
private:
  void BeginIteration();
  void LookForStackedCubes();
  void FinishIteration();
  
  // helper functions to set the ghost object's pose
  void SetGhostBlockPoseRelObject(const ObservableObject* obj, float zOffset);
  
  // helper function to check stack visibility, with optional output argument for whether
  // the ghost cube is outside the current FOV
  bool CheckIfGhostBlockVisible(const ObservableObject* obj, float zOffset);
  bool CheckIfGhostBlockVisible(const ObservableObject* obj, float zOffset, bool& outsideFOV);
  
  // helper function for looking at ghost block location
  template<typename T>
  void LookAtGhostBlock(bool backupFirst, void(T::*callback)());
  
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
