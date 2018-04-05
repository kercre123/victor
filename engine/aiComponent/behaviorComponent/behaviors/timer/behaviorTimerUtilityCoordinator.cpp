/**
* File: behaviorTimerUtilityCoordinator.cpp
*
* Author: Kevin M. Karol
* Created: 2/7/18
*
* Description: Behavior which coordinates timer related behaviors including setting the timer
* antics that the timer is still running and stopping the timer when it's ringing
*
* Copyright: Anki, Inc. 2018
*
**/


#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorTimerUtilityCoordinator.h"

#include "coretech/common/engine/jsonTools.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/timerUtility.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimGetInLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorProceduralClock.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"

namespace Anki {
namespace Cozmo {

namespace{
const char* kAnticConfigKey = "anticConfig";

// antic keys
const char* kRecurIntervalMinKey = "recurIntervalMin_s";
const char* kRecurIntervalMaxKey = "recurIntervalMax_s";
const char* kRuleMinKey = "ruleMin_s";
const char* kRuleMaxKey = "ruleMax_s";
}

class AnticTracker{
public:
  AnticTracker(const Json::Value& config);
  // Notify the tracker that an antic has started
  void PlayingAntic(BehaviorExternalInterface& bei);
  bool GetMinTimeTillNextAntic(BehaviorExternalInterface& bei, 
                               const TimerUtility::SharedHandle timer, 
                               int& outTime_s) const;
  bool GetMaxTimeTillNextAntic(BehaviorExternalInterface& bei, 
                               const TimerUtility::SharedHandle timer, 
                               int& outTime_s) const;

private:
  // Antic recurrances are defined using two criteria: 
  //   1) The time remaining on the timer over which the recurrance rule applies (defined by ruleMin and ruleMax) 
  //   2) The time range during which antics should occur during those times (defined by rucrInterval min/max)
  // E.G. With 10 - 5 mins left on the timer antics should play every 2-3 mins, 
  //   but from 5 - 1 mins they should happen every 30 seconds
  struct RecurranceEntry{
    int ruleMin_s;
    int ruleMax_s;
    int recurIntervalMin_s;
    int recurIntervalMax_s;
  };
  std::vector<RecurranceEntry> _recurranceRules;
  int _lastAnticPlayed_s = 0;

  std::vector<const RecurranceEntry>::const_iterator GetApplicableRule(const TimerUtility::SharedHandle timer) const;
};

///////////
/// AnticTracker
///////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnticTracker::AnticTracker(const Json::Value& config)
{
  const std::string debugStr = "AnticTracker.Constructor.InvalidConfig";
  for(auto& configEntry: config){
    RecurranceEntry entry;
    entry.recurIntervalMin_s = JsonTools::ParseUInt32(configEntry, kRecurIntervalMinKey, debugStr + kRecurIntervalMinKey);
    entry.recurIntervalMax_s = JsonTools::ParseUInt32(configEntry, kRecurIntervalMaxKey, debugStr + kRecurIntervalMinKey);
    if(!JsonTools::GetValueOptional(configEntry, kRuleMinKey, entry.ruleMin_s)){
      entry.ruleMin_s = 0;
    }
    if(!JsonTools::GetValueOptional(configEntry, kRuleMaxKey, entry.ruleMax_s)){
      entry.ruleMax_s = INT_MAX;
    }
    _recurranceRules.emplace_back(entry);
  }

  if(ANKI_DEV_CHEATS &&
     (_recurranceRules.begin() != _recurranceRules.end())){
    // verify that there aren't any overlaps in the recurrance rules and that they're in descending order
    auto previousIter = _recurranceRules.begin();
    for(auto iter = _recurranceRules.begin(); iter != _recurranceRules.end(); ++iter){
      ANKI_VERIFY(iter->ruleMax_s >= iter->ruleMin_s, debugStr.c_str(), 
                  "Rule invalid range: maxTime %d and min time %d",
                  iter->ruleMax_s, iter->ruleMin_s);
      ANKI_VERIFY(iter->recurIntervalMax_s >= iter->recurIntervalMin_s, debugStr.c_str(), 
                  "Rule invalid recurrance interval: maxTime %d and min time %d",
                  iter->recurIntervalMax_s, iter->recurIntervalMin_s);

      ANKI_VERIFY((iter == previousIter) ||
                  (iter->ruleMax_s <= previousIter->ruleMin_s), debugStr.c_str(), 
                  "Rule overlap or not in descending order: maxTime %d and min time %d",
                  iter->ruleMax_s, previousIter->ruleMin_s);
      previousIter = iter;
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnticTracker::PlayingAntic(BehaviorExternalInterface& bei)
{
  auto& timerUtility = bei.GetAIComponent().GetComponent<TimerUtility>(AIComponentID::TimerUtility);
  _lastAnticPlayed_s = timerUtility.GetSystemTime_s();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AnticTracker::GetMinTimeTillNextAntic(BehaviorExternalInterface& bei, 
                                           const TimerUtility::SharedHandle timer, 
                                           int& outTime_s) const
{
  auto iter = GetApplicableRule(timer);
  if(iter != _recurranceRules.end()){
    auto& timerUtility = bei.GetAIComponent().GetComponent<TimerUtility>(AIComponentID::TimerUtility);
    const int currentTime_s = timerUtility.GetSystemTime_s();
    const int timeSinceLastAntic = currentTime_s - _lastAnticPlayed_s;

    // use time since last played and min interval to determine min time till antic
    const int rawTime_s = iter->recurIntervalMin_s - timeSinceLastAntic;
    outTime_s = rawTime_s > 0 ? rawTime_s : 0;
    return true;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AnticTracker::GetMaxTimeTillNextAntic(BehaviorExternalInterface& bei,
                                           const TimerUtility::SharedHandle timer, 
                                           int& outTime_s) const
{
  auto iter = GetApplicableRule(timer);
  if(iter != _recurranceRules.end()){
    auto& timerUtility = bei.GetAIComponent().GetComponent<TimerUtility>(AIComponentID::TimerUtility);
    const int currentTime_s = timerUtility.GetSystemTime_s();
    const int timeSinceLastAntic = currentTime_s - _lastAnticPlayed_s;

    // use time since last played and min interval to determine min time till antic
    const int rawTime_s = iter->recurIntervalMax_s - timeSinceLastAntic;
    outTime_s = rawTime_s > 0 ? rawTime_s : 0;
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
auto AnticTracker::GetApplicableRule(const TimerUtility::SharedHandle timer) const -> std::vector<const RecurranceEntry>::const_iterator
{
  auto secsRemaining = timer->GetTimeRemaining_s();
  for(auto iter = _recurranceRules.begin(); iter != _recurranceRules.end(); ++iter){
    if(secsRemaining > iter->ruleMax_s){break;}
    if((secsRemaining < iter->ruleMax_s) && 
       (secsRemaining > iter->ruleMin_s)){
      return iter;
    }
  }
  return _recurranceRules.end();
}


///////////
/// BehaviorTimerUtilityCoordinator
///////////

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTimerUtilityCoordinator::BehaviorTimerUtilityCoordinator(const Json::Value& config)
: ICozmoBehavior(config)
{
  if(config.isMember(kAnticConfigKey)){
    _iParams.anticTracker = std::make_unique<AnticTracker>(config[kAnticConfigKey]);
  }else{
    Json::Value empty;
    _iParams.anticTracker = std::make_unique<AnticTracker>(empty);
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTimerUtilityCoordinator::~BehaviorTimerUtilityCoordinator()
{
  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(SingletonTimerSet),
                                 BEHAVIOR_CLASS(ProceduralClock),
                                 _iParams.setTimerBehavior);

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(SingletonTimerAntic),
                                 BEHAVIOR_CLASS(ProceduralClock),
                                 _iParams.timerAnticBehavior);

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(SingletonTimerRinging),
                                 BEHAVIOR_CLASS(AnimGetInLoop),
                                 _iParams.timerRingingBehavior);
  
  SetupTimerBehaviorFunctions();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iParams.setTimerBehavior.get());
  delegates.insert(_iParams.timerAnticBehavior.get());
  delegates.insert(_iParams.timerRingingBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTimerUtilityCoordinator::WantsToBeActivatedBehavior() const 
{
  auto& uic = GetBEI().GetAIComponent().GetBehaviorComponent().GetUserIntentComponent();
  const bool setTimerWantsToRun = uic.IsUserIntentPending(Anki::Cozmo::UserIntentTag::set_timer, *_lParams.setTimerIntent);
  const bool timerShouldRing    = TimerShouldRing();
  
  // Todo - need to have a distinction of polite interrupt on min time vs max time
  // for now, just use max as a hard cut criteria
  auto handle = GetTimerUtility().GetActiveTimer();
  int maxTimeTillAntic_s = INT_MAX;
  bool timeToRunAntic = _iParams.anticTracker->GetMaxTimeTillNextAntic(GetBEI(), handle, maxTimeTillAntic_s);
  timeToRunAntic &= (maxTimeTillAntic_s == 0);

  return setTimerWantsToRun || timeToRunAntic || timerShouldRing;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::OnBehaviorActivated() 
{
  const bool persistTimer = _lParams.timerSet;
  auto* persistIntentData = _lParams.setTimerIntent.release();
  _lParams = LifetimeParams();
  _lParams.timerSet = persistTimer;
  _lParams.setTimerIntent.reset(persistIntentData);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::BehaviorUpdate()
{
  if(!IsActivated()){
    return;
  }
 
  if(TimerShouldRing()){
    TransitionToRinging();
  }

  auto& uic = GetBEI().GetAIComponent().GetBehaviorComponent().GetUserIntentComponent();
  const bool robotPickedUp = GetBEI().GetRobotInfo().GetOffTreadsState() != OffTreadsState::OnTreads;
  const bool shouldCancelTimer = robotPickedUp || uic.IsTriggerWordPending();
  if(IsTimerRinging() && shouldCancelTimer){
    // Clear the pending trigger word and cancel the ringing timer
    // Its emergency get out will still play
    uic.ClearPendingTriggerWord();
    CancelSelf();
    return;
  }

  if(IsControlDelegated()){
    return;
  }

  if(_iParams.setTimerBehavior->WantsToBeActivated()){
    TransitionToSetTimer();
  }

  auto handle = GetTimerUtility().GetActiveTimer();
  int minTimeTillAntic_s = INT_MAX;
  const bool validAnticTime = _iParams.anticTracker->GetMinTimeTillNextAntic(GetBEI(), handle, minTimeTillAntic_s);

  // set clock digit quadrants
  if(validAnticTime && (minTimeTillAntic_s == 0) &&
     _iParams.timerAnticBehavior->WantsToBeActivated()){
    TransitionToPlayAntic();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTimerUtilityCoordinator::TimerShouldRing() const
{
  auto& timerUtility = GetTimerUtility();
  auto secRemain = timerUtility.GetActiveTimer()->GetTimeRemaining_s();
  return _lParams.timerSet && (secRemain == 0);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const TimerUtility& BehaviorTimerUtilityCoordinator::GetTimerUtility() const
{
  return GetBEI().GetAIComponent().GetComponent<TimerUtility>(AIComponentID::TimerUtility);
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::SetupTimerBehaviorFunctions() const
{
  using DigitID = BehaviorProceduralClock::DigitID;
  auto& timerUtility = GetBEI().GetAIComponent().GetComponent<TimerUtility>(AIComponentID::TimerUtility);
  
  auto startTimerCallback = [&timerUtility, this](){
    _lParams.timerSet = true;
    // Add one second to the user's requested timer so that the numbers show up on screen appropriately
    timerUtility.StartTimer(_lParams.setTimerIntent->Get_set_timer().time_s + 1);
  };

  _iParams.setTimerBehavior->SetShowClockCallback(startTimerCallback);

  std::map<DigitID, std::function<int()>> timerFuncs;
  // Ten Mins Digit
  {
    auto tenMinsFunc = [&timerUtility](){
      auto timerHandle = timerUtility.GetActiveTimer();
      const int minsRemaining = timerHandle->GetDisplayMinutesRemaining();
      return minsRemaining/10;
    };
    timerFuncs.emplace(std::make_pair(DigitID::DigitOne, tenMinsFunc));
  }
  // One Mins Digit
  {
    auto oneMinsFunc = [&timerUtility](){
      auto timerHandle = timerUtility.GetActiveTimer();
      const int minsRemaining = timerHandle->GetDisplayMinutesRemaining();
      return minsRemaining % 10;
    };
    timerFuncs.emplace(std::make_pair(DigitID::DigitTwo, oneMinsFunc));
  }
  // Ten seconds digit
  {
    auto tenSecsFunc = [&timerUtility](){
      auto timerHandle = timerUtility.GetActiveTimer();
      const int secsRemaining = timerHandle->GetDisplaySecondsRemaining();
      return secsRemaining/10;
    };
    timerFuncs.emplace(std::make_pair(DigitID::DigitThree, tenSecsFunc));
  }
  // One seconds digit
  {
    auto oneSecsFunc = [&timerUtility](){
      auto timerHandle = timerUtility.GetActiveTimer();
      const int secsRemaining = timerHandle->GetDisplaySecondsRemaining();
      return secsRemaining % 10;
    };
    timerFuncs.emplace(std::make_pair(DigitID::DigitFour, oneSecsFunc));
  }
  
  auto intentionalCopy = timerFuncs;
  _iParams.setTimerBehavior->SetDigitFunctions(std::move(timerFuncs));
  _iParams.timerAnticBehavior->SetDigitFunctions(std::move(intentionalCopy));
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::TransitionToSetTimer()
{
  _iParams.anticTracker->PlayingAntic(GetBEI());
  DelegateNow(_iParams.setTimerBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::TransitionToPlayAntic()
{
  _iParams.anticTracker->PlayingAntic(GetBEI());
  DelegateNow(_iParams.timerAnticBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::TransitionToRinging()
{
  _lParams.timerSet = false;
  _iParams.timerRingingBehavior->WantsToBeActivated();
  DelegateNow(_iParams.timerRingingBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTimerUtilityCoordinator::IsTimerRinging()
{ 
  return _iParams.timerRingingBehavior->IsActivated();
}


} // namespace Cozmo
} // namespace Anki
