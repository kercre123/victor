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

#include "clad/types/behaviorComponent/userIntent.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"

#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorAnimGetInLoop.h"
#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorAdvanceClock.h"
#include "engine/aiComponent/behaviorComponent/behaviors/timer/behaviorProceduralClock.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/behaviorComponent/userIntentData.h"
#include "engine/aiComponent/timerUtility.h"
#include "engine/aiComponent/timerUtilityDevFunctions.h"

#include "util/console/consoleInterface.h"

namespace Anki {
namespace Cozmo {

namespace{
const char* kAnticConfigKey   = "anticConfig";
const char* kMinValidTimerKey = "minValidTimer_s";
const char* kMaxValidTimerKey = "maxValidTimer_s";
const char* kTimerRingingBehaviorKey = "timerRingingBehaviorID";

// antic keys
const char* kRecurIntervalMinKey = "recurIntervalMin_s";
const char* kRecurIntervalMaxKey = "recurIntervalMax_s";
const char* kRuleMinKey = "ruleMin_s";
const char* kRuleMaxKey = "ruleMax_s";

Anki::Cozmo::BehaviorTimerUtilityCoordinator* sCoordinator = nullptr;
}


///////////
/// Dev/testing functions
///////////

#if ANKI_DEV_CHEATS
CONSOLE_VAR(u32, kAdvanceAnticSeconds,   "TimerUtility.AdvanceAnticSeconds", 10);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ForceAntic(ConsoleFunctionContextRef context)
{  
  if(sCoordinator != nullptr){
    sCoordinator->DevSetForceAntic();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AdvanceAntic(ConsoleFunctionContextRef context)
{  
  AdvanceAnticBySeconds(kAdvanceAnticSeconds);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AdvanceAnticBySeconds(int seconds)
{
  if(sCoordinator != nullptr){
    sCoordinator->DevAdvanceAnticBySeconds(seconds);
  }
}

CONSOLE_FUNC(ForceAntic, "TimerUtility.ForceAntic");
CONSOLE_FUNC(AdvanceAntic, "TimerUtility.AdvanceAntic");
#endif // ANKI_DEV_CHEATS

///////////
/// AnticTracker
///////////

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


  #if ANKI_DEV_CHEATS
  // "Advance" time by moving the last antic played back in time
  void AdvanceAnticBySeconds(u32 secondsToAdvance){ _lastAnticPlayed_s -= secondsToAdvance;}
  #endif 

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
  auto& timerUtility = bei.GetAIComponent().GetComponent<TimerUtility>();
  _lastAnticPlayed_s = timerUtility.GetSystemTime_s();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AnticTracker::GetMinTimeTillNextAntic(BehaviorExternalInterface& bei, 
                                           const TimerUtility::SharedHandle timer, 
                                           int& outTime_s) const
{
  auto iter = GetApplicableRule(timer);
  if(iter != _recurranceRules.end()){
    auto& timerUtility = bei.GetAIComponent().GetComponent<TimerUtility>();
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
    auto& timerUtility = bei.GetAIComponent().GetComponent<TimerUtility>();
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

#if ANKI_DEV_CHEATS
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::DevAdvanceAnticBySeconds(int seconds)
{
  _iParams.anticTracker->AdvanceAnticBySeconds(seconds);
}


#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTimerUtilityCoordinator::LifetimeParams::LifetimeParams()
{
  shouldForceAntic = false;
  tickToSuppressAnticFor = 0;
}


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

  if(config.isMember(kTimerRingingBehaviorKey)){
    _iParams.timerRingingBehaviorStr = config[kTimerRingingBehaviorKey].asString();
  }
  
  std::string debugStr = "BehaviorTimerUtilityCoordinator.Constructor.MissingConfig.";
  _iParams.minValidTimer_s = JsonTools::ParseUInt32(config, kMinValidTimerKey, debugStr + "MinTimer");
  _iParams.maxValidTimer_s = JsonTools::ParseUInt32(config, kMaxValidTimerKey, debugStr + "MaxTimer");

  // Theoretically we can allow multiple instances, but with current force antic implementation we
  // can't and will assert here
  if( sCoordinator != nullptr ) {
    PRINT_NAMED_WARNING("BehaviorTimerUtilityCoordinator.Constructor.MultipleInstances", "Multiple coordinator instances");
  }
  sCoordinator = this;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorTimerUtilityCoordinator::~BehaviorTimerUtilityCoordinator()
{
  sCoordinator = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kAnticConfigKey,
    kMinValidTimerKey,
    kMaxValidTimerKey,
    kTimerRingingBehaviorKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
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
                                
  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(SingletonTimerCheckTime),
                                 BEHAVIOR_CLASS(ProceduralClock),
                                 _iParams.timerCheckTimeBehavior);

  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(SingletonCancelTimer),
                                 BEHAVIOR_CLASS(AdvanceClock),
                                 _iParams.cancelTimerBehavior);

  auto ringingID = BEHAVIOR_ID(SingletonTimerRinging);
  if(!_iParams.timerRingingBehaviorStr.empty()){
    ringingID = BehaviorTypesWrapper::BehaviorIDFromString(_iParams.timerRingingBehaviorStr);
  }
  _iParams.timerRingingBehavior = BC.FindBehaviorByID(ringingID);
  DEV_ASSERT_MSG(_iParams.timerRingingBehavior != nullptr, 
                 "BehaviorTimerUtilityCoordinator.InitBehavior.InvalidRingingBehavior", 
                 "Unable to find behavior %s for timerRinging",
                 BehaviorTypesWrapper::BehaviorIDToString(ringingID));

  _iParams.timerAlreadySetBehavior = BC.FindBehaviorByID(BEHAVIOR_ID(SingletonTimerAlreadySet));
  _iParams.iCantDoThatBehavior     = BC.FindBehaviorByID(BEHAVIOR_ID(SingletonICantDoThat));
  SetupTimerBehaviorFunctions();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iParams.setTimerBehavior.get());
  delegates.insert(_iParams.timerAnticBehavior.get());
  delegates.insert(_iParams.timerRingingBehavior.get());
  delegates.insert(_iParams.timerAlreadySetBehavior.get());
  delegates.insert(_iParams.iCantDoThatBehavior.get());
  delegates.insert(_iParams.cancelTimerBehavior.get());
  delegates.insert(_iParams.timerCheckTimeBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTimerUtilityCoordinator::WantsToBeActivatedBehavior() const 
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool setTimerWantsToRun = uic.IsUserIntentPending(USER_INTENT(set_timer));
  const bool timerShouldRing    = TimerShouldRing();
  const bool cancelTimerPending = uic.IsUserIntentPending(USER_INTENT(cancel_timer));
  const bool checkTimePending = uic.IsUserIntentPending(USER_INTENT(check_timer));
  
  // Todo - need to have a distinction of polite interrupt on min time vs max time
  // for now, just use max as a hard cut criteria
  bool timeToRunAntic = false;
  if(auto handle = GetTimerUtility().GetTimerHandle()){
    int maxTimeTillAntic_s = INT_MAX;
    timeToRunAntic = _iParams.anticTracker->GetMaxTimeTillNextAntic(GetBEI(), handle, maxTimeTillAntic_s);
    timeToRunAntic &= (maxTimeTillAntic_s == 0);
  }
  
  // Override all antics if it's been suppressed
  const auto tickCount = BaseStationTimer::getInstance()->GetTickCount();
  if(_lParams.tickToSuppressAnticFor == tickCount){
    timeToRunAntic = false;
  }


  return cancelTimerPending || setTimerWantsToRun || 
         timeToRunAntic || timerShouldRing || 
         _lParams.shouldForceAntic || checkTimePending;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::OnBehaviorActivated() 
{
  bool  persistShouldForceAntic = _lParams.shouldForceAntic;
  _lParams = LifetimeParams();

  _lParams.shouldForceAntic = persistShouldForceAntic;
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

  CheckShouldCancelRinging();

  if(IsControlDelegated() || !IsActivated()){
    return;
  }

  CheckShouldSetTimer();
  CheckShouldCancelTimer();
  CheckShouldPlayAntic();
  CheckShouldShowTimeRemaining();
  
  _lParams.shouldForceAntic = false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTimerUtilityCoordinator::TimerShouldRing() const
{
  auto handle = GetTimerUtility().GetTimerHandle();
  auto secRemain = (handle != nullptr) ? handle->GetTimeRemaining_s() : 0;
  return (handle != nullptr) && (secRemain == 0);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::CheckShouldCancelRinging()
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  const bool robotPickedUp = GetBEI().GetRobotInfo().GetOffTreadsState() != OffTreadsState::OnTreads;
  const bool shouldCancelTimer = robotPickedUp || uic.IsTriggerWordPending();
  if(IsTimerRinging() && shouldCancelTimer){
    GetTimerUtility().ClearTimer();
    // Clear the pending trigger word and cancel the ringing timer
    // Its emergency get out will still play
    uic.ClearPendingTriggerWord();
    CancelSelf();
    return;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::CheckShouldSetTimer()
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if(uic.IsUserIntentPending(USER_INTENT(set_timer))){
    UserIntentPtr intentData = SmartActivateUserIntent(USER_INTENT(set_timer));

    int requestedTime_s = intentData->intent.Get_set_timer().time_s;
    const bool isTimerInRange = (_iParams.minValidTimer_s <= requestedTime_s) && 
                                (requestedTime_s          <= _iParams.maxValidTimer_s);

    if(GetTimerUtility().GetTimerHandle() != nullptr){
      // Timer already set - can't set another
      TransitionToTimerAlreadySet();
    }else if(isTimerInRange){
      TransitionToSetTimer();
    }else{
      TransitionToInvalidTimerRequest();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::CheckShouldCancelTimer()
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if(uic.IsUserIntentPending(USER_INTENT(cancel_timer))){
    SmartActivateUserIntent(USER_INTENT(cancel_timer));

    // Cancel a timer if it is set, otherwise play "I Cant Do That"
    auto handle = GetTimerUtility().GetTimerHandle();
    if(handle != nullptr){
      const auto timeRemaining_s = handle->GetTimeRemaining_s();
      const auto displayTime = _iParams.cancelTimerBehavior->GetTimeDisplayClock_sec();
      _iParams.cancelTimerBehavior->SetAdvanceClockParams(timeRemaining_s, 0, displayTime);
      
      GetTimerUtility().ClearTimer();
      TransitionToCancelTimer();
    }else{
      TransitionToNoTimerToCancel();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::CheckShouldShowTimeRemaining()
{
  auto& uic = GetBehaviorComp<UserIntentComponent>();
  if(uic.IsUserIntentPending(USER_INTENT(check_timer))){
    SmartActivateUserIntent(USER_INTENT(check_timer));
    if(auto handle = GetTimerUtility().GetTimerHandle()){
      TransitionToPlayAntic();
    }else{
      TransitionToInvalidTimerRequest();
    }
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::CheckShouldPlayAntic()
{
  if(auto handle = GetTimerUtility().GetTimerHandle()){
    int minTimeTillAntic_s = INT_MAX;
    const bool validAnticTime = _iParams.anticTracker->GetMinTimeTillNextAntic(GetBEI(), handle, minTimeTillAntic_s);

    // set clock digit quadrants
    if((validAnticTime && (minTimeTillAntic_s == 0)) ||
       _lParams.shouldForceAntic){
      TransitionToPlayAntic();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::TransitionToSetTimer()
{
  _iParams.anticTracker->PlayingAntic(GetBEI());
  _iParams.setTimerBehavior->WantsToBeActivated();
  DelegateNow(_iParams.setTimerBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::TransitionToPlayAntic()
{
  _iParams.anticTracker->PlayingAntic(GetBEI());
  _iParams.timerAnticBehavior->WantsToBeActivated();
  DelegateNow(_iParams.timerAnticBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::TransitionToShowTimeRemaining()
{
  _iParams.timerAlreadySetBehavior->WantsToBeActivated();
  DelegateNow(_iParams.timerAlreadySetBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::TransitionToRinging()
{
  GetTimerUtility().ClearTimer();
  _iParams.timerRingingBehavior->WantsToBeActivated();
  DelegateNow(_iParams.timerRingingBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::TransitionToTimerAlreadySet()
{
  _iParams.timerAlreadySetBehavior->WantsToBeActivated();
  DelegateNow(_iParams.timerAlreadySetBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::TransitionToNoTimerToCancel()
{
  _iParams.iCantDoThatBehavior->WantsToBeActivated();
  DelegateNow(_iParams.iCantDoThatBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::TransitionToCancelTimer()
{
  _iParams.cancelTimerBehavior->WantsToBeActivated();
  DelegateNow(_iParams.cancelTimerBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::TransitionToInvalidTimerRequest()
{
  _iParams.iCantDoThatBehavior->WantsToBeActivated();
  DelegateNow(_iParams.iCantDoThatBehavior.get());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorTimerUtilityCoordinator::IsTimerRinging()
{ 
  return _iParams.timerRingingBehavior->IsActivated();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::SuppressAnticThisTick(unsigned long tickCount)
{
  _lParams.tickToSuppressAnticFor = tickCount;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimerUtility& BehaviorTimerUtilityCoordinator::GetTimerUtility() const
{
  return GetBEI().GetAIComponent().GetComponent<TimerUtility>();
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorTimerUtilityCoordinator::SetupTimerBehaviorFunctions()
{  
  auto startTimerCallback = [this](){
    auto& uic = GetBehaviorComp<UserIntentComponent>();
    UserIntentPtr intentData = uic.GetUserIntentIfActive(USER_INTENT(set_timer));

    if( ANKI_VERIFY(intentData != nullptr &&
                    (intentData->intent.GetTag() == USER_INTENT(set_timer)),
                    "BehaviorTimerUtilityCoordinator.SetShowClockCallback.IncorrectIntent",
                    "Active user intent should have been set_timer but wasn't") ) {
      
      auto& timerUtility = GetBEI().GetAIComponent().GetComponent<TimerUtility>();
      timerUtility.StartTimer(intentData->intent.Get_set_timer().time_s);
    }
  };

  _iParams.setTimerBehavior->SetShowClockCallback(startTimerCallback);

  _iParams.setTimerBehavior->SetGetDigitFunction(BuildTimerFunction());
  _iParams.timerAnticBehavior->SetGetDigitFunction(BuildTimerFunction());
  _iParams.timerCheckTimeBehavior->SetGetDigitFunction(BuildTimerFunction());
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorProceduralClock::GetDigitsFunction BehaviorTimerUtilityCoordinator::BuildTimerFunction() const
{
  auto& timerUtility = GetBEI().GetAIComponent().GetComponent<TimerUtility>();
  return [&timerUtility](const int offset){
    std::map<Vision::SpriteBoxName, int> outMap;
    if(auto timerHandle = timerUtility.GetTimerHandle()){
      const int timeRemaining_s = timerHandle->GetTimeRemaining_s() - offset;
      const bool displayingHoursOnScreen = timerHandle->SecondsToDisplayHours(timeRemaining_s) > 0;
      
      const int hoursRemaining = TimerHandle::SecondsToDisplayHours(timeRemaining_s);
      const int minsRemaining = TimerHandle::SecondsToDisplayMinutes(timeRemaining_s);
      const int secsRemaining = TimerHandle::SecondsToDisplaySeconds(timeRemaining_s);

      // Tens Digit (left of colon)
      {
        int tensDigit = 0;
        if(displayingHoursOnScreen){
          // display as minutes
          tensDigit = (hoursRemaining * 60)/10;
        }else{
          tensDigit = minsRemaining/10;
        }
        outMap.emplace(std::make_pair(Vision::SpriteBoxName::TensLeftOfColon, tensDigit));
      }
      
      // Ones Digit (left of colon)
      {
        int onesDigit = 0;
        if(displayingHoursOnScreen){
          // display as minutes
          onesDigit = (hoursRemaining * 60) % 10;
        }else{
          onesDigit = minsRemaining % 10;
        }
        outMap.emplace(std::make_pair(Vision::SpriteBoxName::OnesLeftOfColon, onesDigit));
      }

      // Tens Digit (right of colon)
      {
        int tensDigit = 0;
        if(displayingHoursOnScreen){
          tensDigit = minsRemaining/10;
        }else{
          tensDigit = secsRemaining/10;
        }
        outMap.emplace(std::make_pair(Vision::SpriteBoxName::TensRightOfColon, tensDigit));
      }

      // Ones Digit (right of colon)
      {
        int onesDigit = 0;
        if(displayingHoursOnScreen){
          onesDigit = minsRemaining % 10;
        }else{
          onesDigit = secsRemaining % 10;
        }
        outMap.emplace(std::make_pair(Vision::SpriteBoxName::OnesRightOfColon, onesDigit));
      }

      return outMap;
    }else{
      return outMap;
    }
  };
}


} // namespace Cozmo
} // namespace Anki
