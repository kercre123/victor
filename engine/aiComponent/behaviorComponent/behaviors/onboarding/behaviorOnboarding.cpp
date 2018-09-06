/**
 * File: behaviorOnboarding.cpp
 *
 * Author: ross
 * Created: 2018-06-01
 *
 * Description: The linear progressor for onboarding. It manages interruptions common to all stages,
 *              messaging, calls to IOnboardingStage's for progresson logic, and delegation requested
 *              by the stages.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboarding.h"

#include "audioEngine/multiplexer/audioCladMessageHelper.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingDetectHabitat.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingInterruptionHead.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/stages/iOnboardingStage.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/stages/onboardingStageApp.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/stages/onboardingStageCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/stages/onboardingStageMeetVictor.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/stages/onboardingStageWakeUpComeHere.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/components/cubes/cubeConnectionCoordinator.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/cladProtoTypeTranslator.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "proto/external_interface/shared.pb.h"
#include "util/console/consoleFunction.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/DAS.h"

namespace Anki {
namespace Vector {

const std::string BehaviorOnboarding::kOnboardingFolder   = "onboarding";
const std::string BehaviorOnboarding::kOnboardingFilename = "onboardingState.json";
const std::string BehaviorOnboarding::kOnboardingStageKey = "onboardingStage";

namespace {
  const char* const kInterruptionsKey = "interruptions";
  const char* const kWakeUpKey = "wakeUpBehavior";
  const char* const kKeepFaceAliveLockName = "onboardingBehavior";
  
  const float kRequiredChargeTime_s = 5*60.0f;
  const float kExtraChargingTimePerDischargePeriod_s = 1.0f; // if off the charger for 1 min, must charge an additional 1*X mins
  
  // Behaviors that dim the screen on the app
  const std::set<BehaviorID> kBehaviorsThatPauseFlow = {
    BEHAVIOR_ID(OnboardingPowerOff),
    BEHAVIOR_ID(OnboardingPhysicalReactions),
    BEHAVIOR_ID(OnboardingPickedUp),
  };
  
  // these should match OnboardingStages (clad)
  CONSOLE_VAR_ENUM(int, kDevMoveToStage, "Onboarding", 0, "NotStarted,FinishedComeHere,FinishedMeetVictor,Complete,DevDoNothing");
  CONSOLE_VAR(bool, kDevAllowAnyContinue, "Onboarding", false);
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboarding::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboarding::DynamicVariables::DynamicVariables()
{
  wakeUpState = WakeUpState::Asleep;
  state = BehaviorState::StageNotStarted;
  
  currentStage = OnboardingStages::NotStarted;
  timeOfLastStageChange = 0;
  
  lastBehavior = nullptr;
  lastWhitelistedIntent = USER_INTENT(INVALID);
  lastInterruption = BEHAVIOR_ID(Anonymous);
  lastExpectedStep = external_interface::STEP_EXPECTING_CONTINUE_WAKE_UP;
  
  wakeWordState = WakeWordState::NotSet;
  
  currentStageBehaviorFinished = false;
  
  devConsoleStagePending = false;
  robotWokeUp = false;
  robotHeardTrigger = false;
  
  shouldDriveOffCharger = false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboarding::PendingEvent::PendingEvent(const EngineToGameEvent& event)
{
  type = PendingEvent::EngineToGame;
  new(&engineToGameEvent) EngineToGameEvent{event};
  time_s = event.GetCurrentTime();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboarding::PendingEvent::PendingEvent(const GameToEngineEvent& event)
{
  type = PendingEvent::GameToEngine;
  new(&gameToEngineEvent) GameToEngineEvent{event};
  time_s = event.GetCurrentTime();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboarding::PendingEvent::PendingEvent(const AppToEngineEvent& event)
{
  type = PendingEvent::AppToEngine;
  new(&appToEngineEvent) AppToEngineEvent{event};
  time_s = event.GetCurrentTime();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboarding::PendingEvent::~PendingEvent()
{
  if( type == PendingEvent::GameToEngine ) {
    gameToEngineEvent.~AnkiEvent();
  } else if( type == PendingEvent::EngineToGame ) {
    engineToGameEvent.~AnkiEvent();
  } else if( type == PendingEvent::AppToEngine ) {
    appToEngineEvent.~AnkiEvent();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboarding::BehaviorOnboarding(const Json::Value& config)
 : ICozmoBehavior(config)
{
  for( const auto& interruptionStr : config[kInterruptionsKey] ) {
    BehaviorID behaviorID;
    if( ANKI_VERIFY( BehaviorIDFromString( interruptionStr.asString(), behaviorID ),
                     "BehaviorOnboarding.Ctor.InvalidInterruption",
                     "BehaviorID %s invalid",
                     interruptionStr.asString().c_str() ) )
    {
      _iConfig.interruptionIDs.push_back( behaviorID );
    }
  }
  
  if( ANKI_VERIFY( config[kWakeUpKey].isString(),
                   "BehaviorOnboarding.WakeUpBehavior.Invalid", "Missing or invalid behavior") );
  {
    const auto& wakeUpStr = config[kWakeUpKey].asString();
    ANKI_VERIFY( BehaviorIDFromString( wakeUpStr, _iConfig.wakeUpID ),
                 "BehaviorOnboarding.WakeUpBehavior.InvalidID", "Invalid BehaviorID" );
  }
  
  SubscribeToTags({
    GameToEngineTag::SetOnboardingStage,
  });
  
  SubscribeToAppTags({
    AppToEngineTag::kOnboardingContinue,
    AppToEngineTag::kOnboardingSkip,
    AppToEngineTag::kOnboardingSkipOnboarding,
    AppToEngineTag::kOnboardingGetStep,
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboarding::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  // todo: some special casing is needed here in case the user starts onboarding with the robot in the air
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
  
  modifiers.cubeConnectionRequirements = BehaviorOperationModifiers::CubeConnectionRequirements::OptionalActive;
  modifiers.connectToCubeInBackground = true;
  
  // always look for faces, even during the initial stages, so come here has a higher chance of success
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Med });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.wakeUpBehavior.get() );
  // gather from interruptions
  for( const auto& interruption : _iConfig.interruptions ) {
    delegates.insert( interruption.get() );
  }
  // gather from stages
  for( const auto& stage : _iConfig.stages ) {
    std::set<BehaviorID> stageDelegates;
    stage.second->GetAllDelegates( stageDelegates );
    for( const auto& behaviorID : stageDelegates ) {
      ICozmoBehaviorPtr behavior = GetBEI().GetBehaviorContainer().FindBehaviorByID( behaviorID );
      if( ANKI_VERIFY( behavior != nullptr,
                       "BehaviorOnboarding.GetAllDelegates.Null",
                       "Behavior %s is invalid",
                       BehaviorTypesWrapper::BehaviorIDToString(behaviorID)) )
      {
        delegates.insert( behavior.get() );
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kInterruptionsKey,
    kWakeUpKey,
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::InitBehavior()
{
  // get interruption behaviors from interruption behaviorIDs
  _iConfig.interruptions.reserve( _iConfig.interruptionIDs.size() );
  for( const auto& behaviorID : _iConfig.interruptionIDs ) {
    ICozmoBehaviorPtr behavior = GetBEI().GetBehaviorContainer().FindBehaviorByID( behaviorID );
    if( ANKI_VERIFY( behavior != nullptr,
                     "BehaviorOnboarding.InitBehavior.Null", "Behavior '%s' not found in BC",
                     BehaviorTypesWrapper::BehaviorIDToString(behaviorID) ) )
    {
      _iConfig.interruptions.push_back( behavior );
    }
  }
  
  const auto& BC = GetBEI().GetBehaviorContainer();
  
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(OnboardingPickedUp),
                                  BEHAVIOR_CLASS(OnboardingInterruptionHead),
                                  _iConfig.pickedUpBehavior );
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(OnboardingPlacedOnCharger),
                                  BEHAVIOR_CLASS(OnboardingInterruptionHead),
                                  _iConfig.onChargerBehavior );
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(OnboardingDetectHabitat),
                                  BEHAVIOR_CLASS(OnboardingDetectHabitat),
                                  _iConfig.detectHabitatBehavior );
  
  _iConfig.reactToRobotOnBackBehavior = BC.FindBehaviorByID( BEHAVIOR_ID(ReactToRobotOnBack) );
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(OnboardingPopAWheelie), BEHAVIOR_CLASS(PopAWheelie), _iConfig.onboardingPopAWheelieBehavior );
  
  {
    _iConfig.wakeUpBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID( _iConfig.wakeUpID );
    ANKI_VERIFY( _iConfig.wakeUpBehavior != nullptr,
                 "BehaviorOnboarding.InitBehavior.NullWakeUp", "Behavior '%s' not found in BC",
                 BehaviorTypesWrapper::BehaviorIDToString(_iConfig.wakeUpID) );
  }
  
  if( ANKI_DEV_CHEATS ) {
    // init dev tools
    auto setStageFunc = [this](ConsoleFunctionContextRef context) {
      const OnboardingStages newStage = static_cast<OnboardingStages>(kDevMoveToStage);
      _dVars.devConsoleStagePending = true;
      _dVars.devConsoleStage = newStage;
      // save here, which may save twice if onboarding is running, but ensures it saves at least once outside of onboarding
      SaveToDisk( _dVars.devConsoleStage );
    };
    _iConfig.consoleFuncs.emplace_front( "OldMoveToStage", std::move(setStageFunc), "Onboarding", "" );
    
    auto continueFunc = [this](ConsoleFunctionContextRef context) {
      // cheat and request the continue that's expected
      RequestContinue( _dVars.lastExpectedStep );
    };
    _iConfig.consoleFuncs.emplace_front( "OldContinue", std::move(continueFunc), "Onboarding", "" );
    
    auto skipFunc = [this](ConsoleFunctionContextRef context) {
      RequestSkip();
    };
    _iConfig.consoleFuncs.emplace_front( "OldSkip", std::move(skipFunc), "Onboarding", "" );
    
    auto skipEverythingFunc = [this](ConsoleFunctionContextRef context) {
      RequestSkipRobotOnboarding();
    };
    _iConfig.consoleFuncs.emplace_front( "OldSkipEverything", std::move(skipEverythingFunc), "Onboarding", "" );
    
    auto retryChargingFunc = [this](ConsoleFunctionContextRef context) {
      RequestRetryCharging();
    };
    _iConfig.consoleFuncs.emplace_front( "OldRetryCharging", std::move(retryChargingFunc), "Onboarding", "" );
  }
  
  {
    using namespace Util;
    const auto* platform = GetBEI().GetRobotInfo().GetContext()->GetDataPlatform();
    _iConfig.saveFolder = platform->pathToResource( Data::Scope::Persistent, kOnboardingFolder );
    _iConfig.saveFolder = FileUtils::AddTrailingFileSeparator( _iConfig.saveFolder );
    if( FileUtils::DirectoryDoesNotExist( _iConfig.saveFolder ) ) {
      FileUtils::CreateDirectory( _iConfig.saveFolder, false, true );
    }
  }
}
  
void BehaviorOnboarding::OnBehaviorEnteredActivatableScope()
{
  // don't do this in InitBehavior so the stages don't exist in regular robot operation
  InitStages();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::OnBehaviorActivated() 
{
  // reset dynamic variables, saving the stage passed either through message or function call
  const auto stage = _dVars.currentStage;
  _dVars = DynamicVariables();
  _dVars.currentStage = stage;
  // robot has already woken up and heard trigger if starting in any stage other than the first.
  _dVars.robotWokeUp = (_dVars.currentStage != OnboardingStages::NotStarted);
  _dVars.robotHeardTrigger = (_dVars.currentStage != OnboardingStages::NotStarted);
  _dVars.timeOfLastStageChange = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  
  
  SetWakeWordState( WakeWordState::TriggerDisabled );
  
  
  PRINT_CH_INFO("Behaviors",
                "BehaviorOnboarding.OnBehaviorActivated.OnboardingStatus",
                "Starting onboarding in %s",
                OnboardingStagesToString(stage));
  
  if( _dVars.currentStage == OnboardingStages::NotStarted ) {
    // after ResetOnboarding message is received and has taken effect, send the app the new stage
    SendStageToApp( _dVars.currentStage );
    
    // wake up is handled by stage, which gets delegated to in BehaviorUpdate.
    _dVars.wakeUpState = WakeUpState::Complete;
    // no need to activate anything, Update() does that
  } else {
    PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.OnBehaviorActivated.OnboardingStatus", "Waking up");
    _dVars.wakeUpState = WakeUpState::WakingUp;
    ANKI_VERIFY( _iConfig.wakeUpBehavior->WantsToBeActivated(),
                 "BehaviorOnboarding.OnBehaviorActivated.WakeUp", "Wake up doesnt want to run");
    DelegateIfInControl( _iConfig.wakeUpBehavior.get(), [this](){
      _dVars.wakeUpState = WakeUpState::Complete;
    });
  }
}

void BehaviorOnboarding::OnBehaviorDeactivated()
{
  PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.OnBehaviorDeactivated.OnboardingStatus", "Onboarding complete (deactivated)");
  SmartEnableEngineResponseToTriggerWord();
  SetAllowAnyIntent();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  if( _dVars.wakeUpState != WakeUpState::Complete ) {
    // keep waiting for wake up to finish
    return;
  }
  
  if( _dVars.devConsoleStagePending ) {
    if( _dVars.devConsoleStage == OnboardingStages::Complete ) {
      SaveToDisk( _dVars.devConsoleStage );
      _dVars.currentStage = OnboardingStages::Complete;
      TerminateOnboarding();
    } else {
      if( static_cast<u8>(_dVars.devConsoleStage) < static_cast<u8>(_dVars.currentStage) ) {
        InitStages(false);
      }
      MoveToStage( _dVars.devConsoleStage );
    }
    _dVars.devConsoleStagePending = false;
  }

  if( _dVars.state == BehaviorState::WaitingForTermination ) {
    // todo: warn if this doesn't get canceled
    return;
  }
  
  UpdateBatteryInfo();
  
  int requestedStep = external_interface::STEP_INVALID;
  
  IBehavior* stageBehavior = nullptr;
  
  const auto lastInterruption = _dVars.lastInterruption;
  if( !CheckAndDelegateInterruptions() ) {
    // no interruptions want to run or are running
    
    // ideally we'd guard access to the delegation component here, but it now has to be done within the
    // for loop bc of an edge case where OnboardingLookAtPhone needs to switch actions when receiving a Continue
    
    // this for loop is like a BOUNDED_DO_WHILE and i is unused.
    for( size_t i=0; i<OnboardingStagesNumEntries; ++i ) {
      if( _dVars.state == BehaviorState::InterruptedButComplete ) {
        // an interruption just ended, but instead of resuming, this stage should end
        if( _dVars.currentStage == OnboardingStages::Complete ) {
          TerminateOnboarding();
          return;
        }
        MoveToStage( static_cast<OnboardingStages>(static_cast<u8>(_dVars.currentStage) + 1) );
      }
      
      // maintain the order of ICozmoBehavior: activation, events, completion callbacks, update
      
      if( _dVars.state == BehaviorState::StageNotStarted ) {
        auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
        GetCurrentStage()->OnBegin( GetBEI() );
      } else if( _dVars.state == BehaviorState::Interrupted ) {
        auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
        GetCurrentStage()->OnResume( GetBEI(), lastInterruption );
      }
      
      // send events
      for( const auto& pendingEvent : _dVars.pendingEvents ) {
        if( pendingEvent.second.type == PendingEvent::Continue ) {
          const bool allowedContinue = GetCurrentStage()->OnContinue( GetBEI(), pendingEvent.second.continueNum );
          SendContinueResponse( allowedContinue, pendingEvent.second.continueNum );
        } else if( pendingEvent.second.type == PendingEvent::Skip ) {
          auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
          GetCurrentStage()->OnSkip( GetBEI() );
        } else if( pendingEvent.second.type == PendingEvent::GameToEngine ) {
          auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
          GetCurrentStage()->OnMessage( GetBEI(), pendingEvent.second.gameToEngineEvent );
        } else if( pendingEvent.second.type == PendingEvent::EngineToGame ) {
          auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
          GetCurrentStage()->OnMessage( GetBEI(), pendingEvent.second.engineToGameEvent );
        }
      }
      _dVars.pendingEvents.clear();
      
      // completion callbacks
      if( _dVars.currentStageBehaviorFinished ) {
        _dVars.currentStageBehaviorFinished = false;
        // change the lastBehavior so that if the stage requests the same behavior another time, we delegate to it
        _dVars.lastBehavior = nullptr;
        auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
        GetCurrentStage()->OnBehaviorDeactivated( GetBEI() );
      }
      
      // update
      GetCurrentStage()->Update( GetBEI() );
      
      // check stage for options about the trigger word and intents
      UserIntentTag allowedIntentTag = USER_INTENT(INVALID);
      const bool triggerAllowed = GetCurrentStage()->GetWakeWordBehavior( allowedIntentTag );
      if( triggerAllowed && (allowedIntentTag == USER_INTENT(unmatched_intent)) ) {
        // this is sloppy, but the only time unmatched_intent is the whitelisted intent is when the special response is used
        SetWakeWordState( WakeWordState::SpecialTriggerEnabledCloudDisabled );
      } else if( triggerAllowed ) {
        SetWakeWordState( WakeWordState::TriggerEnabled );
      } else {
        SetWakeWordState( WakeWordState::TriggerDisabled );
      }
      SetAllowedIntent( allowedIntentTag );
      
      _dVars.state = BehaviorState::StageRunning;
      
      // now get what behavior the stage requests for behavior and step num
      stageBehavior = GetCurrentStage()->GetBehavior( GetBEI() );
      if( stageBehavior != nullptr ) {
        auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
        requestedStep = GetCurrentStage()->GetExpectedStep();
        if( (stageBehavior != _dVars.lastBehavior) && !stageBehavior->WantsToBeActivated() ) {
          PRINT_NAMED_WARNING( "BehaviorOnboarding.BehaviorUpdate.DoesntWantToActivate",
                               "OnboardingStatus: Transition from %s to %s, but it doesn't want to activate!",
                               _dVars.lastBehavior == nullptr ? "null" : _dVars.lastBehavior->GetDebugLabel().c_str(),
                               stageBehavior->GetDebugLabel().c_str() );
          // move to next stage
          stageBehavior = nullptr;
        } else {
          break;
        }
      } else {
        // the stage thinks it's done
        PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.BehaviorUodate.OnboardingStatus",
                      "Stage %s signaled it is complete", OnboardingStagesToString(_dVars.currentStage));
      }
      
      // either the stage is done, or stageBehavior isn't valid, so move to next stage
      
      // just in case two stages request the same behavior, cancel the running one when moving to the next stage
      // todo: maybe this shouldn't happen if one stage ends with the same behavior the next begins with, and
      // restarting would not be smooth? the interstitials are lookaround behaviors so far
      CancelDelegates(false);
      _dVars.lastBehavior = nullptr;
      
      if( _dVars.currentStage == OnboardingStages::Complete ) {
        TerminateOnboarding();
        return;
      }
      MoveToStage( static_cast<OnboardingStages>(static_cast<u8>(_dVars.currentStage) + 1) );
    }
  }
  
  if( (stageBehavior != nullptr) && (_dVars.lastBehavior != stageBehavior) ) {
    if( _dVars.lastBehavior != nullptr ) {
      // there was an interruption or other behavior running. Cancel it and run a callback. This
      // is done instead of CancelDelegates(true) since that callback is used to know whether a stage
      // behavior finished normally
      OnDelegateComplete();
    }
    CancelDelegates(false);
    _dVars.lastBehavior = stageBehavior;
    SetRobotExpectingStep( requestedStep );
    _dVars.currentStageBehaviorFinished = false;
    DelegateIfInControl( stageBehavior, [this]() {
      _dVars.currentStageBehaviorFinished = true;
      OnDelegateComplete();
    });
  } else if( stageBehavior == nullptr ) {
    _dVars.lastBehavior = nullptr;
  }
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::AlwaysHandleInScope(const GameToEngineEvent& event)
{
  if( event.GetData().GetTag() == ExternalInterface::MessageGameToEngineTag::SetOnboardingStage ) {
    const auto& msg = event.GetData().Get_SetOnboardingStage();
    MoveToStage( msg.stage );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::AlwaysHandleInScope(const AppToEngineEvent& event)
{
  if( event.GetData().GetTag() == external_interface::GatewayWrapperTag::kOnboardingContinue ) {
    const auto stepNum = event.GetData().onboarding_continue().step_number();
    if( kDevAllowAnyContinue ) {
      RequestContinue( _dVars.lastExpectedStep );
    } else {
      RequestContinue( stepNum );
    }
  } else if( event.GetData().GetTag() == external_interface::GatewayWrapperTag::kOnboardingSkip ) {
    RequestSkip();
  } else if( event.GetData().GetTag() == external_interface::GatewayWrapperTag::kOnboardingSkipOnboarding ) {
    RequestSkipRobotOnboarding();
  } else if( event.GetData().GetTag() == external_interface::GatewayWrapperTag::kOnboardingGetStep ) {
    auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
    if( gi != nullptr ) {
      external_interface::OnboardingSteps stepEnum{ static_cast<external_interface::OnboardingSteps>(_dVars.lastExpectedStep) };
      auto* onboardingStepResponse = new external_interface::OnboardingStepResponse{ stepEnum };
      gi->Broadcast( ExternalMessageRouter::WrapResponse(onboardingStepResponse) );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::InitStages(bool resetExisting)
{
  // init stages
  if( resetExisting || !_iConfig.stages[OnboardingStages::NotStarted] ) {
    _iConfig.stages[OnboardingStages::NotStarted].reset( new OnboardingStageWakeUpComeHere{} );
    _iConfig.stages[OnboardingStages::NotStarted]->Init();
  }
  if( resetExisting || !_iConfig.stages[OnboardingStages::FinishedComeHere] ) {
    _iConfig.stages[OnboardingStages::FinishedComeHere].reset( new OnboardingStageMeetVictor{} );
    _iConfig.stages[OnboardingStages::FinishedComeHere]->Init();
  }
  if( resetExisting || !_iConfig.stages[OnboardingStages::FinishedMeetVictor] ) {
    _iConfig.stages[OnboardingStages::FinishedMeetVictor].reset( new OnboardingStageCube{} );
    _iConfig.stages[OnboardingStages::FinishedMeetVictor]->Init();
  }
  if( resetExisting || !_iConfig.stages[OnboardingStages::Complete] ) {
    // this stage only runs if app onboarding directly follows robot onboarding, since BehaviorOnboarding
    // only activates if the onboarding stage is < Complete. This stage just keeps the robot quiet
    // while you peruse the app, and is not necessary if you restart app onboarding after having used
    // the robot for a while
    _iConfig.stages[OnboardingStages::Complete].reset( new OnboardingStageApp{} );
    _iConfig.stages[OnboardingStages::Complete]->Init();
  }
}
  
void BehaviorOnboarding::SaveToDisk( const OnboardingStages& stage ) const
{
  Json::Value toSave;
  toSave[kOnboardingStageKey] = OnboardingStagesToString(stage);
  const std::string filename = _iConfig.saveFolder + kOnboardingFilename;
  Util::FileUtils::WriteFile( filename, toSave.toStyledString() );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::MoveToStage( const OnboardingStages& stage )
{
  SaveToDisk( stage );
  
  // broadcast
  if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    // if it's Complete, pass a flag to not reset the behavior stack into normal operation.
    // there's one stage left to run that accompanies app onboarding, which simply waits some amount
    // of time for a voice command without doing anything too crazy (like an unprompted fistbump).
    // Since the complete state is saved to disk, any reboots starting now will boot in normal mode, and any
    // other app onboarding that begins right after a reboot will have normal robot behavior (so
    // for now, an unprompted fistbump could occur).
    const bool forceSkipStackReset = (stage == OnboardingStages::Complete);
    
    ExternalInterface::OnboardingState msg{ stage, forceSkipStackReset };
    ei->Broadcast( ExternalInterface::MessageEngineToGame{std::move(msg)} );
  }
  
  SendStageToApp( stage );
  
  // save to whiteboard for convenience
  {
    auto& whiteboard = GetAIComp<AIWhiteboard>();
    whiteboard.SetMostRecentOnboardingStage( stage );
  }
  
  // log
  PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.MoveToStage.OnboardingStatus", "Onboarding moving to stage %s", OnboardingStagesToString(stage));
  
  // das msg
  const EngineTimeStamp_t currTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  DASMSG(onboarding_next_stage, "onboarding.next_stage", "Changed stage in onboarding");
  DASMSG_SET(i1, (int)stage, "Stage number");
  DASMSG_SET(i2, (TimeStamp_t)(currTime - _dVars.timeOfLastStageChange), "Time spent in stage (ms)");
  DASMSG_SEND();
  
  // actually change to the next stage
  _dVars.currentStage = stage;
  _dVars.timeOfLastStageChange = currTime;
  if( _dVars.currentStage == OnboardingStages::DevDoNothing ) {
    // don't set up events for that dev stage since it doesn't have an associated IOnboardingStage
    _dVars.state = BehaviorState::WaitingForTermination;
    return;
  }
  
  _dVars.state = BehaviorState::StageNotStarted;
  _dVars.currentStageBehaviorFinished = false;
  _dVars.pendingEvents.clear();
  // setup event handlers for new stage
  _dVars.stageEventHandles.clear();
  auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
  if( ei != nullptr ) {
    auto onEvent = [this](const EngineToGameEvent& event) {
      _dVars.pendingEvents.emplace( std::piecewise_construct,
                                    std::forward_as_tuple(event.GetCurrentTime()),
                                    std::forward_as_tuple(event) );
    };
    std::set<EngineToGameTag> tags;
    GetCurrentStage()->GetAdditionalMessages( tags );
    for( const auto& tag : tags ) {
      _dVars.stageEventHandles.push_back( ei->Subscribe(tag, onEvent) );
    }
  }
  if( ei != nullptr ) { // having two ifs is only for scope purposes
    auto onEvent = [this](const GameToEngineEvent& event) {
      _dVars.pendingEvents.emplace( std::piecewise_construct,
                                    std::forward_as_tuple(event.GetCurrentTime()),
                                    std::forward_as_tuple(event) );
    };
    std::set<GameToEngineTag> tags;
    GetCurrentStage()->GetAdditionalMessages( tags );
    for( const auto& tag : tags ) {
      _dVars.stageEventHandles.push_back( ei->Subscribe(tag, onEvent) );
    }
  }
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto onEvent = [this](const AppToEngineEvent& event) {
      _dVars.pendingEvents.emplace( std::piecewise_construct,
                                    std::forward_as_tuple(event.GetCurrentTime()),
                                    std::forward_as_tuple(event) );
    };
    std::set<AppToEngineTag> tags;
    GetCurrentStage()->GetAdditionalMessages( tags );
    for( const auto& tag : tags ) {
      _dVars.stageEventHandles.push_back( gi->Subscribe(tag, onEvent) );
    }
  }
  // start with trigger word disabled and no whitelist
  SetWakeWordState( WakeWordState::TriggerDisabled );
  SetAllowAnyIntent();
  
  // drop all IOnboardingStage objects prior to this one so their destructors run
  for( size_t i=0; i<static_cast<u8>(stage); ++i ) {
    _iConfig.stages[static_cast<OnboardingStages>(i)].reset();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboarding::CheckAndDelegateInterruptions()
{
  DEV_ASSERT( _iConfig.interruptions.size() == _iConfig.interruptionIDs.size(),
              "BehaviorOnboarding.CheckAndDelegateInterruptions.SizeMismatch" );
  
  if( (_dVars.currentStage == OnboardingStages::FinishedMeetVictor)
      && (_dVars.lastExpectedStep == external_interface::STEP_CUBE_TRICK)
      && _iConfig.onboardingPopAWheelieBehavior->IsActivated()
      && _iConfig.onboardingPopAWheelieBehavior->IsPoppingWheelie() )
  {
    // hack: suppress the normal react to robot on back behavior so that it doesn't abort the full
    // popawheelie animation. todo VIC-5812: split that animation up into a get onto back animation, and a
    // react to robot on back animation, so that we can play them both without suppressing behaviors
    _iConfig.reactToRobotOnBackBehavior->SetDontActivateThisTick( GetDebugLabel() );
    
    // Also, since the robot OffTreadsState will often flicker into InAir during PopAWheelie,
    // prevent the pickup behavior from running while the special react to on back behavior is running
    _iConfig.pickedUpBehavior->SetDontActivateThisTick( GetDebugLabel() );
  }
  
  for( size_t i=0; i<_iConfig.interruptions.size(); ++i ) {
    
    const auto& interruption = _iConfig.interruptions[i];
    if( interruption->IsActivated() ) {
      return true;
    }
    
    const auto& interruptionID = _iConfig.interruptionIDs[i];
    if( !CanInterruptionActivate( interruptionID ) ) {
      continue;
    }
    
    if( interruption->WantsToBeActivated() ) {
      Interrupt( interruption, interruptionID );
      return true;
    }
  }
  
  // no interruption is runnign or wants to activate
  NotifyOfInterruptionChange( BEHAVIOR_ID(Anonymous) );
  _dVars.lastInterruption = BEHAVIOR_ID(Anonymous);
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::Interrupt( ICozmoBehaviorPtr interruption, BehaviorID interruptionID )
{
  if( (_dVars.lastBehavior != nullptr) || (_dVars.lastInterruption != BEHAVIOR_ID(Anonymous)) ) {
    // there was an interruption or other behavior running. Cancel it and run a callback. This
    // is done instead of CancelDelegates(true) since that callback is used to know whether a stage
    // behavior finished normally
    OnDelegateComplete();
  }
  CancelDelegates(false);
  
  // if a stage was running, tell it it was interrupted
  if( _dVars.state == BehaviorState::StageRunning ) {
    auto& currStage = GetCurrentStage();
    auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
    const bool shouldRestart = currStage->OnInterrupted( GetBEI(), interruptionID );
    if( shouldRestart ) {
      _dVars.state = BehaviorState::InterruptedButComplete;
    } else {
      _dVars.state = BehaviorState::Interrupted;
    }
  }
  
  // tell picked up and on charger whether or not the robot is "groggy" (whether it received a trigger word)
  if( interruptionID == BEHAVIOR_ID(OnboardingPickedUp) ) {
    _iConfig.pickedUpBehavior->SetPlayGroggyAnimations( !_dVars.robotHeardTrigger );
  } else if( interruptionID == BEHAVIOR_ID(OnboardingPlacedOnCharger) ) {
    _iConfig.onChargerBehavior->SetPlayGroggyAnimations( !_dVars.robotHeardTrigger );
  } else if( (interruptionID == BEHAVIOR_ID(OnboardingFirstTriggerWord))
             || (interruptionID == BEHAVIOR_ID(OnboardingTriggerWord)) )
  {
    _dVars.robotHeardTrigger = true;
  }
  
  NotifyOfInterruptionChange(interruptionID);
  
  _dVars.lastInterruption = interruptionID;
  _dVars.lastBehavior = nullptr;
  _dVars.currentStageBehaviorFinished = false;
  DelegateIfInControl( interruption.get() );
  
  
  // disable trigger word during most interruptions, except trigger word obviously. Trigger word wouldn't
  // be activating if it was disabled by the behavior.
  if( interruptionID != BEHAVIOR_ID(OnboardingTriggerWord) ) {
    SetWakeWordState( WakeWordState::TriggerDisabled );
  }
}
  
void BehaviorOnboarding::NotifyOfInterruptionChange( BehaviorID interruptionID )
{
  // currently some interruption behaviors can end and immediately want to activate, so check whether
  // something has changed to prevent giving the app the same message multiple times
  const bool interruptionChanged = (_dVars.lastInterruption != interruptionID);
  
  if( interruptionChanged ) {
    // _dVars.lastInterruption is ending
    
    // if OnboardingPlacedOnCharger or OnboardingPickedUp ended, message the app
    if( _dVars.lastInterruption == BEHAVIOR_ID(OnboardingPlacedOnCharger) ) {
      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      // only send if not low battery, since UpdateLowBattery handles that case
      if( !_dVars.batteryInfo.lowBattery && (gi != nullptr) ) {
        auto* onboardingOnCharger = new external_interface::OnboardingOnCharger{ false, false, -1.0f };
        PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus", "No longer placed on charger");
        gi->Broadcast( ExternalMessageRouter::Wrap(onboardingOnCharger) );
      }
    } else if( kBehaviorsThatPauseFlow.find(_dVars.lastInterruption) != kBehaviorsThatPauseFlow.end() ) {
      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      if( gi != nullptr ) {
        auto* msg = new external_interface::OnboardingPhysicalInterruption{ external_interface::ONBOARDING_INTERRUPTION_NONE };
        PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus", "Put back down, resuming");
        gi->Broadcast( ExternalMessageRouter::Wrap(msg) );
      }
    }
  }
  
  
  // notify app of certain new interruptions:
  // OnboardingPowerOff          : send OnboardingPhysicalInterruption. todo: power off messages
  // OnboardingPhysicalReactions : OnboardingPhysicalInterruption
  // OnboardingLowBattery        : start charging countdown logic
  // OnboardingDetectHabitat     : send OnboardingHabitatDetected
  // OnboardingPlacedOnCharger   : only send if not low battery, since OnboardingLowBattery handles that
  // DriveOffChargerStraight     : no message
  // OnboardingPickedUp          : send OnboardingPhysicalInterruption
  // OnboardingFirstTriggerWord  : will send normal messages (WakeWordBegin/End)
  // OnboardingTriggerWord       : will send normal messages (WakeWordBegin/End)
  if( interruptionID == BEHAVIOR_ID(DriveOffChargerStraight) ) {
    // generic resuming state
    SetRobotExpectingStep( external_interface::STEP_RESUMING );
  } else if( interruptionID == BEHAVIOR_ID(OnboardingPlacedOnCharger) ) {
    // low battery handled from UpdateBatteryInfo
    if( !_dVars.batteryInfo.lowBattery && interruptionChanged ) {
      SetRobotExpectingStep( external_interface::STEP_EXPECTING_RESUME_FROM_CHARGER );
      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      if( gi != nullptr ) {
        PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus", "Placed on charger, waiting for continue");
        auto* onboardingOnCharger = new external_interface::OnboardingOnCharger{ true, false, -1.0f };
        gi->Broadcast( ExternalMessageRouter::Wrap( onboardingOnCharger) );
      }
    }
  } else if( kBehaviorsThatPauseFlow.find(interruptionID) != kBehaviorsThatPauseFlow.end() ) {
    SetRobotExpectingStep( external_interface::STEP_PHYSICAL_INTERRUPTION );
    if( interruptionChanged ) {
      // screen should go dim
      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      if( gi != nullptr ) {
        PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus",
                      "Physical reaction %s. Waiting to be be on a flat surface",
                      BehaviorIDToString(interruptionID));
        const int msgType = GetPhysicalInterruptionMsgType( interruptionID );
        auto* msg = new external_interface::OnboardingPhysicalInterruption;
        msg->set_type( static_cast<external_interface::OnboardingPhysicalInterruptions>( msgType ) );
        gi->Broadcast( ExternalMessageRouter::Wrap(msg) );
      }
    }
  } else if( interruptionID == BEHAVIOR_ID(OnboardingLowBattery) ) {
    SetRobotExpectingStep( external_interface::STEP_LOW_BATTERY );
    if( interruptionChanged ) {
      StartLowBatteryCountdown();
    }
  } else if( interruptionID == BEHAVIOR_ID(OnboardingDetectHabitat) ) {
    SetRobotExpectingStep( external_interface::STEP_EXPECTING_RESUME_FROM_HABITAT );
    auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
    if( interruptionChanged && (gi != nullptr) ) {
      PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus",
                    "Detected the habitat. Pausing until Continue, or until some other interruption");
      auto* msg = new external_interface::OnboardingHabitatDetected;
      gi->Broadcast( ExternalMessageRouter::Wrap(msg) );
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboarding::StagePtr& BehaviorOnboarding::GetCurrentStage()
{
  auto& ptr = _iConfig.stages[ _dVars.currentStage ];
  DEV_ASSERT( ptr != nullptr, "Pointer should never be null" );
  return ptr;
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::TerminateOnboarding()
{
  // we shouldn't CancelSelf() because this is the base behavior in the stack. But we can
  // tell the BehaviorsBootLoader to reboot with a different stack
  if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    ExternalInterface::OnboardingState state;
    state.stage = _dVars.currentStage;
    state.forceSkipStackReset = false; // reset the stack into normal operation
    DEV_ASSERT( _dVars.currentStage == OnboardingStages::Complete, "Expected to be called when complete");
    ei->Broadcast( ExternalInterface::MessageEngineToGame{std::move(state)} );
  }
  _dVars.state = BehaviorState::WaitingForTermination;
  // destroy stages so they don't linger for the rest of the robot's life
  _iConfig.stages.clear();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::RequestContinue( int step )
{
  PRINT_NAMED_INFO("BehaviorOnboarding.RequestContinue.OnboardingStatus", "App requesting %d, expecting %d", step, _dVars.lastExpectedStep);
  if( (_dVars.state == BehaviorState::Interrupted)
      || (_dVars.state == BehaviorState::InterruptedButComplete)
      || (_dVars.state == BehaviorState::StageNotStarted) )
  {
    if( _dVars.lastInterruption == BEHAVIOR_ID(OnboardingLowBattery) ) {
      ANKI_VERIFY( _dVars.lastExpectedStep == external_interface::STEP_EXPECTING_RESUME_FROM_CHARGER,
                   "BehaviorOnboarding.Interrupt.ContinueMismatchLowBattery",
                   "State was %d not STEP_EXPECTING_RESUME_FROM_CHARGER",
                   _dVars.lastExpectedStep );
      const bool accepted = (step == external_interface::STEP_EXPECTING_RESUME_FROM_CHARGER);
      if( accepted ) {
        // the app should only be sending this if the battery is out of low battery mode
        if( GetBEI().GetRobotInfo().GetBatteryLevel() == BatteryLevel::Low ) {
          PRINT_NAMED_WARNING("BehaviorOnboarding.RequestContinue.LowBattery", "App requested off charger, but not done");
        }
        _dVars.batteryInfo = BatteryInfo(); // reset. This will re-trigger everything if it is still low battery
        DEV_ASSERT( IsControlDelegated(), "Should be delegating to OnboardingLowBattery" );
        // start driving off the charger
        PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus",
                      "Robot is charged and received continue. Resuming");
        CancelDelegates(false);
        _dVars.shouldDriveOffCharger = true;
      }
      SendContinueResponse( accepted, step );
    } else if( _dVars.lastInterruption == BEHAVIOR_ID(OnboardingPlacedOnCharger) ) {
      ANKI_VERIFY( _dVars.lastExpectedStep == external_interface::STEP_EXPECTING_RESUME_FROM_CHARGER,
                  "BehaviorOnboarding.Interrupt.ContinueMismatchLowBattery",
                  "State was %d not STEP_EXPECTING_RESUME_FROM_CHARGER",
                  _dVars.lastExpectedStep );
      const bool accepted = (step == external_interface::STEP_EXPECTING_RESUME_FROM_CHARGER);
      if( accepted ) {
        PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus", "Resuming from charger");
        // if the user placed the robot on the charger, continue acts as the command to get it off
        DEV_ASSERT( IsControlDelegated(), "Should be delegating to PlacedOnCharger" );
        // start driving off the charger
        CancelDelegates(false);
        _dVars.shouldDriveOffCharger = true;
      }
      SendContinueResponse( accepted, step );
    } else if( _dVars.lastInterruption == BEHAVIOR_ID(OnboardingDetectHabitat) ) {
      const bool accepted = (step == external_interface::STEP_EXPECTING_RESUME_FROM_HABITAT);
      if( accepted ) {
        PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus", "Resuming from habitat");
        // resume
        CancelDelegates(false);
      }
      SendContinueResponse( accepted, step );
    }
  } else {
    // pass continue along to stage during Update()
    const float time_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    auto itInserted = _dVars.pendingEvents.emplace( std::piecewise_construct,
                                                   std::forward_as_tuple(time_s),
                                                   std::forward_as_tuple() );
    itInserted->second.type = PendingEvent::Continue;
    itInserted->second.continueNum = step;
    itInserted->second.time_s = time_s;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::RequestSkip()
{
  if( (_dVars.state == BehaviorState::StageRunning) || (_dVars.state == BehaviorState::Interrupted) ) {
    if( _dVars.state != BehaviorState::StageRunning ) {
      // The user sent a skip and immediately afterward an interruption ran. We keep it pending until
      // the interruption is over since the app moved on. If this is a problem we might need a skip response.
      PRINT_NAMED_WARNING("BehaviorOnboarding.RequestSkip.SkipDuringInterruption",
                          "Requesting a skip during '%s'", BehaviorIDToString(_dVars.lastInterruption));
    }
    
    double time_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    // use default PendingEvent constructor
    auto itInserted = _dVars.pendingEvents.emplace( std::piecewise_construct,
                                                    std::forward_as_tuple(time_s),
                                                    std::forward_as_tuple() );
    itInserted->second.type = PendingEvent::Skip;
    itInserted->second.time_s = time_s;
    
    DASMSG(onboarding_skip_stage, "onboarding.skip_stage", "Skipped the stage in onboarding");
    DASMSG_SET(i1, (int)_dVars.currentStage, "Current stage number");
    DASMSG_SEND();
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::RequestSkipRobotOnboarding()
{
  MoveToStage( OnboardingStages::Complete );
  _dVars.state = BehaviorState::WaitingForTermination;
  GetBEI().GetAnimationComponent().AddKeepFaceAliveDisableLock(kKeepFaceAliveLockName);
  CancelDelegates(false);
  
  auto* headDownAction = new MoveHeadToAngleAction{ MIN_HEAD_ANGLE };
  DelegateIfInControl( headDownAction, [this](const ActionResult& res){
    TerminateOnboarding();
    GetBEI().GetAnimationComponent().RemoveKeepFaceAliveDisableLock(kKeepFaceAliveLockName);
  });
  
  DASMSG(onboarding_skip_onboarding, "onboarding.skip_onboarding", "Skipped all of onboarding");
  DASMSG_SET(i1, (int)_dVars.currentStage, "Current stage number");
  DASMSG_SEND();
  
};
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::RequestRetryCharging()
{
  _dVars.batteryInfo = BatteryInfo();
  _dVars.batteryInfo.lowBattery = false;
  StartLowBatteryCountdown();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::SetAllowedIntent(UserIntentTag tag)
{
  if( tag == USER_INTENT(INVALID) ) {
    SetAllowAnyIntent();
  } else if( tag != _dVars.lastWhitelistedIntent ) {
    auto& uic = GetBehaviorComp<UserIntentComponent>();
    uic.SetWhitelistedIntents( {tag} );
    _dVars.lastWhitelistedIntent = tag;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::SetAllowAnyIntent()
{
  if( _dVars.lastWhitelistedIntent != USER_INTENT(INVALID) ) {
    auto& uic = GetBehaviorComp<UserIntentComponent>();
    uic.ClearWhitelistedIntents();
    _dVars.lastWhitelistedIntent = USER_INTENT(INVALID);
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::UpdateBatteryInfo()
{
  auto& info = _dVars.batteryInfo;
  
  if( !info.lowBattery
     && !_dVars.robotWokeUp
     && (_dVars.currentStage == OnboardingStages::NotStarted)
     && (GetBEI().GetRobotInfo().GetBatteryLevel() == BatteryLevel::Low) )
  {
    auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
    if( gi != nullptr ) {
      auto* onboardingLowBattery = new external_interface::OnboardingLowBattery{ true };
      gi->Broadcast( ExternalMessageRouter::Wrap(onboardingLowBattery) );
    }
    info.lowBattery = true;
    SetRobotExpectingStep( external_interface::STEP_LOW_BATTERY );
  }
  
  
  if( !info.lowBattery ) {
    return;
  }
 
  // todo: migrate this to use BatteryComponent::GetSuggestedChargerTime()
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // robot should be charging for the first 30 mins or so of being on contacts, so use IsOnChargerContacts instead of IsCharging
  const bool currOnCharger = GetBEI().GetRobotInfo().IsOnChargerContacts();
  const bool currOnChargerPlatform = GetBEI().GetRobotInfo().IsOnChargerPlatform();
  if( currOnCharger && (info.chargerState != BatteryInfo::ChargerState::OnCharger) ) {
    info.chargerState = BatteryInfo::ChargerState::OnCharger;
    float requiredChargeTime_s;
    if( (info.timeChargingDone_s < 0.0f) || (info.timeRemovedFromCharger < 0.0f) ) {
      // was never on charger before
      requiredChargeTime_s = kRequiredChargeTime_s;
      info.timeChargingDone_s = requiredChargeTime_s + currTime_s;
      PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus",
                    "Low battery on charger. Countdown started at %f", requiredChargeTime_s);
    } else {
      // not the first time on the charger, so there should be a timeRemovedFromCharger.
      const float elapsedOffChargerTime_s = currTime_s - info.timeRemovedFromCharger;
      info.timeChargingDone_s += elapsedOffChargerTime_s * (1.0f + kExtraChargingTimePerDischargePeriod_s);
      info.timeChargingDone_s = Anki::Util::Min(info.timeChargingDone_s, currTime_s + kRequiredChargeTime_s);
      requiredChargeTime_s = info.timeChargingDone_s - currTime_s;
      PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus",
                    "Low battery on charger. Countdown resuming at %f", requiredChargeTime_s);
    }
    auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
    if( gi != nullptr ) {
      auto* onboardingOnCharger = new external_interface::OnboardingOnCharger{ true, true, requiredChargeTime_s };
      gi->Broadcast( ExternalMessageRouter::Wrap(onboardingOnCharger) );
    }
    info.sentOutOfLowBattery = false;
  } else if( !currOnCharger && !currOnChargerPlatform && (info.chargerState == BatteryInfo::ChargerState::OnCharger) ) {
    // this also checked charger platform in case it slips off contact while wiggling onto charger
    info.chargerState = BatteryInfo::ChargerState::OffCharger;
    PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus",
                  "Low battery off charger. Countdown paused");
    info.timeRemovedFromCharger = currTime_s;
    auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
    if( gi != nullptr ) {
      auto* onboardingOnCharger = new external_interface::OnboardingOnCharger{ false, true, -1.0f };
      gi->Broadcast( ExternalMessageRouter::Wrap(onboardingOnCharger) );
    }
  }
  
  if( currOnCharger && !info.sentOutOfLowBattery && (GetBEI().GetRobotInfo().GetBatteryLevel() != BatteryLevel::Low) ) {
    // tell the app we're out of low battery state. This will mean that if the countdown hasn't expired
    // yet, then when it does, it will show the Resume screen. If the app countdown expires before this is
    // sent, it will display something about checking cables, and should send a RetryCharging.
    auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
    if( gi != nullptr ) {
      auto* onboardingLowBattery = new external_interface::OnboardingLowBattery{ false };
      gi->Broadcast( ExternalMessageRouter::Wrap(onboardingLowBattery) );
    }
    SetRobotExpectingStep( external_interface::STEP_EXPECTING_RESUME_FROM_CHARGER );
    info.sentOutOfLowBattery = true;
    PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus",
                  "Robot is charged now. Waiting on continue");
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::StartLowBatteryCountdown()
{
  auto& info = _dVars.batteryInfo;
  // only start it if not already started, since low battery can get interrupted by mandatory reactions
  if( !info.lowBattery ) {
    info.lowBattery = true;
    SetRobotExpectingStep( external_interface::STEP_LOW_BATTERY );
    
    PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus",
                  "Low battery. Countdown will start when on charger");
    
    auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
    if( gi != nullptr ) {
      auto* onboardingLowBattery = new external_interface::OnboardingLowBattery{ true };
      gi->Broadcast( ExternalMessageRouter::Wrap(onboardingLowBattery) );
    }
  }
}
            
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::SendContinueResponse( bool acceptedContinue, int step )
{
  if( !acceptedContinue ) {
    PRINT_NAMED_WARNING( "BehaviorOnboarding.SendContinueResponse.InvalidContinue",
                         "OnboardingStatus: Stage %s, behavior '%s' (step '%d') did not accept input step %d",
                         OnboardingStagesToString(_dVars.currentStage),
                         (_dVars.lastBehavior != nullptr) ? _dVars.lastBehavior->GetDebugLabel().c_str() : "[null]",
                         _dVars.lastExpectedStep, step );
  }
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* onboardingContinueResponse = new external_interface::OnboardingContinueResponse;
    
    external_interface::OnboardingSteps expectedStep = static_cast<external_interface::OnboardingSteps>(_dVars.lastExpectedStep);
    external_interface::OnboardingSteps requestedStep = static_cast<external_interface::OnboardingSteps>(step);
    
    onboardingContinueResponse->set_accepted( acceptedContinue );
    onboardingContinueResponse->set_robot_step_number( expectedStep );
    onboardingContinueResponse->set_request_step_number( requestedStep );
    gi->Broadcast( ExternalMessageRouter::WrapResponse(onboardingContinueResponse) );
    
    // we probably don't need this now that the response sends the last expected step, but I won't change until after 1.0
    if( !acceptedContinue ) {
      // resend expected state
      auto* onboardingExpectedStep = new external_interface::OnboardingRobotExpectingStep{ expectedStep };
      gi->Broadcast( ExternalMessageRouter::Wrap(onboardingExpectedStep) );
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::SetRobotExpectingStep( int step )
{
  ANKI_VERIFY( step != external_interface::STEP_INVALID,
               "BehaviorOnboarding.SetRobotExpectingStep.Invalid",
               "Internally requested invalid step" );
  
  if( step >= external_interface::STEP_EXPECTING_FIRST_TRIGGER_WORD ) {
    _dVars.robotWokeUp = true;
  }
  if( _iConfig.detectHabitatBehavior != nullptr ) {
    _iConfig.detectHabitatBehavior->SetOnboardingStep( step );
  }
  
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( (gi != nullptr) && (_dVars.lastExpectedStep != step) ) {
    PRINT_NAMED_INFO("BehaviorOnboarding.SetRobotExpectingStep.OnboardingStatus", "Robot expecting step %d", step);
    external_interface::OnboardingSteps stepEnum{ static_cast<external_interface::OnboardingSteps>(step) };
    auto* onboardingExpectedStep = new external_interface::OnboardingRobotExpectingStep{ stepEnum };
    gi->Broadcast( ExternalMessageRouter::Wrap(onboardingExpectedStep) );
  }
  _dVars.lastExpectedStep = step;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboarding::CanInterruptionActivate( BehaviorID interruptionID ) const
{
  // special logic for first stage
  if( _dVars.currentStage == OnboardingStages::NotStarted ) {
    if( interruptionID == BEHAVIOR_ID(DriveOffChargerStraight) ) {
      // first stage has a special drive off charger
      return false;
    } else if( !_dVars.robotWokeUp ) {
      // dont run interruptions if it hasn't finished waking up
      // todo: exception to this exception: driving off the charger that is placed right next to a cliff.
      // Currently the drive of charger animation stops, and then the robot will wait for a message
      // to leave the charger again with groggy eyes. Ideally there'd be some groggy react to
      // cliff animation prior to that
      return false;
    }
  } else {
    // OnboardingFirstTriggerWord json ensures it only runs once, but we never want it to run at all if
    // the robot boots into a stage later than wake up
    if( interruptionID == BEHAVIOR_ID(OnboardingFirstTriggerWord) ) {
      return false;
    }
  }
  
  if( (interruptionID == BEHAVIOR_ID(DriveOffChargerStraight)) && !_dVars.shouldDriveOffCharger ) {
    // drive off charger should only activate when a flag is set, which usually corresponds to receipt of a "continue"
    return false;
  }
  
  if( (_dVars.currentStage == OnboardingStages::NotStarted)
     && (interruptionID == BEHAVIOR_ID(OnboardingPlacedOnCharger))
     && _dVars.shouldDriveOffCharger )
  {
    // placed on charger should not activate while the robot is driving off the charger (first stage only)
    return false;
  }
  
  // all others should check WantsToBeActivated()
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::OnDelegateComplete()
{
  // not matter what behavior ended, clear the flag that the robot should drive off the charger
  _dVars.shouldDriveOffCharger = false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int BehaviorOnboarding::GetPhysicalInterruptionMsgType( BehaviorID interruptionID ) const
{
  int interruptionType = 0;
  if( interruptionID == BEHAVIOR_ID(OnboardingPickedUp) ) {
    interruptionType = external_interface::ONBOARDING_INTERRUPTION_PICKED_UP;
  } else if( interruptionID == BEHAVIOR_ID(OnboardingPhysicalReactions) ) {
    interruptionType = external_interface::ONBOARDING_INTERRUPTION_PHYSICAL;
  } else if( interruptionID == BEHAVIOR_ID(OnboardingPowerOff) ) {
    interruptionType = external_interface::ONBOARDING_INTERRUPTION_POWER_BUTTON;
  } else {
    ANKI_VERIFY(false,
                "BehaviorOnboarding.GetPhysicalInterruptionMessage.Invalid",
                "kBehaviorsThatPauseFlow does not include %s",
                BehaviorIDToString(interruptionID));
  }
  return interruptionType;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::SendStageToApp( const OnboardingStages& stage ) const
{
  if( GetBEI().GetRobotInfo().HasGatewayInterface() ) {
    auto* onboardingState = new external_interface::OnboardingState{ CladProtoTypeTranslator::ToProtoEnum(stage) };
    auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
    gi->Broadcast( ExternalMessageRouter::Wrap(onboardingState) );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::ConnectionLostCallback()
{
  // cube connection dropped. try again
  const bool connectInBackground = true;
  GetBEI().GetCubeConnectionCoordinator().SubscribeToCubeConnection( this, connectInBackground );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::ConnectionFailedCallback()
{
  // cube connection failed. try again.
  const bool connectInBackground = true;
  GetBEI().GetCubeConnectionCoordinator().SubscribeToCubeConnection( this, connectInBackground );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::SetWakeWordState( WakeWordState wakeWordState )
{
  
  namespace AECH = AudioEngine::Multiplexer::CladMessageHelper;
  
  if( wakeWordState != _dVars.wakeWordState ) {
    if( _dVars.wakeWordState != WakeWordState::NotSet ) {
      SmartPopResponseToTriggerWord();
    }
    _dVars.wakeWordState = wakeWordState;
    
    if( wakeWordState == WakeWordState::TriggerDisabled ) {
      
      // no trigger word allowed
      SmartDisableEngineResponseToTriggerWord();
      SmartPushResponseToTriggerWord();
      
    } else if( wakeWordState == WakeWordState::SpecialTriggerEnabledCloudDisabled ) {
      
      // no streaming, but fake streaming
      SmartEnableEngineResponseToTriggerWord();
      const auto postAudioEvent
        = AECH::CreatePostAudioEvent( AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Wake_Word_On,
                                      AudioMetaData::GameObjectType::Behavior, 0 );
      SmartPushResponseToTriggerWord(AnimationTrigger::OnboardingWakeWordGetIn,
                                     postAudioEvent,
                                     StreamAndLightEffect::StreamingDisabledButWithLight );
      
    } else if( wakeWordState == WakeWordState::TriggerEnabled ) {
      
      SmartEnableEngineResponseToTriggerWord();
      const auto postAudioEvent
        = AECH::CreatePostAudioEvent( AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Wake_Word_On,
                                      AudioMetaData::GameObjectType::Behavior, 0 );
      SmartPushResponseToTriggerWord(AnimationTrigger::VC_ListeningGetIn, postAudioEvent, StreamAndLightEffect::StreamingEnabled );
      
    }
  }
  
}

}
}
