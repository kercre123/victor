/**
 * File: behaviorReactToPalmEdge.h
 *
 * Author: Guillermo Bautista
 * Created: 03/25/2019
 *
 * Description: Behavior for immediately responding to a robot driving more both front cliff sensors
 *              or both rear cliff sensors over an edge when the robot is held in a user's palm.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/
#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToPalmEdge_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToPalmEdge_H__

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"
#include "engine/components/sensors/cliffSensorComponent.h"

namespace Anki {
namespace Vector {

class ICompoundAction;
  
class BehaviorReactToPalmEdge : public ICozmoBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorReactToPalmEdge(const Json::Value& config);
  
public:  
  virtual bool WantsToBeActivatedBehavior() const override;
  
protected:
  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override {
    modifiers.wantsToBeActivatedWhenOffTreads = true;
  }
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;

  virtual void InitBehavior() override;
  virtual void OnBehaviorActivated() override;
  virtual void OnBehaviorDeactivated() override;
  
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  
  virtual void BehaviorUpdate() override;

private:  
  void TransitionToStuckOnPalmEdge();
  void TransitionToPlayingInitialReaction();
  void TransitionToRecoveryBackup();
  // Based on which cliff sensor(s) was tripped, create the appropriate reaction to face the edge
  void TransitionToFaceAndBackAwayFromEdge();
  void TransitionToPlayingJoltReaction();
  
  // Based on which cliff sensor(s) was tripped, create the appropriate initial reaction
  IActionRunner* GetInitialEdgeReactAction(const uint8_t cliffDetectedFlags);
  
  enum class State {
    Activating,
    PlayingInitialReaction,
    FaceAndBackAwayFromEdge,
    RecoveryBackup,
    StuckOnPalmEdge,
    PlayingJoltReaction,
    Inactive
  };
  
  void SetState_internal(State state, const std::string& stateName);
  
  struct InstanceConfig {
    InstanceConfig();
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    
    ICozmoBehaviorPtr askForHelpBehavior;
    ICozmoBehaviorPtr joltInPalmReaction;
    
    float cliffBackupDist_mm;
    float cliffBackupSpeed_mmps;
    
    AnimationTrigger animOnActionFailure;
  };

  InstanceConfig _iConfig;

  struct DynamicVariables {
    DynamicVariables();
    bool canBeInterruptedByJolt;
    bool hasTriggeredMoodEventForJolt;

    struct Persistent {
      State state;
      int numInitialReactAttempts;
      u8 cliffFlagsAtStart;
    };
    Persistent persistent;
  };
  
  DynamicVariables _dVars;
  
}; // class BehaviorReactToPalmEdge
  

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorReactToPalmEdge_H__
