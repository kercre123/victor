/**
 * File: behaviorReactToPet.h
 *
 * Description:  Simple reaction to a pet. Cozmo plays a reaction animation, then tracks the pet
 * for a random time interval.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_ReactToPet_H__
#define __Cozmo_Basestation_Behaviors_ReactToPet_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "coretech/vision/engine/faceIdTypes.h"
#include "clad/types/animationTrigger.h"
#include "clad/types/petTypes.h"

#include <set>

namespace Anki {
namespace Cozmo {

// Forward declarations
template<typename TYPE> class AnkiEvent;
namespace ExternalInterface {
  struct RobotObservedPet;
}

class BehaviorReactToPet : public ICozmoBehavior
{  
public:
  void SetTargets(const std::set<Vision::FaceID_t>& targets){_targets = targets;}

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToPet(const Json::Value& config);
  
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.behaviorAlwaysDelegates = false;
  }

  // IReactionaryBehavior
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  virtual void BehaviorUpdate() override;
  
  virtual bool WantsToBeActivatedBehavior() const override;

  virtual void AddListener(IReactToPetListener* listener) override;
  
private:
  using super = ICozmoBehavior;
  
  static constexpr float NEVER = -1.0f;
  
  // Everything we want to react to before we stop (to handle multiple targets in the same frame).
  // This member must be mutable to retain state from const method WantsToBeActivatedBehavior().
  std::set<Vision::FaceID_t> _targets;
  
  // Current target
  Vision::FaceID_t _target = Vision::UnknownFaceID;

  // Time to end current iteration
  float _endReactionTime_s = NEVER;
  
  // Stages of reaction
  void BeginIteration();
  void EndIteration();

  
  bool AlreadyReacting() const;
  
  AnimationTrigger GetAnimationTrigger(Vision::PetType petType);
  
  std::set<IReactToPetListener*> _petListeners;

}; // class BehaviorReactToPet

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToPet_H__
