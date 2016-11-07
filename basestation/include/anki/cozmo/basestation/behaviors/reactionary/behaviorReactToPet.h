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

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"
#include "anki/vision/basestation/faceIdTypes.h"
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

class BehaviorReactToPet : public IReactionaryBehavior
{
  
public:
  // IReactionaryBehavior
  virtual bool ShouldResumeLastBehavior() const override { return true; }
  virtual bool CarryingObjectHandledInternally() const override { return false; }

  // Behavior manager checks the return value of this function every tick
  // to see if the reactionary behavior has requested a computational switch.
  // Override to trigger a reactionary behavior based on something other than a message.
  virtual bool ShouldComputationallySwitch(const Robot& robot) override;

protected:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToPet(Robot& robot, const Json::Value& config);
  
  // IReactionaryBehavior
  virtual Result InitInternalReactionary(Robot& robot) override;
  virtual void   StopInternalReactionary(Robot& robot) override;
  virtual Status UpdateInternal(Robot& robot) override;
  virtual bool IsRunnableInternalReactionary(const Robot& robot) const override;

private:
  using super = IReactionaryBehavior;
  
  static constexpr float NEVER = -1.0f;
  
  // Everything we want to react to before we stop (to handle multiple targets in the same frame)
  std::set<Vision::FaceID_t> _targets;
  
  // Everything we have already reacted to
  std::set<Vision::FaceID_t> _reactedTo;
  
  // Current target
  Vision::FaceID_t _target = Vision::UnknownFaceID;

  // Time to end current iteration
  float _endReactionTime_s = NEVER;
  
  // Last time we reacted to any target
  float _lastReactionTime_s = NEVER;
  
  // Stages of reaction
  void BeginIteration(Robot& robot);
  void EndIteration(Robot& robot);
  
  // Internal helpers
  bool AlreadyReacting() const;
  bool RecentlyReacted() const;
  
  void InitReactedTo(const Robot& robot);
  void UpdateReactedTo(const Robot& robot);
  
  AnimationTrigger GetAnimationTrigger(Vision::PetType petType);

}; // class BehaviorReactToPet

  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorReactToPet_H__
