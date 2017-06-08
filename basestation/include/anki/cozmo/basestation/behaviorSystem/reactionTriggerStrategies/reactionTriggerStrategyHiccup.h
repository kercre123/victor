/**
 * File: reactionTriggerStrategyHiccup.h
 *
 * Author: Al Chaussee
 * Created: 3/2/17
 *
 * Description: Reaction Trigger strategy for responding to hiccups
 *              Hiccups can be "cured" by putting Cozmo on his back/face
 *
 * Copyright: Anki, Inc. 2017
 *
 *
 **/
#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyHiccup_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyHiccup_H__

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/iReactionTriggerStrategy.h"
#include "anki/common/types.h"

namespace Anki {
namespace Cozmo {

class IExternalInterface;
class AIWhiteboard;
  
class ReactionTriggerStrategyHiccup : public IReactionTriggerStrategy
{
public:
  ReactionTriggerStrategyHiccup(Robot& robot, const Json::Value& config);
  virtual ~ReactionTriggerStrategyHiccup();
  
  virtual bool ShouldResumeLastBehavior() const override { return true;}
  virtual bool CanInterruptOtherTriggeredBehavior() const override { return false; }
  
  // Forcibly gives Cozmo the hiccups
  void ForceHiccups();

protected:
  virtual bool ShouldTriggerBehaviorInternal(const Robot& robot, const IBehavior* behavior) override;
  virtual void SetupForceTriggerBehavior(const Robot& robot, const IBehavior* behavior) override;

private:

  // Returns true if Cozmo currently has the hiccups
  bool HasHiccups() const;

  // Resets various timers and counters for hiccups
  void ResetHiccups();

  // Returns true if we can hiccup right now
  bool CanHiccup(const Robot& robot) const;
  
  // Resets hiccups and applies hiccupsWontOccurAfterBeingCuredTime
  void CureHiccups(bool playerCured);
  
  // Returns which animation to play depending on our state
  std::vector<AnimationTrigger> GetHiccupAnim() const;
  
  virtual void AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot) override;
  
  virtual void EnabledStateChanged(bool enabled) override;
  
  void ParseConfig(const Json::Value& config);

  // The next time a bout of hiccups should occur
  TimeStamp_t _shouldGetHiccupsAtTime = 0;
  
  // How many hiccups are left in the next/current bout
  u32 _numHiccupsLeftInBout   = 0;
  u32 _totalHiccupsInBout     = 0;
  
  // The next time to hiccup during this bout
  u32 _nextHiccupInBoutTime   = 0;
  
  // Whether or not this reaction has been disabled since the last call to ShouldTriggerBehavior
  // Used to prevent hiccuping right after being reenabled
  bool _reactionDisabled = false;
  
  enum class HiccupsCured
  {
    NotCured,    // Either don't have the hiccups or have the hiccups and not yet cured
    PendingCure, // Was put on face or back and am waiting to be back on treads
    PlayerCured, // OnTreads after being put on face/back, play player cure animation
    SelfCured    // Hiccups timed out
  };
  
  // How our hiccups were cured, used to determine which get out animation to play
  HiccupsCured _hiccupsCured = HiccupsCured::NotCured;
  
  IExternalInterface* _externalInterface = nullptr;
  bool _hasBroadcasted = false;
  
  Util::RandomGenerator& _rng;
  AIWhiteboard& _whiteboard;
  
  // The time when the first hiccup in a bout occured
  TimeStamp_t _firstHiccupStartTime = 0;

  // Json defined params
  // How often a bout of hiccups can occur
  u32 _minHiccupOccurrenceFrequency_ms = 0;
  u32 _maxHiccupOccurrenceFrequency_ms = 0;
  
  // How many hiccups to do per bout
  u32 _minNumberOfHiccupsToDo = 0;
  u32 _maxNumberOfHiccupsToDo = 0;
  
  // How far apart should the hiccups in a bout be
  u32 _minHiccupSpacing_ms = 0;
  u32 _maxHiccupSpacing_ms = 0;
  
  // Additional time added to _shouldGetHiccupsAtTime when Cozmo is "cured" of the hiccups
  // Prevents hiccups from occurring soon after being cured by the player
  u32 _hiccupsWontOccurAfterBeingCuredTime_ms = 0;
  
  UnlockId _hiccupsUnlockId = UnlockId::Count;
};
  
}
}

#endif //__Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyHiccup_H__
