/**
 * File: behaviorSayName.h
 *
 * Author: ross
 * Created: 2018-06-22
 *
 * Description: says the user's name
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSayName__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSayName__
#pragma once

#include "engine/aiComponent/behaviorComponent/behaviors/simpleFaceBehaviors/iSimpleFaceBehavior.h"

namespace Anki {
namespace Vector {
  
class BehaviorTextToSpeechLoop;

class BehaviorSayName : public ISimpleFaceBehavior
{
public: 
  virtual ~BehaviorSayName() = default;

protected:

  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  explicit BehaviorSayName(const Json::Value& config);

  virtual void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  virtual void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  virtual void GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const override;
  
  virtual void InitBehavior() override;
  virtual bool WantsToBeActivatedBehavior() const override;
  virtual void OnBehaviorActivated() override;

private:

  struct InstanceConfig {
    InstanceConfig(const Json::Value& config);
    
    // The animation to use while saying name. Must have special TTS keyframe!
    AnimationTrigger knowNameAnimation = AnimationTrigger::MeetVictorSayNameAgain;
    
    // The animation to use to indicate name is not known. No TTS keyframe!
    AnimationTrigger dontKnowNameAnimation = AnimationTrigger::MeetVictorSawWrongFace;
    
    // Text to speak *after* playing dontKnowNameAnimation (empty for none)
    std::string dontKnowText = "eye dont know";
    
    std::shared_ptr<BehaviorTextToSpeechLoop> ttsBehavior;
  };

  struct DynamicVariables {
    DynamicVariables();
    // TODO: put member variables here
  };

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_BehaviorSayName__
