/**
 * File: behaviorOnConfigSeen.h
 *
 * Author: Kevin M. Karol
 * Created: 11/2/16
 *
 * Description: Scored behavior which plays a specified animation when it sees
 * a new block configuration
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_Behaviors_BehaviorOnConfigSeen_H__
#define __Cozmo_Basestation_Behaviors_BehaviorOnConfigSeen_H__

#include "anki/cozmo/basestation/behaviors/behaviorInterface.h"

namespace Anki {
namespace Cozmo {
// forward declarations
enum class AnimationTrigger;
namespace BlockConfigurations{
enum class ConfigurationType;
}
  
class BehaviorOnConfigSeen: public IBehavior
{
private:
  // Enforce creation through BehaviorFactory
  friend class BehaviorFactory;
  BehaviorOnConfigSeen(Robot& robot, const Json::Value& config);
  
  
public:
  virtual bool IsRunnableInternal(const Robot& robot) const override;
  virtual bool CarryingObjectHandledInternally() const override {return false;}
  
protected:
  virtual Result InitInternal(Robot& robot) override;
  // We shouldn't play the animation a second time if it's interrupted so simply return RESULT_OK
  virtual Result ResumeInternal(Robot& robot) override { return RESULT_OK;}
  
  void TransitionToPlayAnimationSequence(Robot& robot);

private:
  // Configurations to look for set in json
  mutable std::map<BlockConfigurations::ConfigurationType, int> _configurationCountMap;
  std::vector<AnimationTrigger> _animTriggers;
  
  int _animTriggerIndex;
  mutable float _lastRunnableCheck_s;
  
  
  void ReadJson(const Json::Value& config);
  
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_Behaviors_BehaviorOnConfigSeen_H__
