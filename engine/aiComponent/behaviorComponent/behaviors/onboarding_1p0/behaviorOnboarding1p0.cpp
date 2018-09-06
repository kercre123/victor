/**
 * File: BehaviorOnboarding1p0.cpp
 *
 * Author: ross
 * Created: 2018-09-04
 *
 * Description: Hail Mary Onboarding (1.0)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "engine/aiComponent/behaviorComponent/behaviors/onboarding_1p0/behaviorOnboarding1p0.h"

#include "audioEngine/multiplexer/audioCladMessageHelper.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/animActions.h"
#include "engine/actions/basicActions.h"
#include "engine/aiComponent/aiWhiteboard.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingLookAtPhone.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/components/batteryComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/robot.h"
#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"

namespace Anki {
namespace Vector {
  
const std::string BehaviorOnboarding1p0::kOnboardingFolder   = "onboarding";
const std::string BehaviorOnboarding1p0::kOnboardingFilename = "onboardingState.json";
const std::string BehaviorOnboarding1p0::kOnboardingStageKey = "onboardingStage";
  
namespace {
  const float kVCTimeout_s = 60.0f;
  const float kTreadsTimeout_s = 120.0f;
  
  const static BehaviorID kBehaviorIDWhileWaiting = BEHAVIOR_ID(OnboardingLookAtUser);
  const static BehaviorID kBehaviorIDWhileWaitingOnCharger = BEHAVIOR_ID(OnboardingLookAtUserOnCharger);
}
  
#define SET_STATE(s) do{ \
  _dVars.state = State::s; \
  PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.OnboardingStatus.State", "State = %s", #s); \
} while(0);

  
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboarding1p0::InstanceConfig::InstanceConfig()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboarding1p0::DynamicVariables::DynamicVariables()
{
  wakeWordState = WakeWordState::NotSet;
  state = State::NotStarted;
  receivedWakeUp = false;
  attemptedLeaveCharger = false;
  markedComplete = false;
  triggerWordStartTime_s = -1.0f;
  treadsStateEndTime_s = -1.0f;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboarding1p0::BehaviorOnboarding1p0(const Json::Value& config)
 : ICozmoBehavior(config)
{
  
  SubscribeToAppTags({
    AppToEngineTag::kOnboardingWakeUpRequest,
  });
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboarding1p0::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Med });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert( _iConfig.behaviorLookAtPhone.get() );
  delegates.insert( _iConfig.behaviorWaitForTriggerWord.get() );
  delegates.insert( _iConfig.behaviorWaitForTriggerWordOnCharger.get() );
  delegates.insert( _iConfig.behaviorTriggerWord.get() );
  delegates.insert( _iConfig.behaviorPowerOff.get() );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(OnboardingLookAtPhone), BEHAVIOR_CLASS(OnboardingLookAtPhone), _iConfig.behaviorLookAtPhone );
  _iConfig.behaviorWaitForTriggerWord = BC.FindBehaviorByID( kBehaviorIDWhileWaiting );
  _iConfig.behaviorWaitForTriggerWordOnCharger = BC.FindBehaviorByID( kBehaviorIDWhileWaitingOnCharger );
  _iConfig.behaviorTriggerWord = BC.FindBehaviorByID( BEHAVIOR_ID(OnboardingTriggerWord) );
  _iConfig.behaviorPowerOff = BC.FindBehaviorByID( BEHAVIOR_ID(OnboardingPowerOff) );
  
  if( ANKI_DEV_CHEATS ) {
    // this console func names match what the 1.1 onboarding tool will send, to avoid having to change that
    auto continueFunc = [this](ConsoleFunctionContextRef context) {
      // flag to wake up (don't start it now so that app messages and console vars are both applied at the same time)
      _dVars.receivedWakeUp = true;
    };
    _iConfig.consoleFuncs.emplace_front( "Continue", std::move(continueFunc), "Onboarding", "" );
    
    auto setCompleteFunc = [this](ConsoleFunctionContextRef context) {
      SetStage( OnboardingStages::Complete, false );
    };
    _iConfig.consoleFuncs.emplace_front( "SetComplete", std::move(setCompleteFunc), "Onboarding", "" );
    
    auto setDoNothingFunc = [this](ConsoleFunctionContextRef context) {
      SetStage( OnboardingStages::DevDoNothing, false );
    };
    _iConfig.consoleFuncs.emplace_front( "SetDevDoNothing", std::move(setDoNothingFunc), "Onboarding", "" );
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::OnBehaviorActivated() 
{
  // reset dynamic variables
  _dVars = DynamicVariables();
  
  // init battery info, but don't msg the app
  const bool sendEvent = false;
  UpdateBatteryInfo( sendEvent );
  
  // disable wake word for now
  DisableWakeWord();
  
  TransitionToPhoneIcon();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::BehaviorUpdate() 
{
  if( !IsActivated() ) {
    return;
  }
  
  UpdateBatteryInfo();
  
  if( !_iConfig.behaviorTriggerWord->IsActivated() && _iConfig.behaviorTriggerWord->WantsToBeActivated() ) {
    // check for trigger word in any stage
    TransitionToTriggerWord();
  }
  
  if( ShouldCheckPowerOff() ) {
    if( !_iConfig.behaviorPowerOff->IsActivated() && _iConfig.behaviorPowerOff->WantsToBeActivated() ) {
      TransitionToPoweringOff();
    }
  }
  
  if( _dVars.receivedWakeUp ) {
    _dVars.receivedWakeUp = false;
    // this only works from state LookAtPhone, but we _try_ in any state so that a msg can be sent on failure
    TryTransitioningToReceivedWakeUp();
  }
  
  
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  switch( _dVars.state ) {
      
    case State::LookAtPhone: // just listen for receivedWakeUp
      break;
      
    case State::ReceivedWakeUp: // just wait for head to go down and phone behavior to end
    {
      // just in case the callback for the phone icon behavior never ran(?), transition to the next state
      if( !IsControlDelegated() ) {
        TransitionToWakingUp();
      }
    }
      break;
      
    case State::WakingUp: // just wait for animation to complete
    case State::DriveOffCharger: // just wait for animation to complete
      break;
      
    
    case State::WaitForPutDown:
    {
      if( GetBEI().GetRobotInfo().GetOffTreadsState() == OffTreadsState::OnTreads ) {
        // will drive off charger if it hasnt already, or will wait for VC otherwise
        TransitionToDrivingOffCharger();
      } else if( currTime_s >= _dVars.treadsStateEndTime_s ) {
        // got stuck off treads somehow
        {
          DASMSG(onboarding_offtreads, "onboarding.offtreads", "Robot was stuck off treads");
          DASMSG_SEND();
        }
        TransitionToDrivingOffCharger();
      }
    }
      break;
      
    case State::WaitForTriggerWord:
    case State::WaitForTriggerWordOnCharger:
    {
      const bool onChargerState = (_dVars.state == State::WaitForTriggerWordOnCharger);
      const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerPlatform();
      if( currTime_s >= _dVars.triggerWordStartTime_s + kVCTimeout_s ) {
        const bool timeout = true;
        TerminateOnboarding( timeout );
      } else if( GetBEI().GetRobotInfo().GetOffTreadsState() != OffTreadsState::OnTreads ) {
        TransitionToWaitingForPutDown();
      } else if( onCharger != onChargerState ) {
        TransitionToWaitingForVC();
      }
    }
      break;
      
    case State::TriggerWord: // just wait for behavior to end
    case State::PowerOff: // just wait for behavior to end
    case State::WaitingForTermination: // just wait for BehaviorsBootLoader to switch stacks
      break;
      
    case State::NotStarted:
    {
      ANKI_VERIFY( false && "unexpected state", "BehaviorOnboarding.BehaviorUpdate.InvalidState", "" );
    }
      break;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::TransitionToPhoneIcon()
{
  CancelDelegates( false );
  
  SET_STATE( LookAtPhone );
  // this behavior runs until its ContinueReceived is called
  DelegateIfInControl( _iConfig.behaviorLookAtPhone.get(), [this](){
    TransitionToWakingUp();
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::TryTransitioningToReceivedWakeUp()
{
  const bool canWakeUp = (_dVars.state == State::LookAtPhone) && IsBatteryCountdownDone();
  
  // send msg before transitioning
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( gi != nullptr ) {
    auto* chargingInfo = new external_interface::Onboarding1p0ChargingInfo;
    chargingInfo->set_on_charger( _dVars.batteryInfo.onCharger );
    chargingInfo->set_needs_to_charge( _dVars.batteryInfo.needsToCharge );
    chargingInfo->set_suggested_charger_time( GetChargerTime() );
    
    PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.OnboardingStatus.ReceivedWakeup",
                  "Received wakeup. Responding with ok=%d, onCharger=%d, needsToCharger=%d, chargerTime=%f",
                  canWakeUp, _dVars.batteryInfo.onCharger, _dVars.batteryInfo.needsToCharge, GetChargerTime());
    
    auto* onboardingWakeUpResponse = new external_interface::OnboardingWakeUpResponse;
    onboardingWakeUpResponse->set_waking_up( canWakeUp );
    onboardingWakeUpResponse->set_allocated_charging_info( chargingInfo );
    gi->Broadcast( ExternalMessageRouter::WrapResponse(onboardingWakeUpResponse) );
  }
  
  if( _dVars.state == State::LookAtPhone ) { // only send DAS if in the correct state
    DASMSG(onboarding_wakeup, "onboarding.wakeup", "User tried to wake the robot up");
    DASMSG_SET(i1, canWakeUp, "Whether the robot is able to wake up (1) or if it is low battery (0)");
    DASMSG_SET(i2, (int) (1000 * GetActivatedDuration()), "Time elapsed (ms) before wake up");
    DASMSG_SET(i3, GetBEI().GetRobotInfo().IsOnChargerPlatform(), "Whether the robot is on the charger");
    DASMSG_SEND();
  }
  
  if( canWakeUp ) {
    if( IsControlDelegated() ) {
      _iConfig.behaviorLookAtPhone->ContinueReceived();
      // then wait for this behavior to end and the callback to transition to waking up
      SET_STATE( ReceivedWakeUp );
    } else {
      // somehow the phone behavior isnt running
      TransitionToWakingUp();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::TransitionToWakingUp()
{
  CancelDelegates( false );
  
  SET_STATE( WakingUp );
  auto* wakeUpAnimAction = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingWakeUp };
  DelegateIfInControl( wakeUpAnimAction, [this](){
    
    // tell app that wakeup finished, even if it was aborted, because it only happens once
    auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
    if( gi != nullptr ) {
      auto* onboardingWakeUpFinished = new external_interface::OnboardingWakeUpFinished;
      gi->Broadcast( ExternalMessageRouter::Wrap(onboardingWakeUpFinished) );
    }
    
    // if the user drops the robot, this action got aborted, and we should not try to play the next one until put down.
    // also, the if statement below checks != OnTreads instead of Falling so that this behavior effectively pauses if
    // dropped and then still held
    if( GetBEI().GetRobotInfo().GetOffTreadsState() != OffTreadsState::OnTreads ) {
      TransitionToWaitingForPutDown();
    } else {
      TransitionToDrivingOffCharger();
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::TransitionToDrivingOffCharger()
{
  // whether or not driving off the charger is successful (or even needed), don't do it again.
  // note that TransitionToDrivingOffCharger is called even if the robot is never on the charger,
  // so that if the robot is placed on the charger post-wakeup, it won't try to drive off again
  bool shouldDriveOffCharger = !_dVars.attemptedLeaveCharger;
  _dVars.attemptedLeaveCharger = true;
  
  // in case the user drops the robot when it's driving off the charger, mark it as complete now
  // instead of after driving off the charger
  MarkOnboardingComplete();
  
  if( shouldDriveOffCharger && GetBEI().GetRobotInfo().IsOnChargerPlatform() ) {
    CancelDelegates( false );
    SET_STATE( DriveOffCharger );
    auto* action = new TriggerLiftSafeAnimationAction{ AnimationTrigger::OnboardingDriveOffCharger_1p0 };
    DelegateIfInControl( action, [this](){
      TransitionToWaitingForVC();
    });
  } else {
    TransitionToWaitingForVC();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::TransitionToWaitingForVC()
{
  CancelDelegates( false );
  
  // from here on out, the wakeword is enabled, even if moved back to the charger or picked up
  EnableWakeWord();
  
  // set the timeout, unless the user has moved it on and off charger during this time
  if( _dVars.triggerWordStartTime_s < 0.0f ) {
    _dVars.triggerWordStartTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  }

  const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerPlatform();
  
  if( GetBEI().GetRobotInfo().GetOffTreadsState() != OffTreadsState::OnTreads ) {
    // behaviorWaitForTriggerWord(OnCharger) won't want to activate when picked up, so pass the time in
    // the waiting-to-be-put-down state
    TransitionToWaitingForPutDown();
  } else {
    IBehavior* behavior = nullptr;
    if( onCharger ) {
      SET_STATE( WaitForTriggerWordOnCharger );
      behavior = _iConfig.behaviorWaitForTriggerWordOnCharger.get();
    } else {
      SET_STATE( WaitForTriggerWord );
      behavior = _iConfig.behaviorWaitForTriggerWord.get();
    }
    CancelDelegates( false );
    if( ANKI_VERIFY( behavior->WantsToBeActivated(), "BehaviorOnboarding.TransitionToWaitingForVC.DoesntWTBA",
                    "Behavior %s doesnt want to be activated",
                    (onCharger ? BehaviorIDToString(kBehaviorIDWhileWaitingOnCharger)
                               : BehaviorIDToString(kBehaviorIDWhileWaiting)) ) )
    {
      DelegateIfInControl( behavior, [this](){
        // keep doing this if it somehow ends
        TransitionToWaitingForVC();
      });
    } else {
      const bool timeout = true; // an error is closer to a timeout than a voice command
      TerminateOnboarding(timeout);
    }
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::TransitionToWaitingForPutDown()
{
  CancelDelegates( false );
  
  SET_STATE( WaitForPutDown );
  if( _dVars.treadsStateEndTime_s < 0.0f ) {
    // just in case the robot never returns from being off treads, help it get past this step with a timeout
    _dVars.treadsStateEndTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kTreadsTimeout_s;
  }
  
  auto waitForOnTreads = [](Robot& robot) {
    return robot.GetOffTreadsState() == OffTreadsState::OnTreads;
  };
  
  auto* action = new WaitForLambdaAction{ waitForOnTreads, std::numeric_limits<float>::max() };
  DelegateIfInControl( action, [this](){
    if( GetBEI().GetRobotInfo().GetOffTreadsState() != OffTreadsState::OnTreads ) {
      // still not on treads... was likely falling again, causing this action to abort
      TransitionToWaitingForPutDown();
    } else {
      TransitionToDrivingOffCharger();
    }
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::TransitionToTriggerWord()
{
  CancelDelegates( false );
  SET_STATE( TriggerWord );
  DelegateIfInControl( _iConfig.behaviorTriggerWord.get(), [this](){
    auto& uic = GetBehaviorComp<UserIntentComponent>();
    if( uic.IsAnyUserIntentPending() ) {
      const bool timeout = false;
      TerminateOnboarding( timeout );
    } else {
      TransitionToWaitingForVC();
    }
  });
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::TransitionToPoweringOff()
{
  CancelDelegates( false );
  SET_STATE( PowerOff );
  DelegateIfInControl( _iConfig.behaviorPowerOff.get(), [this](){
    // power button was released
    TransitionToWaitingForVC();
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::DisableWakeWord()
{
  if( _dVars.wakeWordState == WakeWordState::Disabled ) {
    return;
  } else if( _dVars.wakeWordState == WakeWordState::Enabled ) {
    SmartPopResponseToTriggerWord();
  }
  
  _dVars.wakeWordState = WakeWordState::Disabled;
  
  SmartPushResponseToTriggerWord();
  SmartDisableEngineResponseToTriggerWord();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::EnableWakeWord()
{
  if( _dVars.wakeWordState == WakeWordState::Enabled ) {
    return;
  } else if( _dVars.wakeWordState == WakeWordState::Disabled ) {
    SmartPopResponseToTriggerWord();
  }
  
  _dVars.wakeWordState = WakeWordState::Enabled;
  
  SmartEnableEngineResponseToTriggerWord();
  namespace AECH = AudioEngine::Multiplexer::CladMessageHelper;
  const auto postAudioEvent
    = AECH::CreatePostAudioEvent( AudioMetaData::GameEvent::GenericEvent::Play__Robot_Vic_Sfx__Wake_Word_On,
                                  AudioMetaData::GameObjectType::Behavior, 0 );
  SmartPushResponseToTriggerWord( AnimationTrigger::VC_ListeningGetIn, postAudioEvent, StreamAndLightEffect::StreamingEnabled );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::HandleWhileActivated(const AppToEngineEvent& event)
{
  if( event.GetData().GetTag() == AppToEngineTag::kOnboardingWakeUpRequest ) {
    // flag to wake up (don't start it now so that app messages and console vars are both applied at the same time)
    _dVars.receivedWakeUp = true;
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::UpdateBatteryInfo( bool sendEvent )
{
  const float chargerTime_s = GetChargerTime();
  const bool needsToCharge = (chargerTime_s != 0.0f);
  const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerContacts();
  
  if( (needsToCharge != _dVars.batteryInfo.needsToCharge) || (onCharger != _dVars.batteryInfo.onCharger) ) {
    _dVars.batteryInfo.needsToCharge = needsToCharge;
    _dVars.batteryInfo.onCharger = onCharger;
    
    PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.OnboardingStatus.UpdateBatteryInfo",
                  "Charging status changed: onCharger=%d, needsToCharge=%d, chargerTime=%f",
                  _dVars.batteryInfo.onCharger, _dVars.batteryInfo.needsToCharge, chargerTime_s);
    
    // send message
    auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
    if( sendEvent && (gi != nullptr) ) {
      auto* msg = new external_interface::Onboarding1p0ChargingInfo;
      msg->set_on_charger( onCharger );
      msg->set_needs_to_charge( needsToCharge );
      msg->set_suggested_charger_time( chargerTime_s );
      gi->Broadcast( ExternalMessageRouter::Wrap(msg) );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorOnboarding1p0::GetChargerTime() const
{
  const float chargerTime_s = GetBEI().GetRobotInfo().GetBatteryComponent().GetSuggestedChargerTime();
  return chargerTime_s;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboarding1p0::IsBatteryCountdownDone() const
{
  const float chargerTime_s = GetBEI().GetRobotInfo().GetBatteryComponent().GetSuggestedChargerTime();
  const bool done = (chargerTime_s == 0.0f);
  return done;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::MarkOnboardingComplete()
{
  if( _dVars.markedComplete ) {
    return;
  }
  _dVars.markedComplete = true;
  
  SetStage( OnboardingStages::Complete );
}
  
void BehaviorOnboarding1p0::SetStage( OnboardingStages stage, bool forceSkipStackReset )
{
  
  SaveToDisk( stage );
  
  // broadcast
  if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    ExternalInterface::OnboardingState msg{ stage, forceSkipStackReset };
    ei->Broadcast( ExternalInterface::MessageEngineToGame{std::move(msg)} );
  }
  
  // save to whiteboard for convenience
  {
    auto& whiteboard = GetAIComp<AIWhiteboard>();
    whiteboard.SetMostRecentOnboardingStage( stage );
  }
  
  // log
  PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.MarkOnboardingComplete.OnboardingStatus",
                "Onboarding moved to stage %s", OnboardingStagesToString(stage));
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::TerminateOnboarding( bool timeout )
{
  // we shouldn't CancelSelf() because this is the base behavior in the stack. But we can
  // tell the BehaviorsBootLoader to reboot with a different stack
  if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    ExternalInterface::OnboardingState state;
    state.stage = OnboardingStages::Complete;
    state.forceSkipStackReset = false; // reset the stack into normal operation
    ei->Broadcast( ExternalInterface::MessageEngineToGame{std::move(state)} );
  }
  _dVars.state = State::WaitingForTermination;
  
  {
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    DASMSG(onboarding_terminated, "onboarding.terminated", "Onboarding ended, and why");
    DASMSG_SET(i1, timeout, "Whether the behavior timed out waiting for a trigger word");
    DASMSG_SET(i2, (int)(1000 * (currTime_s - _dVars.triggerWordStartTime_s)), "Duration of waiting for trigger word (ms)");
    DASMSG_SET(i3, (int)(1000 * GetActivatedDuration()), "Total onboarding duration (ms)");
    DASMSG_SEND();
  }
  
  PRINT_CH_INFO("Behaviors", "BehaviorOnboarding.TerminateOnboarding.OnboardingStatus",
                "Terminating onboarding (timeout? = %d)", timeout);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboarding1p0::SaveToDisk( OnboardingStages stage ) const
{
  Json::Value toSave;
  toSave[kOnboardingStageKey] = OnboardingStagesToString( stage );
  const std::string filename = _iConfig.saveFolder + kOnboardingFilename;
  Util::FileUtils::WriteFile( filename, toSave.toStyledString() );
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboarding1p0::ShouldCheckPowerOff() const
{
  switch( _dVars.state ) {
    case State::WaitForPutDown:
    case State::WaitForTriggerWord:
    case State::WaitForTriggerWordOnCharger:
    case State::TriggerWord:
      return true;
    case State::NotStarted:
    case State::LookAtPhone:
    case State::ReceivedWakeUp:
    case State::WakingUp:
    case State::DriveOffCharger:
    case State::PowerOff:
    case State::WaitingForTermination:
      return false;
  }
}


}
}
