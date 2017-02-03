/**
 * File: reactionTriggerStrategyFistBump.h
 *
 * Author: Kevin Yoon
 * Created: 01/26/17
 *
 * Description: Reaction Trigger strategy for doing fist bump
 *              both at the end of sparks or during freeplay as a celebratory behavior.
 *
 * Copyright: Anki, Inc. 2017
 *
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyFistBump_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyFistBump_H__

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "anki/common/basestation/jsonTools.h"
#include <unordered_map>


namespace Anki {
namespace Cozmo {

class ReactionTriggerStrategyFistBump : public IReactionTriggerStrategy{
public:
  ReactionTriggerStrategyFistBump(Robot& robot, const Json::Value& config);

  virtual bool ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;
  virtual bool ShouldResumeLastBehavior() const override { return false; }
  
protected:
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  
private:
  
  void LoadJson(const Json::Value& config);
  
  struct BehaviorObjectiveTriggerParams {
    float cooldownTime_s;      // Amount of time that must have expired since the last time fist bump completed before this can trigger it again
    float triggerProbability;  // Probability (0,1) of fist bump executing when objective is achieved and cooldown time has expired
  };
  
  std::unordered_map<BehaviorObjective, BehaviorObjectiveTriggerParams> _triggerParamsMap;
  
  // Whether or not fist bump should play
  bool _shouldTrigger;
  
  // The last time _shouldTrigger was set to true
  float _shouldTriggerSetTime_sec;
  
  // Amount of time since _shouldTrigger is set to true that it should automatically become false.
  // Playing the fist bump too long after the objective that it's triggered off of is achieved makes it confusing.
  float _shouldTriggerTimeout_sec;
  
  // The last time that the fist bump objective was achieved
  float _lastFistBumpCompleteTime_sec;
};


} // namespace Cozmo
} // namespace Anki

#endif //
