/**
 * File: BehaviorCoordinateWhileHeldInPalm.h
 *
 * Author: Guillermo Bautista
 * Created: 2019-01-14
 *
 * Description: Behavior responsible for managing the robot's behaviors when held in a user's palm/hand
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateWhileHeldInPalm__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateWhileHeldInPalm__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/dispatch/behaviorDispatcherPassThrough.h"
#include "engine/aiComponent/behaviorComponent/behaviorTreeStateHelpers.h"
#include "engine/engineTimeStamp.h"

namespace Anki {
namespace Vector {

// Forward declarations:
class BehaviorReactToVoiceCommand;

class BehaviorCoordinateWhileHeldInPalm : public BehaviorDispatcherPassThrough
{
public: 
  virtual ~BehaviorCoordinateWhileHeldInPalm();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorCoordinateWhileHeldInPalm(const Json::Value& config);
  virtual void GetPassThroughJsonKeys(std::set<const char*>& expectedKeys) const override;
  virtual void InitPassThrough() override;
  virtual void OnPassThroughActivated() override;
  virtual void OnFirstPassThroughUpdate() override;
  virtual void PassThroughUpdate() override;
  virtual void OnPassThroughDeactivated() override;
  
  virtual void OnBehaviorLeftActivatableScope() override;
  
private:
  
  struct InstanceConfig {
    InstanceConfig() {}
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    
    ICozmoBehaviorPtr heldInPalmDispatcher;
    ICozmoBehaviorPtr initialHeldInPalmReaction;
    ICozmoBehaviorPtr heldInPalmSleepingBehavior;
    
    std::shared_ptr<BehaviorReactToVoiceCommand> heldInPalmTriggerWordBehavior;
    
    std::set<BehaviorID> behaviorStatesToSuppressHeldInPalmReactions;
    std::unique_ptr<AreBehaviorsActivatedHelper> suppressHeldInPalmBehaviorSet;
    
    std::set<BehaviorID> behaviorsToSuppressWhenSleepingInPalm;
    std::unordered_set<ICozmoBehaviorPtr> sleepSuppressedBehaviorSet;
  };
  
  InstanceConfig _iConfig;
  
  struct DynamicVariables {
    DynamicVariables() {}
    
    struct Persistent {
      // For tracking initial pickup reaction
      bool hasInitialHIPReactionPlayed = false;
      
      bool hasStartedSleepingInPalm = false;
      bool hasSetNewTriggerWordListeningAnims = false;
    };
    Persistent persistent;
  };
  
  DynamicVariables _dVars;
  
  void SuppressHeldInPalmReactionsIfAppropriate();
  void SuppressInitialHeldInPalmReactionIfAppropriate();
  void SuppressNonGentleWakeUpBehaviorsIfAppropriate();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorCoordinateWhileHeldInPalm__
