/**
 * File: BehaviorListenForBeats.h
 *
 * Author: Matt Michini
 * Created: 2018-08-20
 *
 * Description: Listens for beats from the BeatDetectorComponent, playing animations along the way
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorListenForBeats__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorListenForBeats__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.h"

namespace Anki {
namespace Vector {
  
enum class AnimationTrigger : int32_t;

class BehaviorListenForBeats : public ICozmoBehavior
{
public: 
  virtual ~BehaviorListenForBeats();

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorListenForBeats(const Json::Value& config);  

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;
  virtual void BehaviorUpdate() override;

private:

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config, const std::string& debugName);
    
    AnimationTrigger preListeningAnim;
    AnimationTrigger listeningAnim;
    AnimationTrigger postListeningAnim;
    
    float minListeningTime_sec = 0.f;
    float maxListeningTime_sec = 0.f;
    
    bool cancelSelfIfBeatLost = false;
  };

  struct DynamicVariables {
    float listeningStartTime_sec = -1.f;
  };
  
  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
  void TransitionToListening();
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorListenForBeats__
