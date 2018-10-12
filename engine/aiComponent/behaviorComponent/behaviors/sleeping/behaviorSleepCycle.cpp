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

#include "clad/types/behaviorComponent/streamAndLightEffect.h"
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
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/sdkComponent.h"
#include "engine/cozmoContext.h"
#include "engine/faceWorld.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/sayNameProbabilityTable.h"
#include "osState/osState.h"
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
  const char* kEmergencyConditionKey = "emergencyCondition";

  const int kMaxTicksForPostBehaviorSuggestions = 5;
  const int kSecondsThatMeanRecentBoot = 20.0f;

#if REMOTE_CONSOLE_ENABLED
  static bool sForcePersonCheck = false;

  void ForcePersonCheck(ConsoleFunctionContextRef context)
  {
    sForcePersonCheck = true;
  }
#endif

  static const char* kWebVizModuleNameSleeping = "sleeping";
}

#define CONSOLE_GROUP "Sleeping.SleepCycle"

CONSOLE_VAR(f32, kSleepCycle_DeepSleep_PersonCheckInterval_s, CONSOLE_GROUP, 4 * 60.0f * 60.0f);
CONSOLE_VAR(f32, kSleepCycle_LightSleep_PersonCheckInterval_s, CONSOLE_GROUP, 1 * 60.0f * 60.0f);

CONSOLE_VAR(f32, kSleepCycle_ComatoseLength_s, CONSOLE_GROUP, 2 * 60.0f);

// this is like a "min time awake" for naturally falling asleep
CONSOLE_VAR(f32, kSleepCycle_RecentSleepLength_s, CONSOLE_GROUP, 10 * 60.0f);

// minimum amount of sleep debt, to avoid quickly waking up (naturally)
CONSOLE_VAR(f32, kSleepCycle_MinSleepDebt_s, CONSOLE_GROUP, 50 * 60.0f);

CONSOLE_VAR(bool, kSleepCycle_EnableWiggleWhileSleeping, CONSOLE_GROUP, true);

CONSOLE_VAR(bool, kSleepCycleForceSleep, CONSOLE_GROUP, false);

CONSOLE_VAR(bool, kSleepCycleForceLightSleep, CONSOLE_GROUP, false);

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

  _iConfig.emergencyCondition = BEIConditionFactory::CreateBEICondition(config[kEmergencyConditionKey], GetDebugLabel());
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
        reason != WakeReason::NotSleepy &&
        reason != WakeReason::Interrupted ) {
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
  delegates.insert(_iConfig.emergencyModeAnimBehavior.get());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kBehaviorIDConfigKey,
    kWakeReasonConditionsKey,
    kAlwaysWakeReasonsKey,
    kWakeFromStatesKey,
    kFindChargerBehaviorKey,
    kEmergencyConditionKey
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
  _iConfig.emergencyModeAnimBehavior     = BC.FindBehaviorByID( BEHAVIOR_ID( EmergencyModeAnimDispatcher ) );
  _iConfig.personCheckBehavior           = BC.FindBehaviorByID( BEHAVIOR_ID( SleepingPersonCheck ) );

  for( const auto& conditionPair : _iConfig.wakeConditions ) {
    conditionPair.second->Init( GetBEI() );
  }

  _iConfig.emergencyCondition->Init( GetBEI() );

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

  _iConfig.emergencyCondition->SetActive( GetBEI(), true );

  // if we just rebooted, and it's night time, then start out asleep
  const bool shouldStartAsleep = WasNightlyReboot();

  PRINT_CH_INFO("Behaviors", "BehaviorSleepCycle.Activated",
                "Starting out %s",
                shouldStartAsleep ? "asleep" : "awake");

  if( shouldStartAsleep ) {
    TransitionToLightOrDeepSleep();
    SendGoToSleepDasEvent(SleepReason::NightTimeReboot);
  }
  else {
    SetState( SleepStateID::Awake );

    // starts in "awake", so delegate right away to awake behavior
    if( _iConfig.awakeDelegate->WantsToBeActivated() ) {
      DelegateIfInControl(_iConfig.awakeDelegate.get());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::OnBehaviorDeactivated()
{
  if( _dVars.isMuted ) {
    MuteForPersonCheck(false);
  }

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
    case SleepStateID::EmergencySleep:
    case SleepStateID::DeepSleep:
    case SleepStateID::LightSleep: {

      PlayEmergencyGetOut(AnimationTrigger::WakeupGetout);

#if ANKI_DEV_CHEATS
      _dVars.lastWakeReason = WakeReason::Interrupted;

      auto* context = GetBEI().GetRobotInfo().GetContext();
      if( context ) {
        if( auto webSender = WebService::WebVizSender::CreateWebVizSender(kWebVizModuleNameSleeping,
                                                                          context->GetWebService()) ) {
          PopulateWebVizJson(webSender->Data());
        }
      }
#endif
      break;
    }
  }

  // set all "always running" conditions to non-active
  for( const auto& reason : _iConfig.alwaysWakeReasons ) {
    _iConfig.wakeConditions[reason]->SetActive( GetBEI(), false );
  }

  SetConditionsActiveForState( _dVars.currState, false);

  _iConfig.emergencyCondition->SetActive( GetBEI(), false );
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

      auto clearReactionState = [this]() {
        SetReactionState(SleepReactionType::NotReacting);
      };

      if( _dVars.currState == SleepStateID::EmergencySleep ) {
        // show "charger face" or "too hot face" animation to explain to the user why the voice command failed
        if( _iConfig.emergencyModeAnimBehavior->WantsToBeActivated() ) {
          if( DelegateIfInControl(_iConfig.emergencyModeAnimBehavior.get(), clearReactionState) ) {
            return;
          }
        }
        PRINT_NAMED_WARNING("BehaviorSleepCycle.BehaviorUpdate.CantPlayEmergencyAnim",
                            "For some reason, emergency anim behavior '%s' wont activate",
                            _iConfig.emergencyModeAnimBehavior ?
                            _iConfig.emergencyModeAnimBehavior->GetDebugLabel().c_str() :
                            "<NULL>");
        clearReactionState();
        return;
      }
      else {
        // not an emergency, so we're done with the reaction now, wake reason will wake us if needed (e.g. an
        // intent is set)
        clearReactionState();
      }
    }
  }

  if( _dVars.currState != SleepStateID::EmergencySleep &&
      _iConfig.emergencyCondition->AreConditionsMet( GetBEI() ) ) {
    TransitionToEmergencySleep();
    SendGoToSleepDasEvent(SleepReason::Emergency);
    return;
  }

  if( _dVars.currState == SleepStateID::Awake ) {
    GoToSleepIfNeeded();
  }
  else {
    // some sleeping state (or go to charger / person check) is active

    // first check wake word
    if( _iConfig.sleepingWakeWordBehavior->WantsToBeActivated() ) {
      CancelDelegates(false);
      SetReactionState(SleepReactionType::TriggerWord);
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

    if( _dVars.currState == SleepStateID::EmergencySleep ) {
      if( !_iConfig.emergencyCondition->AreConditionsMet( GetBEI() ) ) {

        // no longer in emergency sleep. If a voice command was activated for sleeping (before we went into
        // emergency mode) go to deep sleep, otherwise select the right sleep automatically
        auto& uic = GetBehaviorComp<UserIntentComponent>();
        if( uic.IsUserIntentActive( USER_INTENT( system_sleep ) ) ) {
          TransitionToDeepSleep();
        }
        else {
          TransitionToLightOrDeepSleep();
        }

        return;
      }
    }

    if( _dVars.currState == SleepStateID::Comatose ) {
      const bool timeToTransition = currTime_s > _dVars.comatoseStartTime_s + kSleepCycle_ComatoseLength_s;
      if( timeToTransition ) {
        // We only get into Comatose from the sleeping voice command, so always go directly to deep sleep
        TransitionToDeepSleep();
        return;
      }
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

    if( _dVars.reactionState == SleepReactionType::NotReacting &&
        ShouldReactToSoundInState(_dVars.currState) &&
        _iConfig.sleepingSoundReactionBehavior->WantsToBeActivated() ) {
      CancelDelegates(false);
      SetReactionState(SleepReactionType::Sound);
      DelegateIfInControl(_iConfig.sleepingSoundReactionBehavior.get());
      return;
    }

    if( ShouldWiggleOntoChargerFromSleep() ) {
      CancelDelegates(false);
      SetReactionState(SleepReactionType::WiggleOntoCharger);

      // manually disable face keep alive to keep us sleeping while backing up
      SmartDisableKeepFaceAlive();

      DelegateIfInControl(_iConfig.wiggleBackOntoChargerBehavior.get(), [this]() {
          // even if this fails, clear the variable so we don't keep trying
          _dVars.wasOnChargerContacts = false;
          SetReactionState(SleepReactionType::NotReacting);
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

    if( _dVars.reactionState == SleepReactionType::NotReacting &&
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

  const bool notReacting = ( _dVars.reactionState == SleepReactionType::NotReacting );
  const bool onContacts = GetBEI().GetRobotInfo().IsOnChargerContacts();

  return notReacting &&
    _dVars.wasOnChargerContacts &&
    !onContacts &&
    _iConfig.wiggleBackOntoChargerBehavior->WantsToBeActivated();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSleepCycle::WasNightlyReboot() const
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool engineStartedRecently = currTime_s < kSecondsThatMeanRecentBoot;
  const bool wasReboot = OSState::getInstance()->RebootedForMaintenance();
  const bool isNight = GetBEI().GetSleepTracker().IsNightTime();
  const bool wasNightlyReboot = engineStartedRecently && wasReboot && isNight;

  return wasNightlyReboot;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorSleepCycle::GoToSleepIfNeeded()
{

  if( kSleepCycleForceSleep ) {
    TransitionToLightOrDeepSleep();
    SendGoToSleepDasEvent(SleepReason::ConsoleVar);
    return true;
  }

  auto& uic = GetBehaviorComp<UserIntentComponent>();

  const bool isSleepIntentPending = uic.IsUserIntentPending( USER_INTENT( system_sleep ) );
  if( isSleepIntentPending ) {
    SmartActivateUserIntent( USER_INTENT( system_sleep ) );
    TransitionToComatose();
    SendGoToSleepDasEvent(SleepReason::SleepCommand);
    return true;
  }

  const bool isGoodnightIntentPending = uic.IsUserIntentPending( USER_INTENT( greeting_goodnight ) );
  if( isGoodnightIntentPending ) {
    SmartActivateUserIntent( USER_INTENT( greeting_goodnight ) );
    TransitionToSayingGoodnight();
    SendGoToSleepDasEvent(SleepReason::GoodnightCommand);
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
    SendGoToSleepDasEvent(SleepReason::SleepOnChargerSuggestion);
    return true;
  }

  if( !wokeRecently && checkSuggestion(PostBehaviorSuggestions::Sleep) ) {
    TransitionToLightOrDeepSleep();
    SendGoToSleepDasEvent(SleepReason::SleepSuggestion);
    return true;
  }

  const bool alreadyAsleep = false;
  const bool isSleepy = GetBEI().GetSleepTracker().IsSleepy( alreadyAsleep );
  const bool lowStim = GetBEI().GetMoodManager().GetSimpleMood() == SimpleMoodType::LowStim;
  const ActiveFeature currActiveFeature = GetBehaviorComp<ActiveFeatureComponent>().GetActiveFeature();
  const bool isObserving = ( currActiveFeature == ActiveFeature::Observing) ||
                           ( currActiveFeature == ActiveFeature::ObservingOnCharger );

  if( isSleepy &&
      lowStim &&
      isObserving &&
      !wokeRecently ) {
    TransitionToCharger();
    SendGoToSleepDasEvent(SleepReason::Sleepy);
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
      const bool defaultVal = false;
      const bool playAnim = ShouldPlayWakeUpAnimForReason( forReason, defaultVal );
      WakeUp( forReason, playAnim );
      return true;
    }
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::SendGoToSleepDasEvent(const SleepReason& reason)
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
  const bool wasPlayingReaction = _dVars.reactionState != SleepReactionType::NotReacting;

  SetState(SleepStateID::Awake);

  // if we went to sleep due to a voice intent, clear that now
  SmartDeactivateUserIntent();

  // clear charger contacts (may be on or off now)
  _dVars.wasOnChargerContacts = false;

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.lastWakeUpTime_s = currTime_s;

#if ANKI_DEV_CHEATS
  _dVars.lastWakeReason = reason;
#endif

  GetAIComp<AIWhiteboard>().OnRobotWakeUp();
  
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

  // Mute audio and lock the lift so that the person check is as quiet as possible
  MuteForPersonCheck(true);

  DelegateIfInControl( new TriggerAnimationAction(AnimationTrigger::WakeupGetout), [this]() {
      if( _iConfig.personCheckBehavior->WantsToBeActivated() ) {
        DelegateIfInControl( _iConfig.personCheckBehavior.get(), &BehaviorSleepCycle::RespondToPersonCheck );
      }
      else {
        PRINT_NAMED_WARNING("BehaviorSleepCycle.PersonCheck.BehaviorWontActivate",
                            "Behavior '%s' doesn't want to activate",
                            _iConfig.personCheckBehavior->GetDebugLabel().c_str());
        RespondToPersonCheck();
      }
    });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::RespondToPersonCheck()
{
  // undo mute / track locking for sleep
  MuteForPersonCheck(false);

  // if there's a face, time to wake up
  const bool onlyRecognizable = true;
  if( GetBEI().GetFaceWorld().HasAnyFaces(_dVars.personCheckStartTimestamp, onlyRecognizable) ) {
    WakeUp(WakeReason::SawPerson, false);
    return;
  }

  auto& sleepTracker = GetBEI().GetSleepTracker();

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
void BehaviorSleepCycle::SleepTransitionHelper(const SleepStateID& newState, const bool playSleepGetIn)
{
  if( _dVars.currState == SleepStateID::Awake ) {
    // not going to be awake anymore
    CancelDelegates(false);
  }

  SetState(newState);

  SleepIfInControl(playSleepGetIn);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToComatose()
{
  SleepTransitionHelper(SleepStateID::Comatose);

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.comatoseStartTime_s = currTime_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToEmergencySleep()
{
  SleepTransitionHelper(SleepStateID::EmergencySleep);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToDeepSleep()
{
  SleepTransitionHelper(SleepStateID::DeepSleep);

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.nextPersonCheckTime_s = currTime_s + kSleepCycle_DeepSleep_PersonCheckInterval_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToLightSleep()
{
  SleepTransitionHelper(SleepStateID::LightSleep);

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.nextPersonCheckTime_s = currTime_s + kSleepCycle_LightSleep_PersonCheckInterval_s;
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

    // even if requested, skip the get-in if this is a recent reboot (go straight to sleep in this case)
    const bool skipGetIn = WasNightlyReboot();

    if( playGetIn && !skipGetIn ) {
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

  GetBEI().GetMoodManager().TriggerEmotionEvent("RespondToGoodNight");

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
                          "Not on charger contacts, but find charger behavior '%s' doesn't want to activate",
                          _iConfig.findChargerBehavior->GetDebugLabel().c_str());
      TransitionToLightOrDeepSleep();
    }
  }
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::TransitionToLightOrDeepSleep()
{
  auto& sleepTracker = GetBEI().GetSleepTracker();
  sleepTracker.EnsureSleepDebtAtLeast( kSleepCycle_MinSleepDebt_s );

  // decide if we should go to light or deep sleep
  const bool isNightTime = sleepTracker.IsNightTime();
  if( isNightTime && !kSleepCycleForceLightSleep ) {
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

  const bool wasEmergencyMode = _dVars.currState == SleepStateID::EmergencySleep;

  _dVars.currState = state;
  PRINT_CH_INFO("Behaviors", "BehaviorSleepCycle.SetState",
                "%s",
                SleepStateIDToString(state));

  const bool isEmergencyMode = _dVars.currState == SleepStateID::EmergencySleep;

  if( wasEmergencyMode && !isEmergencyMode ) {
    // leaving emergency mode, enable wake word streaming again
    SmartPopResponseToTriggerWord();
  }
  else if( !wasEmergencyMode && isEmergencyMode ) {
    // within emergency mode, disable trigger word audio streaming
    SmartAlterStreamStateForCurrentResponse(StreamAndLightEffect::StreamingDisabledButWithLight);
  }


  SetConditionsActiveForState(_dVars.currState, true);

  // if the state changes, our sleeping reaction will not be active (or be canceled) so clear here
  _dVars.reactionState = SleepReactionType::NotReacting;


  auto* context = GetBEI().GetRobotInfo().GetContext();
  if( context ) {
    if( auto webSender = WebService::WebVizSender::CreateWebVizSender(kWebVizModuleNameSleeping,
                                                                      context->GetWebService()) ) {
      PopulateWebVizJson(webSender->Data());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::SetReactionState(SleepReactionType reaction)
{
  _dVars.reactionState = reaction;

  PRINT_CH_INFO("Behaviors", "BehaviorSleepCycle.Reaction",
                "sleeping reaction: %s",
                SleepReactionTypeToString(reaction));


  auto* context = GetBEI().GetRobotInfo().GetContext();
  if( context ) {
    if( auto webSender = WebService::WebVizSender::CreateWebVizSender(kWebVizModuleNameSleeping,
                                                                      context->GetWebService()) ) {
      PopulateWebVizJson(webSender->Data());
    }
  }
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
    case SleepStateID::EmergencySleep:
    case SleepStateID::DeepSleep:
    case SleepStateID::CheckingForPerson:
      return false;

    case SleepStateID::LightSleep:
      return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorSleepCycle::MuteForPersonCheck( bool mute )
{
  if( _dVars.isMuted != mute ) {

    if( mute ) {
      // lock the lift track because the robot likes to slam it around in a noisy way in some animations
      SmartLockTracks(static_cast<u8>(AnimTrackFlag::LIFT_TRACK), GetDebugLabel(), GetDebugLabel());
    }
    else {
      SmartUnLockTracks(GetDebugLabel());
    }

    SetAudioActive(!mute);

    _dVars.isMuted = mute;

    PRINT_CH_INFO("Behaviors", "BehaviorSleepCycle", "Setting mute: %d", mute);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NOTE:(bn) this code is copied from BehaviorQuietModeCoordinator. Decided against sharing it somewhere
// common for now because this is not a pattern I want to encourage
void BehaviorSleepCycle::SetAudioActive( bool active )
{
  using GE = AudioMetaData::GameEvent::GenericEvent;
  using GO = AudioMetaData::GameObjectType;
  const auto event = active ? GE::Play__Robot_Vic_Scene__Quiet_Off : GE::Play__Robot_Vic_Scene__Quiet_On;
  GetBEI().GetRobotAudioClient().PostEvent( event, GO::Behavior );
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

    data["reaction_state"] = SleepReactionTypeToString(_dVars.reactionState);
  }
#endif
}

}
}
