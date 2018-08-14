/**
 * File: BehaviorSleepCycle.cpp
 *
 * Author: Brad
 * Created: 2018-08-13
 *
 * Description: Top level behavior to coordinate sleep / wake cycles of the robot
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/sleeping/behaviorSleepCycle.h"

#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/aiComponent/behaviorComponent/activeFeatureComponent.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviorTypesWrapper.h"
#include "engine/aiComponent/behaviorComponent/sleepTracker.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "engine/aiComponent/beiConditions/conditions/conditionLambda.h"
#include "engine/aiComponent/timerUtility.h"
#include "engine/components/sdkComponent.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/moodSystem/moodManager.h"
#include "util/cladHelpers/cladFromJSONHelpers.h"
#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"
#include "util/logging/logging.h"
#include "webServerProcess/src/webService.h"
#include "webServerProcess/src/webVizSender.h"

namespace Anki {
namespace Vector {

namespace {
  const char* kBehaviorIDConfigKey = "delegateID";
  const char* kWakeReasonConditionsKey = "wakeReasonConditions";
  const char* kAlwaysWakeReasonsKey = "alwaysWakeReasons";
  const char* kWakeFromStatesKey = "wakeFromStates";
  const char* kFindChargerBehaviorKey = "goToChargerBehavior";

  const int kMaxTicksForPostBehaviorSuggestions = 5;

#if REMOTE_CONSOLE_ENABLED
  static bool sForcePersonCheck = false;

  void ForcePersonCheck(ConsoleFunctionContextRef context)
  {
    sForcePersonCheck = true;
  }
#endif

  static const char* kWebVizModuleNameSleeping = "sleeping";
}

#define CONSOLE_GROUP "Sleeping"

CONSOLE_VAR(f32, kSleepCycle_DeepSleep_PersonCheckInterval_s, CONSOLE_GROUP, 4 * 60.0f * 60.0f);
CONSOLE_VAR(f32, kSleepCycle_LightSleep_PersonCheckInterval_s, CONSOLE_GROUP, 1 * 60.0f * 60.0f);

CONSOLE_VAR(f32, kSleepCycle_ComatoseLength_s, CONSOLE_GROUP, 2 * 60.0f);

// this is like a "min time awake" for naturally falling asleep
CONSOLE_VAR(f32, kSleepCycle_RecentSleepLength_s, CONSOLE_GROUP, 10 * 60.0f);

// minimum amount of sleep debt, to avoid quickly waking up (naturally)
CONSOLE_VAR(f32, kSleepCycle_MinSleepDebt_s, CONSOLE_GROUP, 50 * 60.0f);

CONSOLE_VAR(bool, kSleepCycle_EnableWiggleWhileSleeping, CONSOLE_GROUP, true);

CONSOLE_VAR(bool, kSleepCycleForceSleep, CONSOLE_GROUP, false);
CONSOLE_FUNC(ForcePersonCheck, CONSOLE_GROUP);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSleepCycle::BehaviorSleepCycle(const Json::Value& config)
 : ICozmoBehavior(config)
{
  auto debugStr = "BehaviorSleepCycle.Constructor.MissingDelegateID";
  _iConfig.awakeDelegateName = JsonTools::ParseString(config, kBehaviorIDConfigKey, debugStr);
  _iConfig.findChargerBehaviorName = JsonTools::ParseString(config, kFindChargerBehaviorKey, debugStr);

  ParseWakeReasonConditions(config);
  CreateCustomWakeReasonConditions();
  CheckWakeReasonConfig();

  ParseWakeReasons(config);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorSleepCycle::~BehaviorSleepCycle()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::ParseWakeReasonConditions(const Json::Value& config)
{
  for( const auto& group : config[kWakeReasonConditionsKey] ) {
    WakeReason reason = WakeReason::Invalid;
    JsonTools::GetCladEnumFromJSON(group, "reason", reason, "BehaviorSleepCycle.ParseWakeReasonConditions");
    auto condition = BEIConditionFactory::CreateBEICondition(group["condition"], GetDebugLabel());
    if( ANKI_VERIFY(condition != nullptr,
                    "BehaviorSleepCycle.ParseWakeReasonConditions.FailedToCrateCondition",
                    "Failed to get a valid condition for reason '%s'",
                    EnumToString(reason)) ) {
      _iConfig.wakeConditions.emplace( reason, condition );
    }
  }

  PRINT_CH_INFO("Behaviors", "BehaviorSleepCycle.ParseWakeReasonConditions.Parsed",
                "Parsed %zu wake reason conditions from json",
                _iConfig.wakeConditions.size());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::CreateCustomWakeReasonConditions()
{

  _iConfig.wakeConditions.emplace( WakeReason::SDK, new ConditionLambda( [](BehaviorExternalInterface& bei) {
      auto& robotInfo = bei.GetRobotInfo();
      auto& sdkComponent = robotInfo.GetSDKComponent();
      const bool wantsControl = sdkComponent.SDKWantsControl();
      return wantsControl;
    },
    GetDebugLabel() ) );

  _iConfig.wakeConditions.emplace( WakeReason::TimerShouldRing, new ConditionLambda( [](BehaviorExternalInterface& bei) {
      auto& utility = bei.GetAIComponent().GetComponent<TimerUtility>();
      auto handle = utility.GetTimerHandle();
      auto secRemain = (handle != nullptr) ? handle->GetTimeRemaining_s() : 0;
      const bool shouldRing = (handle != nullptr) && (secRemain == 0);
      return shouldRing;
    },
    GetDebugLabel() ) );

  _iConfig.wakeConditions.emplace( WakeReason::VoiceCommand,
                                   new ConditionLambda( [this](BehaviorExternalInterface& bei) {
      const bool isIntentPending = GetBehaviorComp<UserIntentComponent>().IsAnyUserIntentPending();
      const bool isListening = ( _dVars.reactionState == SleepReactionType::TriggerWord );
      return isIntentPending && !isListening;
    },
    GetDebugLabel() ) );

  _iConfig.wakeConditions.emplace( WakeReason::Sound, new ConditionLambda( [this](BehaviorExternalInterface& bei) {
      const bool isDoneReacting = (_dVars.reactionState == SleepReactionType::Sound && !IsControlDelegated());
      return isDoneReacting;
    },
    GetDebugLabel() ) );

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::CheckWakeReasonConfig()
{
  // check through the wake conditions to make sure everything is accounted for

  // start at 1 to skip "invalid"
  for( uint8_t reasonIdx = 1; reasonIdx < WakeReasonNumEntries; ++reasonIdx ) {
    const WakeReason reason = (WakeReason)reasonIdx;

    if( reason != WakeReason::SawPerson &&
        reason != WakeReason::NotSleepy ) {
      // skip special cases, everything else must exist

      ANKI_VERIFY( _iConfig.wakeConditions.find(reason) != _iConfig.wakeConditions.end(),
                   "BehaviorSleepCycle.WakeReasonCondition.Missing",
                   "Missing wake condition for reason '%s'",
                   WakeReasonToString(reason) );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::ParseWakeReasons(const Json::Value& config)
{
  for( const auto& reasonJson : config[kAlwaysWakeReasonsKey] ) {
    WakeReason reason = WakeReason::Invalid;
    if( JsonTools::ParseCladEnumFromJSON(reasonJson, reason, "BehaviorSleepCycle.ParseWakeReasons") ) {
      _iConfig.alwaysWakeReasons.push_back(reason);
    }
  }

  PRINT_CH_INFO("Behaviors", "BehaviorSleepCycle.ParseWakeReasons.AlwaysWakeReasons",
                "Parsed %zu 'always wake' reasons",
                _iConfig.alwaysWakeReasons.size());

  for( const auto& stateGroup : config[kWakeFromStatesKey] ) {
    SleepStateID fromState = SleepStateID::Invalid;
    JsonTools::GetCladEnumFromJSON(stateGroup, "fromState", fromState, "BehaviorSleepCycle.ParseWakeStates.FromState");

    auto& wakeReasons = _iConfig.wakeReasonsPerState[ fromState ];

    for( const auto& reasonJson : stateGroup["reasons"] ) {
      WakeReason reason = WakeReason::Invalid;
      if( JsonTools::ParseCladEnumFromJSON(reasonJson, reason, "BehaviorSleepCycle.ParseWakeReasons") ) {
        wakeReasons.push_back(reason);
      }
    }

    PRINT_CH_INFO("Behaviors", "BehaviorSleepCycle.ParseWakeReasons.WakeReasonsFromState",
                  "Parsed %zu additional reasons to wake from state '%s'",
                  wakeReasons.size(),
                  SleepStateIDToString(fromState));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSleepCycle::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  // always wants to be activated
  modifiers.wantsToBeActivatedWhenCarryingObject = true;
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.awakeDelegate.get());
  delegates.insert(_iConfig.awakeDelegate.get());
  delegates.insert(_iConfig.goToSleepBehavior.get());
  delegates.insert(_iConfig.asleepBehavior.get());
  delegates.insert(_iConfig.wakeUpBehavior.get());
  delegates.insert(_iConfig.personCheckBehavior.get());
  delegates.insert(_iConfig.findChargerBehavior.get());
  delegates.insert(_iConfig.sleepingSoundReactionBehavior.get());
  delegates.insert(_iConfig.sleepingWakeWordBehavior.get());
  delegates.insert(_iConfig.wiggleBackOntoChargerBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kBehaviorIDConfigKey,
    kWakeReasonConditionsKey,
    kAlwaysWakeReasonsKey,
    kWakeFromStatesKey,
    kFindChargerBehaviorKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::InitBehavior()
{
  auto& BC = GetBEI().GetBehaviorContainer();

  {
    BehaviorID delegateID = BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.awakeDelegateName);
    _iConfig.awakeDelegateName.clear();
    _iConfig.awakeDelegate = BC.FindBehaviorByID(delegateID);
  }

  {
    BehaviorID delegateID = BehaviorTypesWrapper::BehaviorIDFromString(_iConfig.findChargerBehaviorName);
    _iConfig.findChargerBehaviorName.clear();
    _iConfig.findChargerBehavior = BC.FindBehaviorByID( delegateID );
  }

  _iConfig.asleepBehavior                = BC.FindBehaviorByID( BEHAVIOR_ID( Asleep ) );
  _iConfig.goToSleepBehavior             = BC.FindBehaviorByID( BEHAVIOR_ID( GoToSleep ) );
  _iConfig.wakeUpBehavior                = BC.FindBehaviorByID( BEHAVIOR_ID( SleepingWakeUp ) );
  _iConfig.sleepingSoundReactionBehavior = BC.FindBehaviorByID( BEHAVIOR_ID( ReactToSoundAsleep ) );
  _iConfig.sleepingWakeWordBehavior      = BC.FindBehaviorByID( BEHAVIOR_ID( SleepingTriggerWord ) );
  _iConfig.wiggleBackOntoChargerBehavior = BC.FindBehaviorByID( BEHAVIOR_ID( WiggleBackOntoChargerFromPlatform ) );
  _iConfig.personCheckBehavior           = BC.FindBehaviorByID( BEHAVIOR_ID( SleepingPersonCheck ) );

  for( const auto& conditionPair : _iConfig.wakeConditions ) {
    conditionPair.second->Init( GetBEI() );
  }

  const auto* context = GetBEI().GetRobotInfo().GetContext();
  if( context != nullptr ) {
    auto* webService = context->GetWebService();
    if( webService != nullptr ) {
      auto onWebVizSubscribed = [this](const std::function<void(const Json::Value&)>& sendToClient) {
        // just got a subscription, send now
        Json::Value data;
        PopulateWebVizJson(data);
        sendToClient(data);
      };
      _eventHandles.emplace_back( webService->OnWebVizSubscribed( kWebVizModuleNameSleeping ).ScopedSubscribe(
                                    onWebVizSubscribed ));
    }
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::OnBehaviorActivated()
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  // set all "always running" conditions to active
  for( const auto& reason : _iConfig.alwaysWakeReasons ) {
    _iConfig.wakeConditions[reason]->SetActive( GetBEI(), true );
  }

  SetState( SleepStateID::Awake );

  // starts in "awake", so delegate right away to awake behavior
  if( _iConfig.awakeDelegate->WantsToBeActivated() ) {
    DelegateIfInControl(_iConfig.awakeDelegate.get());
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::OnBehaviorDeactivated()
{
  // if we were asleep, and being canceled by something above, play emergency get out (it is disabled in the
  // sleeping behavior itself)
  switch( _dVars.currState ) {
    case SleepStateID::Invalid:
    case SleepStateID::Awake:
    case SleepStateID::PreSleepAnimation:
    case SleepStateID::GoingToCharger:
    case SleepStateID::CheckingForPerson:
      // no need to play get out from here
      break;

    case SleepStateID::Comatose:
    case SleepStateID::DeepSleep:
    case SleepStateID::LightSleep:
      PlayEmergencyGetOut(AnimationTrigger::WakeupGetout);
      break;
  }

  // set all "always running" conditions to non-active
  for( const auto& reason : _iConfig.alwaysWakeReasons ) {
    _iConfig.wakeConditions[reason]->SetActive( GetBEI(), false );
  }

  SetConditionsActiveForState( _dVars.currState, false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::BehaviorUpdate()
{
  if( ! IsActivated() ) {
    return;
  }

#if REMOTE_CONSOLE_ENABLED
  if( sForcePersonCheck ) {
    PRINT_NAMED_WARNING("BehaviorSleepCycle.ConsoleFunc.ForcingPersonCheck",
                        "forcing a person check from state '%s' (if we are in light or deep sleep)",
                        SleepStateIDToString(_dVars.currState));
    _dVars.nextPersonCheckTime_s = 0.01f;
    sForcePersonCheck = false;
  }
#endif

  if( _dVars.currState == SleepStateID::Invalid ) {
    PRINT_NAMED_ERROR("BehaviorSleepCycle.Update.InvalidState",
                      "sleep state invalid");
    return;
  }

  if( _dVars.reactionState == SleepReactionType::TriggerWord ) {
    // trigger word response (from sleep) is active.
    if( IsControlDelegated() ) {
      // wait for the listening behavior to end
      return;
    }
    else {
      // finished listening behavior, clear reaction state
      _dVars.reactionState = SleepReactionType::None;

      // go on with the loop here, wake reason will wake us if needed
    }
  }

  if( _dVars.currState == SleepStateID::Awake ) {
    GoToSleepIfNeeded();
  }
  else {
    // some sleeping state (or go to charger / person check) is active

    // first check wake word
    if( _iConfig.sleepingWakeWordBehavior->WantsToBeActivated() ) {
      CancelDelegates(false);
      _dVars.reactionState = SleepReactionType::TriggerWord;
      DelegateIfInControl(_iConfig.sleepingWakeWordBehavior.get());
    }

    // check always-on wake conditions
    for( const auto& reason : _iConfig.alwaysWakeReasons ) {
      if( WakeIfNeeded( reason ) ) {
        return;
      }
    }

    // not awake from any of the global conditions, check any conditions based on the current sleep state
    const auto it = _iConfig.wakeReasonsPerState.find( _dVars.currState );
    if( it != _iConfig.wakeReasonsPerState.end() ) {
      for( const auto& reason : it->second ) {
        if( WakeIfNeeded( reason ) ) {
          return;
        }
      }
    }

    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

    if( _dVars.currState == SleepStateID::Comatose &&
        currTime_s > _dVars.comatoseStartTime_s + kSleepCycle_ComatoseLength_s ) {
      // since we only get to comatose from a voice command, always go to deep sleep, even if it's daytime
      TransitionToDeepSleep();
      return;
    }

    if( _dVars.currState == SleepStateID::LightSleep ||
        _dVars.currState == SleepStateID::DeepSleep ) {

      if( _dVars.nextPersonCheckTime_s > 0.0f &&
          currTime_s >= _dVars.nextPersonCheckTime_s ) {
        PRINT_CH_INFO("Behaviors", "BehaviorSleepCycle.TimeForPersonCheck",
                      "In state '%s', reached t=%f (>= %f), time to do a person check",
                      SleepStateIDToString( _dVars.currState ),
                      currTime_s,
                      _dVars.nextPersonCheckTime_s);
        TransitionToCheckingForPerson();
        return;
      }
    }

    if( _dVars.reactionState == SleepReactionType::None &&
        ShouldReactToSoundInState(_dVars.currState) &&
        _iConfig.sleepingSoundReactionBehavior->WantsToBeActivated() ) {
      CancelDelegates(false);
      _dVars.reactionState = SleepReactionType::Sound;
      DelegateIfInControl(_iConfig.sleepingSoundReactionBehavior.get());
      return;
    }

    if( ShouldWiggleOntoChargerFromSleep() ) {
      CancelDelegates(false);
      _dVars.reactionState = SleepReactionType::WiggleOntoCharger;;

      // manually disable face keep alive to keep us sleeping while backing up
      SmartDisableKeepFaceAlive();

      DelegateIfInControl(_iConfig.wiggleBackOntoChargerBehavior.get(), [this]() {
          // even if this fails, clear the variable so we don't keep trying
          _dVars.wasOnChargerContacts = false;
          _dVars.reactionState = SleepReactionType::None;
          if( _dVars.currState != SleepStateID::Awake ) {
            // go back to slip, but skip the get in
            const bool playGetIn = false;
            SleepIfInControl(playGetIn);

            // remove our keep alive disable lock
            SmartReEnableKeepFaceAlive();
          }
        });
      return;
    }

    if( _dVars.reactionState == SleepReactionType::None &&
        !_dVars.wasOnChargerContacts &&
        GetBEI().GetRobotInfo().IsOnChargerContacts() ) {
      _dVars.wasOnChargerContacts = true;
    }


    if( !IsControlDelegated() ) {
      // in a sleeping state but not sleeping, re-start the behavior here (this may happen, e.g. if the wake
      // word behavior completed without a valid intent)
      SleepIfInControl();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSleepCycle::ShouldWiggleOntoChargerFromSleep()
{
  if( !kSleepCycle_EnableWiggleWhileSleeping ) {
    return false;
  }

  const bool notReacting = ( _dVars.reactionState == SleepReactionType::None );
  const bool onContacts = GetBEI().GetRobotInfo().IsOnChargerContacts();

  return notReacting &&
    _dVars.wasOnChargerContacts &&
    !onContacts &&
    _iConfig.wiggleBackOntoChargerBehavior->WantsToBeActivated();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSleepCycle::GoToSleepIfNeeded()
{

  if( kSleepCycleForceSleep ) {
    TransitionToLightOrDeepSleep();
    SendToGoSleepDasEvent(SleepReason::ConsoleVar);
    return true;
  }

  auto& uic = GetBehaviorComp<UserIntentComponent>();

  const bool isSleepIntentPending = uic.IsUserIntentPending( USER_INTENT( system_sleep ) );
  if( isSleepIntentPending ) {
    SmartActivateUserIntent( USER_INTENT( system_sleep ) );
    TransitionToComatose();
    SendToGoSleepDasEvent(SleepReason::SleepCommand);
    return true;
  }

  const bool isGoodnightIntentPending = uic.IsUserIntentPending( USER_INTENT( greeting_goodnight ) );
  if( isGoodnightIntentPending ) {
    SmartActivateUserIntent( USER_INTENT( greeting_goodnight ) );
    TransitionToSayingGoodnight();
    SendToGoSleepDasEvent(SleepReason::GoodnightCommand);
    return true;
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool wokeRecently = _dVars.lastWakeUpTime_s > 0.0f &&
    ( currTime_s - _dVars.lastWakeUpTime_s ) <= kSleepCycle_RecentSleepLength_s;

  // if "react to dark" has finished (or some other behavior that makes the robot want to sleep), it will
  // offer a post behavior suggestion. Check that here
  auto checkSuggestion = [this](PostBehaviorSuggestions suggestion) {
    auto& whiteboard = GetAIComp<AIWhiteboard>();
    size_t tickSuggested = 0;
    if( whiteboard.GetPostBehaviorSuggestion( suggestion, tickSuggested) ) {
      size_t currTick = BaseStationTimer::getInstance()->GetTickCount();
      if( tickSuggested + kMaxTicksForPostBehaviorSuggestions >= currTick ) {
        whiteboard.ClearPostBehaviorSuggestions();
        return true;
      }
    }
    return false;
  };

  if( !wokeRecently && checkSuggestion(PostBehaviorSuggestions::SleepOnCharger) ) {
    TransitionToCharger();
    SendToGoSleepDasEvent(SleepReason::SleepOnChargerSuggestion);
    return true;
  }

  if( !wokeRecently && checkSuggestion(PostBehaviorSuggestions::Sleep) ) {
    TransitionToLightOrDeepSleep();
    SendToGoSleepDasEvent(SleepReason::SleepSuggestion);
    return true;
  }

  const bool alreadyAsleep = false;
  const bool isSleepy = GetBehaviorComp<SleepTracker>().IsSleepy( alreadyAsleep );
  const bool lowStim = GetBEI().GetMoodManager().GetSimpleMood() == SimpleMoodType::LowStim;
  const ActiveFeature currActiveFeature = GetBehaviorComp<ActiveFeatureComponent>().GetActiveFeature();
  const bool isObserving = ( currActiveFeature == ActiveFeature::Observing) ||
                           ( currActiveFeature == ActiveFeature::ObservingOnCharger );

  if( isSleepy &&
      lowStim &&
      isObserving &&
      !wokeRecently ) {
    TransitionToCharger();
    SendToGoSleepDasEvent(SleepReason::Sleepy);
    return true;
  }

  // no reason to sleep, stay awake
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSleepCycle::WakeIfNeeded(const WakeReason& forReason)
{
  auto conditionIt = _iConfig.wakeConditions.find( forReason );
  if( ANKI_VERIFY( conditionIt != _iConfig.wakeConditions.end() &&
                   conditionIt->second != nullptr ,
                   "BehaviorSleepCycle.WakeIfNeeded.MissingReasonCondition",
                   "Dont have a condition for reason '%s'",
                   EnumToString( forReason ) ) ) {
    if( conditionIt->second->AreConditionsMet( GetBEI() ) ) {
      const bool defaulVal = false;
      const bool playAnim = ShouldPlayWakeUpAnimForReason( forReason, defaulVal );
      WakeUp( forReason, playAnim );
      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::SendToGoSleepDasEvent(const SleepReason& reason)
{
  if( _dVars.currState == SleepStateID::Awake ) {
    PRINT_NAMED_WARNING("BehaviorSleepCycle.GoToSleep.InvalidDASState",
                        "Going to sleep for reason '%s', but state is still 'Awake'",
                        SleepReasonToString(reason));
  }

  DASMSG(goto_sleep, "behavior.sleeping.falling_asleep", "The robot is preparing to fall asleep");
  DASMSG_SET(s1, SleepReasonToString(reason), "The reason the robot is falling asleep");
  DASMSG_SET(s2, SleepStateIDToString(_dVars.currState), "The state of sleeping we are going to");
  DASMSG_SEND();

#if ANKI_DEV_CHEATS
  _dVars.lastSleepReason = reason;
#endif

  auto* context = GetBEI().GetRobotInfo().GetContext();
  if( context ) {
    if( auto webSender = WebService::WebVizSender::CreateWebVizSender(kWebVizModuleNameSleeping,
                                                                      context->GetWebService()) ) {
      PopulateWebVizJson(webSender->Data());
    }
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::WakeUp(const WakeReason& reason, bool playWakeUp)
{

  PRINT_CH_INFO("Behaviors", "BehaviorSleepCycle.WakeUp",
                "Waking up from '%s' because of reason '%s'",
                SleepStateIDToString(_dVars.currState),
                WakeReasonToString(reason));

  DASMSG(wakeup, "behavior.sleeping.wake_up", "Robot is waking up from sleep");
  DASMSG_SET(s1, WakeReasonToString(reason), "The reason for waking (see SleepingTypes.clad)");
  DASMSG_SET(s2, SleepStateIDToString(_dVars.currState), "The state of sleeping we were in before waking");
  DASMSG_SEND();

  // never play wake up if we were in the middle of a reaction (we're already visibly "awake" in that case)
  const bool wasPlayingReaction = _dVars.reactionState != SleepReactionType::None;

  SetState(SleepStateID::Awake);

  // clear charger contacts (may be on or off now)
  _dVars.wasOnChargerContacts = false;

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.lastWakeUpTime_s = currTime_s;

#if ANKI_DEV_CHEATS
  _dVars.lastWakeReason = reason;
#endif

  auto awakeCallback = [this]() {
    if( _iConfig.awakeDelegate->WantsToBeActivated() ) {
      DelegateNow( _iConfig.awakeDelegate.get() );
    }
  };

  if( !wasPlayingReaction &&
      playWakeUp &&
      _iConfig.wakeUpBehavior->WantsToBeActivated() ) {
    DelegateNow( _iConfig.wakeUpBehavior.get(), awakeCallback);
  }
  else {
    awakeCallback();
  }

  auto* context = GetBEI().GetRobotInfo().GetContext();
  if( context ) {
    if( auto webSender = WebService::WebVizSender::CreateWebVizSender(kWebVizModuleNameSleeping,
                                                                      context->GetWebService()) ) {
      PopulateWebVizJson(webSender->Data());
    }
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::SetConditionsActiveForState(SleepStateID state, bool active)
{
  const auto it = _iConfig.wakeReasonsPerState.find( state );
  if( it != _iConfig.wakeReasonsPerState.end() ) {
    for( const auto& reason : it->second ) {
      _iConfig.wakeConditions[reason]->SetActive( GetBEI(), active );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToCheckingForPerson()
{
  SetState(SleepStateID::CheckingForPerson);

  // sleep states will set the next time appropriately, for now disable the next check
  _dVars.nextPersonCheckTime_s = -1.0f;

  // note: for now this will cause a wake up animation.... might be nice to do a different animation or not
  // wake up here
  CancelDelegates(false);

  _dVars.personCheckStartTimestamp = GetBEI().GetRobotInfo().GetLastImageTimeStamp();

  if( _iConfig.personCheckBehavior->WantsToBeActivated() ) {
    DelegateIfInControl( _iConfig.personCheckBehavior.get(), &BehaviorSleepCycle::RespondToPersonCheck );
  }
  else {
    PRINT_NAMED_WARNING("BehaviorSleepCycle.PersonCheck.BehaviorWontActivate",
                        "Behavior '%s' doesnt want to activate",
                        _iConfig.personCheckBehavior->GetDebugLabel().c_str());
    RespondToPersonCheck();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::RespondToPersonCheck()
{
  // if there's a face, time to wake up
  const bool onlyRecognizable = true;
  if( GetBEI().GetFaceWorld().HasAnyFaces(_dVars.personCheckStartTimestamp, onlyRecognizable) ) {
    WakeUp(WakeReason::SawPerson, false);
    return;
  }

  auto& sleepTracker = GetBehaviorComp< SleepTracker >();

  // no face, is it time to wake up?
  const bool fromSleep = true;
  if( !sleepTracker.IsSleepy(fromSleep) &&
      !sleepTracker.IsNightTime() ) {
    WakeUp(WakeReason::NotSleepy, false);
    return;
  }

  // not time to wake up
  TransitionToLightOrDeepSleep();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToComatose()
{
  if( _dVars.currState == SleepStateID::Awake ) {
    // not going to be awake anymore
    CancelDelegates(false);
  }

  SetState(SleepStateID::Comatose);

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.comatoseStartTime_s = currTime_s;

  SleepIfInControl();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToDeepSleep()
{
  if( _dVars.currState == SleepStateID::Awake ) {
    CancelDelegates(false);
  }

  SetState(SleepStateID::DeepSleep);

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.nextPersonCheckTime_s = currTime_s + kSleepCycle_DeepSleep_PersonCheckInterval_s;

  SleepIfInControl();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToLightSleep()
{
  if( _dVars.currState == SleepStateID::Awake ) {
    CancelDelegates(false);
  }

  SetState(SleepStateID::LightSleep);

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.nextPersonCheckTime_s = currTime_s + kSleepCycle_LightSleep_PersonCheckInterval_s;

  SleepIfInControl();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::SleepIfInControl(bool playGetIn)
{
  if( !IsControlDelegated() ) {

    auto playAsleepBehavior = [this]() {
      if( _iConfig.asleepBehavior->WantsToBeActivated() ) {
        DelegateIfInControl( _iConfig.asleepBehavior.get() );
      }
      else {
        PRINT_NAMED_WARNING("BehaviorSleepCycle.SleepIfInControl.CantSleep",
                            "Asleep behavior '%s' doesn't want to be activated",
                            _iConfig.asleepBehavior->GetDebugLabel().c_str());
      }
    };

    if( playGetIn ) {
      if( _iConfig.goToSleepBehavior->WantsToBeActivated() ) {
        DelegateIfInControl( _iConfig.goToSleepBehavior.get(), playAsleepBehavior );
      }
      else {
        PRINT_NAMED_WARNING("BehaviorSleepCycle.SleepIfInControl.CantGoToSleep",
                            "GoToSleep behavior '%s' doesn't want to be activated",
                            _iConfig.goToSleepBehavior->GetDebugLabel().c_str());
        playAsleepBehavior();
      }
    }
    else {
      // go straight to sleeping
      playAsleepBehavior();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToSayingGoodnight()
{
  SetState(SleepStateID::PreSleepAnimation);

  if( IsControlDelegated() ) {
    CancelDelegates(false);
  }

  DelegateIfInControl(new TriggerLiftSafeAnimationAction(AnimationTrigger::ReactToGoodNight),
                      &BehaviorSleepCycle::TransitionToCharger);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToCharger()
{
  if( GetBEI().GetRobotInfo().IsOnChargerContacts() ) {
    // skip straight to sleeping
    TransitionToLightOrDeepSleep();
  }
  else {
    CancelDelegates(false);
    if( _iConfig.findChargerBehavior->WantsToBeActivated() ) {
      SetState(SleepStateID::GoingToCharger);
      DelegateIfInControl( _iConfig.findChargerBehavior.get(),
                           &BehaviorSleepCycle::TransitionToLightOrDeepSleep );
    }
    else {
      PRINT_NAMED_WARNING("BehaviorSleepCycle.TransitionToCharger.WontRun",
                          "Not on charger contacts, but find charger behavior '%s' doesnt want to activate",
                          _iConfig.findChargerBehavior->GetDebugLabel().c_str());
      TransitionToLightOrDeepSleep();
    }
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToLightOrDeepSleep()
{
  auto& sleepTracker = GetBehaviorComp< SleepTracker >();
  sleepTracker.EnsureSleepDebtAtLeast( kSleepCycle_MinSleepDebt_s );

  // decide if we should go to light or deep sleep
  const bool isNightTime = sleepTracker.IsNightTime();
  if( isNightTime ) {
    TransitionToDeepSleep();
  }
  else {
    TransitionToLightSleep();
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::SetState(SleepStateID state)
{
  SetConditionsActiveForState(_dVars.currState, false);

  _dVars.currState = state;
  PRINT_CH_INFO("Behaviors", "BehaviorSleepCycle.SetState",
                "%s",
                SleepStateIDToString(state));

  SetConditionsActiveForState(_dVars.currState, true);

  auto* context = GetBEI().GetRobotInfo().GetContext();
  if( context ) {
    if( auto webSender = WebService::WebVizSender::CreateWebVizSender(kWebVizModuleNameSleeping,
                                                                      context->GetWebService()) ) {
      PopulateWebVizJson(webSender->Data());
    }
  }

  // if the state changes, our sleeping reaction will not be active (or be canceled) so clear here
  _dVars.reactionState = SleepReactionType::None;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSleepCycle::ShouldReactToSoundInState(const SleepStateID& state)
{
  switch(state) {
    case SleepStateID::Invalid:
      return false;

    case SleepStateID::Awake:
      // this is specifically for the "react from asleep", so don't do it if we're awake
      return false;

    case SleepStateID::PreSleepAnimation:
    case SleepStateID::GoingToCharger:
    case SleepStateID::Comatose:
    case SleepStateID::DeepSleep:
      return false;

    case SleepStateID::LightSleep:
    case SleepStateID::CheckingForPerson:
      return true;
  }
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::PopulateWebVizJson(Json::Value& data) const
{
#if ANKI_DEV_CHEATS
  if( IsActivated() ) {
    data["sleep_cycle"] = SleepStateIDToString(_dVars.currState);

    if( _dVars.lastWakeReason != WakeReason::Invalid ) {
      data["last_wake_reason"] = WakeReasonToString(_dVars.lastWakeReason);
    }

    if( _dVars.lastSleepReason != SleepReason::Invalid ) {
      data["last_sleep_reason"] = SleepReasonToString(_dVars.lastSleepReason);
    }
  }
#endif
}

}
}
