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
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/iOnboardingStage.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/onboardingStageApp.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/onboardingStageCube.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/onboardingStageMeetVictor.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/onboardingStageWakeUpWelcomeHome.h"
#include "engine/components/mics/micComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "util/console/consoleFunction.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"

namespace Anki {
namespace Cozmo {

const std::string BehaviorOnboarding::kOnboardingFolder   = "onboarding";
const std::string BehaviorOnboarding::kOnboardingFilename = "onboardingState.json";
const std::string BehaviorOnboarding::kOnboardingStageKey = "onboardingStage";

namespace {
  const char* const kInterruptionsKey = "interruptions";
  const char* const kWakeUpKey = "wakeUpBehavior";
  
  const float kRequiredChargeTime_s = 5*60.0f;
  const float kExtraChargingTimePerDischargePeriod_s = 1.0f; // if off the charger for 1 min, must charge an additional 1*X mins
  
  // these should match OnboardingStages (clad)
  CONSOLE_VAR_ENUM(int, kDevMoveToStage, "Onboarding", 0, "NotStarted,FinishedWelcomeHome,FinishedMeetVictor,Complete,DevDoNothing");
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
  
  lastBehavior = nullptr;
  lastTriggerWordEnabled = true; // needs to be true so that trigger detection will get disabled upon activation
  lastWhitelistedIntent = USER_INTENT(INVALID);
  lastInterruption = BEHAVIOR_ID(Anonymous);
  
  currentStageBehaviorFinished = false;
  
  devConsoleStagePending = false;
  receivedContinue = false;
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
BehaviorOnboarding::PendingEvent::~PendingEvent()
{
  if( type == PendingEvent::GameToEngine ) {
    gameToEngineEvent.~AnkiEvent();
  } else if( type == PendingEvent::EngineToGame ) {
    engineToGameEvent.~AnkiEvent();
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
    GameToEngineTag::OnboardingContinue,
    GameToEngineTag::OnboardingSkip,
    GameToEngineTag::OnboardingRetryCharging,
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
                       BehaviorIDToString(behaviorID)) )
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
                     BehaviorIDToString(behaviorID) ) )
    {
      _iConfig.interruptions.push_back( behavior );
    }
  }
  
  {
    _iConfig.wakeUpBehavior = GetBEI().GetBehaviorContainer().FindBehaviorByID( _iConfig.wakeUpID );
    ANKI_VERIFY( _iConfig.wakeUpBehavior != nullptr,
                 "BehaviorOnboarding.InitBehavior.NullWakeUp", "Behavior '%s' not found in BC",
                 BehaviorIDToString(_iConfig.wakeUpID) );
  }
  
  if( ANKI_DEV_CHEATS ) {
    // init dev tools
    auto setStageFunc = [this](ConsoleFunctionContextRef context) {
      const OnboardingStages newStage = static_cast<OnboardingStages>(kDevMoveToStage);
      _dVars.devConsoleStagePending = true;
      _dVars.devConsoleStage = newStage;
    };
    _iConfig.consoleFuncs.emplace_front( "MoveToStage", std::move(setStageFunc), "Onboarding", "" );
    
    auto continueFunc = [this](ConsoleFunctionContextRef context) {
      RequestContinue();
    };
    _iConfig.consoleFuncs.emplace_front( "Continue", std::move(continueFunc), "Onboarding", "" );
    
    auto skipFunc = [this](ConsoleFunctionContextRef context) {
      RequestSkip();
    };
    _iConfig.consoleFuncs.emplace_front( "Skip", std::move(skipFunc), "Onboarding", "" );
    
    auto retryChargingFunc = [this](ConsoleFunctionContextRef context) {
      RequestRetryCharging();
    };
    _iConfig.consoleFuncs.emplace_front( "RetryCharging", std::move(retryChargingFunc), "Onboarding", "" );
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
  
  // default to no trigger word allowed
  SetTriggerWordEnabled( false );
  PRINT_CH_INFO("Behaviors",
                "BehaviorOnboarding.OnBehaviorActivated.OnboardingStatus",
                "Starting onboarding in %s",
                OnboardingStagesToString(stage));
  
  if( _dVars.currentStage == OnboardingStages::NotStarted ) {
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
  PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.OnBehaviorDeactivated.OnboardingStatus", "Onboarding complete");
  SetTriggerWordEnabled( true );
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
  
  IBehavior* stageBehavior = nullptr;
  if( !CheckAndDelegateInterruptions() ) {
    // no interruptions want to run or are running
    
    // if OnboardingPlacedOnCharger or OnboardingPickedUp ended, message the app
    if( _dVars.lastInterruption == BEHAVIOR_ID(OnboardingPlacedOnCharger) ) {
      auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
      // only send if not low battery, since UpdateLowBattery handles that case
      if( !_dVars.batteryInfo.lowBattery && (ei != nullptr) ) {
        ExternalInterface::OnboardingOnCharger msg{false, false, -1.0f };
        PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus", "No longer placed on charger");
        ei->Broadcast( ExternalMessageRouter::Wrap(std::move(msg)) );
      }
    } else if( _dVars.lastInterruption == BEHAVIOR_ID(OnboardingPickedUp) ) {
      auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
      if( ei != nullptr ) {
        ExternalInterface::OnboardingPickedUp msg{false};
        PRINT_CH_INFO("Behaviors", "OnboardingPickedUp", "false");
        PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus", "Put back down, resuming");
        ei->Broadcast( ExternalMessageRouter::Wrap(std::move(msg)) );
      }
    }
    const auto lastInterruption = _dVars.lastInterruption;
    _dVars.lastInterruption = BEHAVIOR_ID(Anonymous);
    
    
    auto accessGuard = GetBEI().GetComponentWrapper(BEIComponentID::Delegation).StripComponent();
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
        GetCurrentStage()->OnBegin( GetBEI() );
      } else if( _dVars.state == BehaviorState::Interrupted ) {
        GetCurrentStage()->OnResume( GetBEI(), lastInterruption );
      }
      
      // send events
      for( const auto& pendingEvent : _dVars.pendingEvents ) {
        if( pendingEvent.second.type == PendingEvent::Continue ) {
          GetCurrentStage()->OnContinue( GetBEI() );
        } else if( pendingEvent.second.type == PendingEvent::Skip ) {
          GetCurrentStage()->OnSkip( GetBEI() );
        } else if( pendingEvent.second.type == PendingEvent::GameToEngine ) {
          GetCurrentStage()->OnMessage( GetBEI(), pendingEvent.second.gameToEngineEvent );
        } else if( pendingEvent.second.type == PendingEvent::EngineToGame ) {
          GetCurrentStage()->OnMessage( GetBEI(), pendingEvent.second.engineToGameEvent );
        }
      }
      _dVars.pendingEvents.clear();
      
      // completion callbacks
      if( _dVars.currentStageBehaviorFinished ) {
        _dVars.currentStageBehaviorFinished = false;
        GetCurrentStage()->OnBehaviorDeactivated( GetBEI() );
      }
      
      // update
      GetCurrentStage()->Update( GetBEI() );
      
      // check stage for options about the trigger word and intents
      UserIntentTag allowedIntentTag = USER_INTENT(INVALID);
      const bool triggerAllowed = GetCurrentStage()->GetWakeWordBehavior( allowedIntentTag );
      SetTriggerWordEnabled( triggerAllowed );
      SetAllowedIntent( allowedIntentTag );
      
      _dVars.state = BehaviorState::StageRunning;
      
      // now get what behavior the stage requests
      stageBehavior = GetCurrentStage()->GetBehavior( GetBEI() );
      if( stageBehavior != nullptr ) {
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
        PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.BehaviorUodate.OnboardingStatus",
                      "Stage %s signaled it is complete", OnboardingStagesToString(_dVars.currentStage));
      }
      
      // stageBehavior wasn't valid, so move to next stage
      
      // just in case two stages request the same behavior, cancel the running one when moving to the next stage
      // todo: maybe this shouldn't happen if one stage ends with the same behavior the next begins with, and
      // restarting would not be smooth?
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
    _dVars.lastBehavior = stageBehavior;
    _dVars.currentStageBehaviorFinished = false;
    CancelDelegates(false);
    DelegateIfInControl( stageBehavior, [this]() {
      _dVars.currentStageBehaviorFinished = true;
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
  } else if( event.GetData().GetTag() == ExternalInterface::MessageGameToEngineTag::OnboardingContinue ) {
    RequestContinue();
  } else if( event.GetData().GetTag() == ExternalInterface::MessageGameToEngineTag::OnboardingSkip ) {
    RequestSkip();
  } else if( event.GetData().GetTag() == ExternalInterface::MessageGameToEngineTag::OnboardingRetryCharging ) {
    RequestRetryCharging();
  }
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::InitStages(bool resetExisting)
{
  // init stages
  if( resetExisting || !_iConfig.stages[OnboardingStages::NotStarted] ) {
    _iConfig.stages[OnboardingStages::NotStarted].reset( new OnboardingStageWakeUpWelcomeHome{} );
  }
  if( resetExisting || !_iConfig.stages[OnboardingStages::FinishedWelcomeHome] ) {
    _iConfig.stages[OnboardingStages::FinishedWelcomeHome].reset( new OnboardingStageMeetVictor{} );
  }
  if( resetExisting || !_iConfig.stages[OnboardingStages::FinishedMeetVictor] ) {
    _iConfig.stages[OnboardingStages::FinishedMeetVictor].reset( new OnboardingStageCube{} );
  }
  if( resetExisting || !_iConfig.stages[OnboardingStages::Complete] ) {
    // this stage only runs if app onboarding directly follows robot onboarding, since BehaviorOnboarding
    // only activates if the onboarding stage is < Complete. This stage just keeps the robot quiet
    // while you peruse the app, and is not necessary if you restart app onboarding after having used
    // the robot for a while
    _iConfig.stages[OnboardingStages::Complete].reset( new OnboardingStageApp{} );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::MoveToStage( const OnboardingStages& stage )
{
  ExternalInterface::OnboardingState saveState;
  saveState.stage = stage;
  
  // save to disk
  {
    Json::Value toSave;
    toSave[kOnboardingStageKey] = OnboardingStagesToString(stage);
    const std::string filename = _iConfig.saveFolder + kOnboardingFilename;
    Util::FileUtils::WriteFile( filename, toSave.toStyledString() );
  }
  
  // broadcast
  if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    if( stage != OnboardingStages::Complete ) {
      // only if it's not Complete, broadcast to the rest of the robot. If it were to broadcast
      // Complete, the robot would pop out into normal operation, but there's one stage left to
      // run that accompanies app onboarding, which simply waits some amount of time for a voice
      // command without doing anything too crazy (like an unprompted fistbump). Since the complete
      // state is saved to disk, any reboots starting now will boot in normal mode, and any
      // other app onboarding that begins right after a reboot will have normal robot behavior (so
      // for now, an unprompted fistbump could occur).
      const auto& msgRef = saveState; // force copy ctor
      ExternalInterface::OnboardingState msgCopy{msgRef};
      ei->Broadcast( ExternalInterface::MessageEngineToGame{std::move(msgCopy)} );
    }
    PRINT_CH_INFO("Behaviors", "OnboardingState", "saved");
    ei->Broadcast( ExternalMessageRouter::Wrap(std::move(saveState)) );
  }
  
  // save to whiteboard for convenience
  {
    auto& whiteboard = GetAIComp<AIWhiteboard>();
    whiteboard.SetMostRecentOnboardingStage( stage );
  }
  
  // log
  PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.MoveToStage.OnboardingStatus", "Onboarding moving to stage %s", OnboardingStagesToString(stage));
  
  // actually change to the next stage
  _dVars.currentStage = stage;
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
      _dVars.pendingEvents.emplace( std::piecewise_construct, std::forward_as_tuple(event.GetCurrentTime()),  std::forward_as_tuple(event) );
    };
    std::set<EngineToGameTag> tags;
    GetCurrentStage()->GetAdditionalMessages( tags );
    for( const auto& tag : tags ) {
      _dVars.stageEventHandles.push_back( ei->Subscribe(tag, onEvent) );
    }
  }
  if( ei != nullptr ) { // having two ifs is only for scope purposes
    auto onEvent = [this](const GameToEngineEvent& event) {
      _dVars.pendingEvents.emplace( std::piecewise_construct, std::forward_as_tuple(event.GetCurrentTime()),  std::forward_as_tuple(event) );
    };
    std::set<GameToEngineTag> tags;
    GetCurrentStage()->GetAdditionalMessages( tags );
    for( const auto& tag : tags ) {
      _dVars.stageEventHandles.push_back( ei->Subscribe(tag, onEvent) );
    }
  }
  // start with trigger word disabled and no whitelist
  SetTriggerWordEnabled(false);
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
  for( size_t i=0; i<_iConfig.interruptions.size(); ++i ) {
    const auto& interruption = _iConfig.interruptions[i];
    const auto& interruptionID = _iConfig.interruptionIDs[i];
    
    // special logic for first stage
    if( _dVars.currentStage == OnboardingStages::NotStarted ) {
      if( interruptionID == BEHAVIOR_ID(DriveOffCharger) ) {
        // first stage has a special drive off charger
        continue;
      } else if( !_dVars.receivedContinue ) {
        // dont run interruptions if we havent received the first Continue, which would drive the robot off charger
        continue;
      }
    }
    
    if( interruption->IsActivated() ) {
      return true;
    }
    if( interruption->WantsToBeActivated() ) {
      Interrupt( interruption, interruptionID );
      return true;
    }
  }
  
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::Interrupt( ICozmoBehaviorPtr interruption, BehaviorID interruptionID )
{
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
  
  _dVars.lastInterruption = interruptionID;
  _dVars.lastBehavior = nullptr;
  DelegateIfInControl( interruption.get() );
  
  // notify app of certain interruptions
  // MandatoryPhysicalReactions: no app update needed for now
  // OnboardingLowBattery      : start charging countdown logic
  // TriggerWordDetected       : will send normal messages
  // OnboardingPickedUp        : sends messages here
  // OnboardingPlacedOnCharger : only send if not low battery, since OnboardingLowBattery handles that
  if( interruptionID == BEHAVIOR_ID(OnboardingPlacedOnCharger) ) {
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    if( !_dVars.batteryInfo.lowBattery && (ei != nullptr) ) {
      PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus", "Placed on charger, waiting for continue");
      ExternalInterface::OnboardingOnCharger msg{true, false, -1.0f};
      ei->Broadcast( ExternalMessageRouter::Wrap(std::move(msg)) );
    }
  } else if( interruptionID == BEHAVIOR_ID(OnboardingPickedUp) ) {
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    if( ei != nullptr ) {
      PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus", "Picked up. Waiting to be put down");
      ExternalInterface::OnboardingPickedUp msg{true};
      ei->Broadcast( ExternalMessageRouter::Wrap(std::move(msg)) );
    }
  } else if( interruptionID == BEHAVIOR_ID(OnboardingLowBattery) ) {
    StartLowBatteryCountdown();
  }
  
  // disable trigger word during most interruptions, except trigger word obviously. Trigger word wouldn't
  // be activating if it was disabled by the behavior.
  if( interruptionID != BEHAVIOR_ID(TriggerWordDetected) ) {
    SetTriggerWordEnabled( false );
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
    DEV_ASSERT( _dVars.currentStage == OnboardingStages::Complete, "Expected to be called when complete");
    ei->Broadcast( ExternalInterface::MessageEngineToGame{std::move(state)} );
  }
  _dVars.state = BehaviorState::WaitingForTermination;
  // destroy stages so they don't linger for the rest of the robot's life
  _iConfig.stages.clear();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::RequestContinue()
{
  _dVars.receivedContinue = true;
  if( (_dVars.state == BehaviorState::Interrupted) || (_dVars.state == BehaviorState::InterruptedButComplete) ) {
    if( _dVars.lastInterruption == BEHAVIOR_ID(OnboardingLowBattery) ) {
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
    } else if( _dVars.lastInterruption == BEHAVIOR_ID(OnboardingPlacedOnCharger) ) {
      PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus", "Resuming from charger");
      // if the user placed the robot on the charger, continue acts as the command to get it off
      DEV_ASSERT( IsControlDelegated(), "Should be delegating to PlacedOnCharger" );
      // start driving off the charger
      CancelDelegates(false);
    }
  } else {
    double time_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    // use default PendingEvent constructor
    auto itInserted = _dVars.pendingEvents.emplace( std::piecewise_construct, std::forward_as_tuple(time_s), std::forward_as_tuple() );
    itInserted->second.type = PendingEvent::Continue;
    itInserted->second.time_s = time_s;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::RequestSkip()
{
  // todo: there's probably some edge case where the app sends a skip right around the time an interruption
  // begins, so we should probably add a SkipResponse message or just provide more state info. Same for Continue
  if( _dVars.state == BehaviorState::StageRunning ) {
    double time_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    // use default PendingEvent constructor
    auto itInserted = _dVars.pendingEvents.emplace( std::piecewise_construct, std::forward_as_tuple(time_s), std::forward_as_tuple() );
    itInserted->second.type = PendingEvent::Skip;
    itInserted->second.time_s = time_s;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::RequestRetryCharging()
{
  _dVars.batteryInfo = BatteryInfo();
  _dVars.batteryInfo.lowBattery = false;
  StartLowBatteryCountdown();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding::SetTriggerWordEnabled(bool enabled)
{
  if( _dVars.lastTriggerWordEnabled != enabled ) {
    GetBEI().GetMicComponent().SetTriggerWordDetectionEnabled( enabled );
    _dVars.lastTriggerWordEnabled = enabled;
  }
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
     && !_dVars.receivedContinue
     && (_dVars.currentStage == OnboardingStages::NotStarted)
     && (GetBEI().GetRobotInfo().GetBatteryLevel() == BatteryLevel::Low) )
  {
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    if( ei != nullptr ) {
      ExternalInterface::OnboardingLowBattery msg{true};
      ei->Broadcast( ExternalMessageRouter::Wrap(std::move(msg)) );
    }
    info.lowBattery = true;
  }
  
  
  if( !info.lowBattery ) {
    return;
  }
 
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  // robot should be charging for the first 30 mins or so of being on contacts, so use IsOnChargerContacts instead of IsCharging
  const bool currOnCharger = GetBEI().GetRobotInfo().IsOnChargerContacts();
  const bool currOnChargerPlatform = GetBEI().GetRobotInfo().IsOnChargerPlatform();
  if( currOnCharger && !info.onCharger ) {
    info.onCharger = true;
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
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    if( ei != nullptr ) {
      ExternalInterface::OnboardingOnCharger msg{true, true, requiredChargeTime_s};
      ei->Broadcast( ExternalMessageRouter::Wrap(std::move(msg)) );
    }
    info.sentOutOfLowBattery = false;
  } else if( !currOnCharger && !currOnChargerPlatform && info.onCharger ) {
    // this also checked charger platform in case it slips off contact while wiggling onto charger
    info.onCharger = false;
    PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus",
                  "Low battery off charger. Countdown paused");
    info.timeRemovedFromCharger = currTime_s;
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    if( ei != nullptr ) {
      ExternalInterface::OnboardingOnCharger msg{false, true, -1.0f}; // no need to calc the time since the app just pauses it
      ei->Broadcast( ExternalMessageRouter::Wrap(std::move(msg)) );
    }
  }
  
  if( currOnCharger && !info.sentOutOfLowBattery && (GetBEI().GetRobotInfo().GetBatteryLevel() != BatteryLevel::Low) ) {
    // tell the app we're out of low battery state. This will mean that if the countdown hasn't expired
    // yet, then when it does, it will show the Resume screen. If the app countdown expires before this is
    // sent, it will display something about checking cables, and should send a RetryCharging.
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    if( ei != nullptr ) {
      ExternalInterface::OnboardingLowBattery msg{false};
      ei->Broadcast( ExternalMessageRouter::Wrap(std::move(msg)) );
    }
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
    
    PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.Interrupt.OnboardingStatus",
                  "Low battery. Countdown will start when on charger");
    
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    if( ei != nullptr ) {
      ExternalInterface::OnboardingLowBattery msg{true};
      ei->Broadcast( ExternalMessageRouter::Wrap(std::move(msg)) );
    }
  }
}
  

}
}
