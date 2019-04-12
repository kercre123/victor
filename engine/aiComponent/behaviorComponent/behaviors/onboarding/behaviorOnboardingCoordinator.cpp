/**
 * File: BehaviorOnboardingCoordinator.cpp
 *
 * Author: Sam Russell
 * Created: 2018-10-26
 *
 * Description: Modular App driven onboarding extension of Hail Mary(1p0)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/behaviorOnboardingCoordinator.h"

#include "clad/types/onboardingPhase.h"
#include "clad/types/onboardingPhaseState.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding/phases/iOnboardingPhaseWithProgress.h"
#include "engine/aiComponent/behaviorComponent/onboardingMessageHandler.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/components/cubes/cubeCommsComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/cladProtoTypeTranslator.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/moodSystem/moodManager.h"

#include "util/console/consoleInterface.h"
#include "util/logging/DAS.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

const std::string BehaviorOnboardingCoordinator::kOnboardingFolder   = "onboarding";
const std::string BehaviorOnboardingCoordinator::kOnboardingFilename = "onboardingState.json";
const std::string BehaviorOnboardingCoordinator::kOnboardingStageKey = "onboardingStage";
const std::string BehaviorOnboardingCoordinator::kOnboardingTimeKey = "onboardingCompletionTime";

namespace {
  const char* kDebugLabel = "BehaviorOnboardingCoordinator";

  const char* kPhasesKey          = "phases";
  const char* kPhaseNameKey       = "phaseName";
  const char* kPhaseBehaviorIDKey = "phaseBehaviorID";
  const char* kPhaseTimeoutKey    = "phaseTimeout_s";
  const char* kAllowPowerOffKey   = "allowCoordinatorPowerOff";

  const char* kStartTimeoutKey    = "startTimeout_s";
  const char* kFinishTimeoutKey   = "completionTimeout_s";
  const char* kAppDisconnectKey   = "appDisconnectTimeout_s";
  const char* kIdleTimeoutKey     = "idleTimeout_s";

  const char* kPowerOffFromIdleKey = "allowPowerOffFromIdle";

  const float kBattInfoUpdateTime_s = 5.0f;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingCoordinator::InstanceConfig::InstanceConfig(const Json::Value& config)
{
  const Json::Value& phases = config[kPhasesKey];
  for( const Json::Value& phase : phases ){
    std::string phaseName = JsonTools::ParseString(phase, kPhaseNameKey, kDebugLabel);
    OnboardingPhase onboardingPhase;
    if( EnumFromString(phaseName, onboardingPhase) ) {
      OnboardingPhaseData phaseData;
      phaseData.behaviorIDStr = JsonTools::ParseString(phase, kPhaseBehaviorIDKey, kDebugLabel);
      phaseData.timeout_s     = JsonTools::ParseFloat(phase, kPhaseTimeoutKey, kDebugLabel);
      phaseData.allowPowerOff = false;
      JsonTools::GetValueOptional(phase, kAllowPowerOffKey, phaseData.allowPowerOff);

      OnboardingPhaseMap.insert({onboardingPhase, phaseData});
    }
    else{
      LOG_ERROR("BehaviorOnboardingCoordinator.InstanceConfigCtor.InvalidOnboardingPhase",
                "%s, specified in onboarding.json is not a valid OnboardingPhase.",
                phaseName.c_str());
    }
  }

  auto iter = OnboardingPhaseMap.find(OnboardingPhase::Default);
  if(OnboardingPhaseMap.end() == iter){
    LOG_ERROR("BehaviorOnboardingCoordinator.InvalidDefaultPhase",
              "A valid 'Default' phase must be specified in Onboarding.json!");
  }

  startTimeout_s           = JsonTools::ParseFloat(config, kStartTimeoutKey,   kDebugLabel);
  completionTimeout_s      = JsonTools::ParseFloat(config, kFinishTimeoutKey,  kDebugLabel);
  appDisconnectTimeout_s   = JsonTools::ParseFloat(config, kAppDisconnectKey,  kDebugLabel);
  idleTimeout_s            = JsonTools::ParseFloat(config, kIdleTimeoutKey,    kDebugLabel);
  allowPowerOffFromIdle    = JsonTools::ParseBool(config, kPowerOffFromIdleKey, kDebugLabel);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingCoordinator::DynamicVariables::DynamicVariables()
: pendingPhase(OnboardingPhase::InvalidPhase)
, lastSetPhase(OnboardingPhase::InvalidPhase)
, lastSetPhaseBehavior(nullptr)
, lastSetPhaseState(OnboardingPhaseState::PhaseInvalid)
, currentPhase(OnboardingPhase::InvalidPhase)
, nextTimeSendBattInfo_s(0.0f)
, globalExitTime_s(0.0f)
, phaseExitTime_s(0.0f)
, appDisconnectExitTime_s(0.0f)
, markCompleteAndExitOnNextUpdate(false)
, skipOnboardingOnNextUpdate(false)
, emulate1p0Onboarding(false)
, started1p0WakeUp(false)
, onboardingStarted(false)
, waitingForAppReconnect(false)
, waitingForTermination(false)
, terminatedNaturally(true)
, shouldCheckPowerOff(false)
, isStimMaxed(false)
, poweringOff(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorOnboardingCoordinator::BehaviorOnboardingCoordinator(const Json::Value& config)
 : ICozmoBehavior(config)
 , _iConfig(config)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::GetBehaviorJsonKeys(std::set<const char*>& expectedKeys) const
{
  const char* list[] = {
    kPhasesKey,
    kPhaseNameKey,
    kPhaseBehaviorIDKey,
    kPhaseTimeoutKey,
    kAllowPowerOffKey,
    kStartTimeoutKey,
    kFinishTimeoutKey,
    kAppDisconnectKey,
    kIdleTimeoutKey,
    kPowerOffFromIdleKey
  };
  expectedKeys.insert( std::begin(list), std::end(list) );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingCoordinator::WantsToBeActivatedBehavior() const
{
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
  modifiers.visionModesForActiveScope->insert({ VisionMode::Faces, EVisionUpdateFrequency::Med });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.behaviorPowerOff.get());
  for(auto& pair : _iConfig.OnboardingPhaseMap){
    delegates.insert( pair.second.behavior.get() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::InitBehavior()
{
  // Subscribe for App messages via the OnboardingMessageHandler
  GetBehaviorComp<OnboardingMessageHandler>().SubscribeToAppTags({
    AppToEngineTag::kOnboardingRestart,
    AppToEngineTag::kOnboardingWakeUpRequest,
    AppToEngineTag::kOnboardingWakeUpStartedRequest,
    AppToEngineTag::kOnboardingSetPhaseRequest,
    AppToEngineTag::kOnboardingPhaseProgressRequest,
    AppToEngineTag::kOnboardingChargeInfoRequest,
    AppToEngineTag::kOnboardingSkipOnboarding,
    AppToEngineTag::kOnboardingMarkCompleteAndExit,
    AppToEngineTag::kAppDisconnected
  },
  std::bind(&BehaviorOnboardingCoordinator::HandleOnboardingMessageFromApp, this, std::placeholders::_1));

  const auto& BC = GetBEI().GetBehaviorContainer();
  _iConfig.behaviorPowerOff = BC.FindBehaviorByID( BEHAVIOR_ID(OnboardingPowerOff) );

  for(auto& pair : _iConfig.OnboardingPhaseMap){
    pair.second.behavior = BC.FindBehaviorByID(BehaviorTypesWrapper::BehaviorIDFromString(pair.second.behaviorIDStr));
    if(nullptr == pair.second.behavior){
      LOG_ERROR("BehaviorOnboardingCoordinator.InitBehavior.InvalidBehaviorID",
                "%s is not a valid behaviorID",
                pair.second.behaviorIDStr.c_str());
    }
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

  // Dev console utilities
  if( ANKI_DEV_CHEATS ) {
    auto clearOnboardingDataFunc = [this](ConsoleFunctionContextRef context){
      const std::string filename = _iConfig.saveFolder + kOnboardingFilename;
      Util::FileUtils::DeleteFile( filename );
      GetBEI().GetCubeCommsComponent().ForgetPreferredCube();
      if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
        auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
        ei->Broadcast(ExternalInterface::MessageGameToEngine{ExternalInterface::EraseAllEnrolledFaces{}});
      }
    };
    _iConfig.consoleFuncs.emplace_front("Clear Onboarding Data",
                                        std::move(clearOnboardingDataFunc),
                                        "OnboardingCoordinator",
                                        "");

    for(auto& phaseDataPair : _iConfig.OnboardingPhaseMap){
      OnboardingPhase phase = phaseDataPair.first;
      auto setPhaseFunc = [this, phase](ConsoleFunctionContextRef context){
        external_interface::GatewayWrapper wrapper;
        auto* request = new external_interface::OnboardingSetPhaseRequest;
        request->set_phase(CladProtoTypeTranslator::ToProtoEnum(phase));
        wrapper.set_allocated_onboarding_set_phase_request(request);
        auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
        gi->Broadcast(wrapper);
      };
      auto funcNameStr = "Set Phase: " + std::string(EnumToString(phase));
      _iConfig.consoleFuncs.emplace_front(funcNameStr, std::move(setPhaseFunc), "OnboardingCoordinator", "");
    }
  }// Dev console utilities
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::OnBehaviorActivated()
{
  // If the behavior exited naturally, reset dynamic variables. Otherwise, assume we were interrupted
  // by something and pick up where we left off to maintain App Authoritative design if at all possible
  if(_dVars.terminatedNaturally){
    _dVars = DynamicVariables();
    // If this isn't set to true before exiting, onboarding was interrupted, probably by an infoFace feature like
    // mute or customer care
    _dVars.terminatedNaturally = false;

    float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _dVars.globalExitTime_s = (_iConfig.startTimeout_s > 0.0f) ? (currTime_s + _iConfig.startTimeout_s) : 0.0f;

    TransitionToPhase(OnboardingPhase::Default);
  } else {
    if(OnboardingPhase::InvalidPhase != _dVars.currentPhase){
      const bool appCommandedTransition = false;
      const bool resumingPhaseAfterInterruption = true;
      TransitionToPhase(_dVars.currentPhase, appCommandedTransition, resumingPhaseAfterInterruption);
    }
  }

  // init battery info, but don't msg the app
  const bool sendEvent = false;
  UpdateBatteryInfo(sendEvent);

  FixStimAtMax();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::HandleOnboardingMessageFromApp(const AppToEngineEvent& event)
{
  switch( event.GetData().GetTag() ){
    case AppToEngineTag::kOnboardingRestart:
    {
      RestartOnboarding();
      break;
    }
    // kOnboardingWakeUpRequest and kOnboardingWakeUpStartedRequest are maintained for backward compatiblity
    // with older app versions which use "hail mary" onboarding sequence
    case AppToEngineTag::kOnboardingWakeUpRequest:
    {
      _dVars.emulate1p0Onboarding = true;
      const float chargerTime_s = GetBEI().GetRobotInfo().GetBatteryComponent().GetSuggestedChargerTime();
      const bool canWakeUp = (chargerTime_s == 0.0f);
      Send1p0WakeUpResponseToApp(canWakeUp);
      // Internal handling of the emulation
      if(canWakeUp){
        _dVars.pendingPhase = OnboardingPhase::WakeUp;
        _dVars.started1p0WakeUp = true;
      }
      break;
    }
    case AppToEngineTag::kOnboardingWakeUpStartedRequest:
    {
      _dVars.emulate1p0Onboarding = true;
      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      if( nullptr != gi ) {
        // tell app wakeup started if we already received the wakeup command from the app
        auto* msg = new external_interface::OnboardingWakeUpStartedResponse{ _dVars.started1p0WakeUp };
        gi->Broadcast( ExternalMessageRouter::WrapResponse(msg) );
      }
      break;
    }
    case AppToEngineTag::kOnboardingSetPhaseRequest:
    {
      _dVars.pendingPhase = CladProtoTypeTranslator::ToCladEnum(event.GetData().onboarding_set_phase_request().phase());
      _dVars.lastSetPhase = _dVars.pendingPhase;
      auto iter = _iConfig.OnboardingPhaseMap.find(_dVars.lastSetPhase);
      _dVars.lastSetPhaseBehavior = (_iConfig.OnboardingPhaseMap.end() != iter) ? iter->second.behavior : nullptr;
      _dVars.lastSetPhaseState = OnboardingPhaseState::PhasePending;
      SendSetPhaseResponseToApp();
      break;
    }
    case AppToEngineTag::kOnboardingPhaseProgressRequest:
    {
      SendPhaseProgressResponseToApp();
      break;
    }
    case AppToEngineTag::kOnboardingChargeInfoRequest:
    {
      SendChargeInfoResponseToApp();
      break;
    }
    case AppToEngineTag::kOnboardingSkipOnboarding:
    {
      _dVars.skipOnboardingOnNextUpdate = true;
      break;
    }
    case AppToEngineTag::kOnboardingMarkCompleteAndExit:
    {
      _dVars.markCompleteAndExitOnNextUpdate = true;
      break;
    }
    case AppToEngineTag::kAppDisconnected:
    {
      // If we were running 1p0 onboarding emulation and we've already started waking up, don't react to the
      // lost connection, just let the emulation behavior finish up onboarding naturally on a 60 sec timeout
      // or the first voice command
      if( !_dVars.emulate1p0Onboarding || !_dVars.started1p0WakeUp ){
        // Don't timeout based on AppDisconnected messages if we haven't yet started onboarding
        if( _dVars.onboardingStarted ){
          const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
          const bool appDisconnectTimeOutSet = _iConfig.appDisconnectTimeout_s > 0.0f;
          _dVars.appDisconnectExitTime_s = appDisconnectTimeOutSet ?
            (currTime_s + _iConfig.appDisconnectTimeout_s) : 0.0f;
        }
      }
      // This particular message should NOT reset appDisconnect params; return early
      return;
    }
    default:
    {
      LOG_WARNING("BehaviorOnboardingCoordinator.HandleOnboardingMessageFromApp.UnhandledAppMessageReceived",
                  "Received App message which is not yet handled");
      // Simply return for unsupported messages
      return;
    }
  }

  // If we handled the app message at all, consider the app reconnected
  _dVars.appDisconnectExitTime_s = 0.0f;
  _dVars.waitingForAppReconnect = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::OnBehaviorDeactivated()
{
  UnFixStim();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::BehaviorUpdate()
{
  if( !IsActivated() || _dVars.poweringOff || _dVars.waitingForTermination ){
    return;
  }

  if(_dVars.markCompleteAndExitOnNextUpdate){
    TerminateOnboarding();
    SaveToDisk(OnboardingStages::Complete);
    return;
  } else if(_dVars.skipOnboardingOnNextUpdate){
    // Skipping is treated just like it never started, and should overwrite previous timeouts
    TerminateOnboarding();
    SaveToDisk(OnboardingStages::NotStarted);
    return;
  }

  float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  if( OnboardingPhase::InvalidPhase != _dVars.pendingPhase ){
    LOG_INFO("BehaviorOnboardingCoordinator.PendingPhase",
             "Transitioning to phase: %s from App command",
             EnumToString(_dVars.pendingPhase));
    const bool sendSetPhaseResponseToApp = true;
    TransitionToPhase(_dVars.pendingPhase, sendSetPhaseResponseToApp);
    _dVars.pendingPhase = OnboardingPhase::InvalidPhase;
  }
  else if( _dVars.appDisconnectExitTime_s != 0.0f && !_dVars.waitingForAppReconnect ){
    LOG_INFO("BehaviorOnboardingCoordinator.AppDisconnectDetected",
             "App disconnect detected, transitioning to default phase");
    TransitionToPhase(OnboardingPhase::Default);
    _dVars.waitingForAppReconnect = true;
  }

  UpdateBatteryInfo();
  // If the battery is low, tell the App, but don't do anything special other than that
  if( (_dVars.batteryInfo.needsToCharge && !_dVars.batteryInfo.onCharger) &&
      ( (_dVars.nextTimeSendBattInfo_s > 0.0f) &&  (_dVars.nextTimeSendBattInfo_s < currTime_s) ) ){
    SendBatteryInfoToApp();
  }

  if( ShouldExitDueToTimeout() ){
    TerminateOnboarding();
    // For now we don't distinguish between OnboardingStages::Complete and OnboardingStages::TimedOut for any
    // functional purpose, so just mark complete. In the future we could use TimedOut to deliberately offer users
    // the option to redo onboarding if it times out on them.
    SaveToDisk(OnboardingStages::Complete);
    return;
  }

  if( _dVars.shouldCheckPowerOff ) {
    if( !_iConfig.behaviorPowerOff->IsActivated() && _iConfig.behaviorPowerOff->WantsToBeActivated() ) {
      LOG_INFO("BehaviorOnboardingCoordinator.PowerButtonPressDetected",
               "Power button pressed, delegating to Power Off behavior");
      TransitionToPoweringOff();
    }
  }

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::TransitionToPhase(const OnboardingPhase& phase,
                                                      bool appCommandedTransition,
                                                      bool resumingPhaseAfterInterruption)
{
  // App Authoritative. Abandon anything we're currently doing without callbacks
  CancelDelegates(false);

  const auto currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  if( !_dVars.onboardingStarted && appCommandedTransition ){
    _dVars.globalExitTime_s = (_iConfig.completionTimeout_s > 0.0f) ? (currTime_s + _iConfig.completionTimeout_s) : 0.0f;
    _dVars.onboardingStarted = true;
  }

  auto iter = _iConfig.OnboardingPhaseMap.find(phase);
  if( _iConfig.OnboardingPhaseMap.end() != iter ){
    ICozmoBehaviorPtr behavior = iter->second.behavior;
    if( behavior->WantsToBeActivated() ){

      if(resumingPhaseAfterInterruption){
        IOnboardingPhaseWithProgress* phaseWithProgress = dynamic_cast<IOnboardingPhaseWithProgress*>(behavior.get());
        if(phaseWithProgress){
          phaseWithProgress->ResumeUponNextActivation();
        }
      }
      else {
        // Only reset the phase timeout if this transition is NOT resuming after an interruption
        const bool phaseHasTimeout = iter->second.timeout_s > 0.0f;
        _dVars.phaseExitTime_s = phaseHasTimeout ? (currTime_s + iter->second.timeout_s) : 0.0f;
      }

      if( appCommandedTransition ){
        _dVars.lastSetPhaseState = OnboardingPhaseState::PhaseInProgress;
      }

      _dVars.currentPhase = iter->first;
      _dVars.shouldCheckPowerOff = iter->second.allowPowerOff;

      DelegateIfInControl(behavior.get(),
        [this, phase](){
          OnPhaseComplete(phase);
        });
    }
    else {
      LOG_ERROR("BehaviorOnboardingCoordinator.TransitionToPhase.PhaseBehaviorFailure",
                "Phase %s behavior did not want to be activated. Setting Default phase if available",
                EnumToString(phase));
      if( OnboardingPhase::Default != phase ){
        TransitionToPhase(OnboardingPhase::Default);
      }
      else{
        // If we can't get to the default phase, exit onboarding and don't re-enter it... its broken.
        TerminateOnboarding();
        SaveToDisk(OnboardingStages::Complete);
        return;
      }

      if( appCommandedTransition ){
        _dVars.lastSetPhaseState = OnboardingPhaseState::PhaseInvalid;
      }
    }
  }
  else {
    LOG_ERROR("BehaviorOnboardingCoordinator.TransitionToPhase.InvalidPhase",
              "No behavior specified for phase %s in the onboarding coordinator.",
              EnumToString(phase));
    if( OnboardingPhase::Default != phase ){
      TransitionToPhase(OnboardingPhase::Default);
    }
    else{
      // If the "default" phase is invalid, exit onboarding and don't re-enter it... its broken.
      TerminateOnboarding();
      SaveToDisk(OnboardingStages::Complete);
      return;
    }

    if( appCommandedTransition ){
      _dVars.lastSetPhaseState = OnboardingPhaseState::PhaseInvalid;
    }
  }

  // Send the SetPhaseResponse back to the App
  if( appCommandedTransition ){
    SendSetPhaseResponseToApp();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::OnPhaseComplete(const OnboardingPhase& phase)
{
  LOG_INFO("BehaviorOnboardingCoordinator.OnPhaseComplete",
           "Completed onboarding phase %s. Idling.",
           EnumToString(phase));

  if( _dVars.emulate1p0Onboarding ){
    if( OnboardingPhase::WakeUp == phase ){
      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      if( gi != nullptr ) {
        auto* onboardingWakeUpFinished = new external_interface::OnboardingWakeUpFinished;
        gi->Broadcast( ExternalMessageRouter::Wrap(onboardingWakeUpFinished) );
      }
      TransitionToPhase(OnboardingPhase::Emulate1p0WaitForVC);
    } else if ( OnboardingPhase::Emulate1p0WaitForVC == phase ){
      // If we come back from the Emulation phase, always exit. We either got a voice command
      // or something broke
      TerminateOnboarding();
      SaveToDisk(OnboardingStages::Complete);
    }
    return;
  }

  const auto currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool hasIdleTimeout = _iConfig.idleTimeout_s > 0.0f;
  _dVars.phaseExitTime_s = hasIdleTimeout ? (currTime_s + _iConfig.idleTimeout_s) : 0.0f;
  _dVars.shouldCheckPowerOff = _iConfig.allowPowerOffFromIdle;
  _dVars.currentPhase = OnboardingPhase::InvalidPhase;
  _dVars.lastSetPhaseState = OnboardingPhaseState::PhaseComplete;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::TransitionToPoweringOff()
{
  CancelDelegates( false );
  _dVars.poweringOff = true;

  DelegateIfInControl( _iConfig.behaviorPowerOff.get(), [this](){
    // power button was released
    _dVars.poweringOff = false;
    // Restore whatever phase we were in when the power button was pressed
    if( OnboardingPhase::InvalidPhase != _dVars.currentPhase ){
      const bool appCommandedTransition = false;
      // pick up where we left off, if possible, and don't reset phase timeouts
      const bool resumingPhaseAfterInterruption = true;
      TransitionToPhase(_dVars.currentPhase, appCommandedTransition, resumingPhaseAfterInterruption);
    }
  });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::UpdateBatteryInfo( bool sendEvent )
{
  const float chargerTime_s = GetChargerTime();
  const bool needsToCharge = (chargerTime_s != 0.0f);
  const bool onCharger = GetBEI().GetRobotInfo().IsOnChargerContacts();

  if( (needsToCharge != _dVars.batteryInfo.needsToCharge) || (onCharger != _dVars.batteryInfo.onCharger) ) {
    _dVars.batteryInfo.chargerTime_s = chargerTime_s;
    _dVars.batteryInfo.needsToCharge = needsToCharge;
    _dVars.batteryInfo.onCharger = onCharger;

    PRINT_CH_INFO("Behaviors", "BehaviorOnboardingCoordinator.OnboardingStatus.UpdateBatteryInfo",
                  "Charging status changed: onCharger=%d, needsToCharge=%d, chargerTime=%f",
                  _dVars.batteryInfo.onCharger, _dVars.batteryInfo.needsToCharge, chargerTime_s);

    // send message
    if( sendEvent ) {
      SendBatteryInfoToApp();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::SendBatteryInfoToApp()
{
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( nullptr != gi ) {
    auto* msg = new external_interface::Onboarding1p0ChargingInfo;
    msg->set_on_charger( _dVars.batteryInfo.onCharger );
    msg->set_needs_to_charge( _dVars.batteryInfo.needsToCharge );
    msg->set_suggested_charger_time( _dVars.batteryInfo.chargerTime_s );
    gi->Broadcast( ExternalMessageRouter::Wrap(msg) );
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _dVars.nextTimeSendBattInfo_s = (kBattInfoUpdateTime_s > 0.0f) ? (currTime_s + kBattInfoUpdateTime_s) : 0.0f;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float BehaviorOnboardingCoordinator::GetChargerTime() const
{
  const float chargerTime_s = GetBEI().GetRobotInfo().GetBatteryComponent().GetSuggestedChargerTime();
  return chargerTime_s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingCoordinator::IsBatteryCountdownDone() const
{
  const float chargerTime_s = GetBEI().GetRobotInfo().GetBatteryComponent().GetSuggestedChargerTime();
  const bool done = (chargerTime_s == 0.0f);
  return done;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::SendSetPhaseResponseToApp()
{
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if(nullptr != gi){
    auto* msg = new external_interface::OnboardingSetPhaseResponse;
    if(!IsActivated()){
      // Indicate to the App that this message was handled atypically incase the App wants to disregard results
      msg->set_allocated_status(new external_interface::ResponseStatus(
        external_interface::ResponseStatus::StatusCode::ResponseStatus_StatusCode_UNKNOWN));
    } else {
      msg->set_allocated_status(new external_interface::ResponseStatus(
        external_interface::ResponseStatus::StatusCode::ResponseStatus_StatusCode_OK));
    }
    msg->set_phase(CladProtoTypeTranslator::ToProtoEnum(_dVars.lastSetPhase));
    msg->set_phase_state(CladProtoTypeTranslator::ToProtoEnum(_dVars.lastSetPhaseState));
    gi->Broadcast( ExternalMessageRouter::WrapResponse(msg) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::SendChargeInfoResponseToApp()
{
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( nullptr != gi ) {
    auto* msg = new external_interface::OnboardingChargeInfoResponse;
    if(!IsActivated()){
      // Indicate to the App that this message was handled atypically incase the App wants to disregard results
      msg->set_allocated_status(new external_interface::ResponseStatus(
        external_interface::ResponseStatus::StatusCode::ResponseStatus_StatusCode_UNKNOWN));
    } else {
      msg->set_allocated_status(new external_interface::ResponseStatus(
        external_interface::ResponseStatus::StatusCode::ResponseStatus_StatusCode_OK));
    }
    msg->set_on_charger( _dVars.batteryInfo.onCharger );
    msg->set_needs_to_charge( _dVars.batteryInfo.needsToCharge );
    msg->set_required_charge_time( _dVars.batteryInfo.chargerTime_s );
    gi->Broadcast( ExternalMessageRouter::WrapResponse(msg) );
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _dVars.nextTimeSendBattInfo_s = (kBattInfoUpdateTime_s > 0.0f) ? (currTime_s + kBattInfoUpdateTime_s) : 0.0f;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::Send1p0WakeUpResponseToApp(bool canWakeUp)
{
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( nullptr != gi ) {
    auto* chargingInfo = new external_interface::Onboarding1p0ChargingInfo;
    chargingInfo->set_on_charger( _dVars.batteryInfo.onCharger );
    chargingInfo->set_needs_to_charge( _dVars.batteryInfo.needsToCharge );
    chargingInfo->set_suggested_charger_time( GetChargerTime() );

    LOG_INFO("BehaviorOnboarding.OnboardingStatus.Received1p0Wakeup",
            "Received wakeup. Responding with ok=%d, onCharger=%d, needsToCharger=%d, chargerTime=%f",
            canWakeUp, _dVars.batteryInfo.onCharger, _dVars.batteryInfo.needsToCharge, GetChargerTime());

    auto* onboardingWakeUpResponse = new external_interface::OnboardingWakeUpResponse;
    onboardingWakeUpResponse->set_waking_up( canWakeUp );
    onboardingWakeUpResponse->set_allocated_charging_info( chargingInfo );
    gi->Broadcast( ExternalMessageRouter::WrapResponse(onboardingWakeUpResponse) );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::SendPhaseProgressResponseToApp()
{
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if(nullptr != gi){
    auto* msg = new external_interface::OnboardingPhaseProgressResponse;
    if(!IsActivated()){
      // Indicate to the App that this message was handled atypically incase the App wants to disregard results
      msg->set_allocated_status(new external_interface::ResponseStatus(
        external_interface::ResponseStatus::StatusCode::ResponseStatus_StatusCode_UNKNOWN));
    } else {
      msg->set_allocated_status(new external_interface::ResponseStatus(
        external_interface::ResponseStatus::StatusCode::ResponseStatus_StatusCode_OK));
    }
    msg->set_last_set_phase(CladProtoTypeTranslator::ToProtoEnum(_dVars.lastSetPhase));
    msg->set_last_set_phase_state(CladProtoTypeTranslator::ToProtoEnum(_dVars.lastSetPhaseState));

    IOnboardingPhaseWithProgress* phaseWithProgress =
      dynamic_cast<IOnboardingPhaseWithProgress*>(_dVars.lastSetPhaseBehavior.get());
    int progress = (nullptr != phaseWithProgress) ? phaseWithProgress->GetPhaseProgressInPercent() : 0.0f;

    msg->set_percent_completed(progress);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(msg) );
}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::RestartOnboarding()
{
  if(IsActivated()){
    CancelDelegates(false);
  }

  _dVars.waitingForTermination = true;
  _dVars.terminatedNaturally = true;

  // we shouldn't CancelSelf() because this is the base behavior in the stack. But we can
  // tell the BehaviorsBootLoader to reboot with a different stack
  if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    ExternalInterface::OnboardingState state;
    state.stage = OnboardingStages::NotStarted;
    state.forceSkipStackReset = false; // reset the stack into normal operation
    ei->Broadcast(ExternalInterface::MessageEngineToGame{std::move(state)});
  }

  SaveToDisk(OnboardingStages::NotStarted);
  LOG_INFO("BehaviorOnboardingCoordinator.RestartOnboarding.OnboardingStatus",
           "Restarting onboarding");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::TerminateOnboarding()
{
  CancelDelegates(false);
  _dVars.waitingForTermination = true;
  _dVars.terminatedNaturally = true;

  // we shouldn't CancelSelf() because this is the base behavior in the stack. But we can
  // tell the BehaviorsBootLoader to reboot with a different stack
  if( GetBEI().GetRobotInfo().HasExternalInterface() ) {
    auto* ei = GetBEI().GetRobotInfo().GetExternalInterface();
    ExternalInterface::OnboardingState state;
    state.stage = OnboardingStages::Complete;
    state.forceSkipStackReset = false; // reset the stack into normal operation
    ei->Broadcast(ExternalInterface::MessageEngineToGame{std::move(state)});
  }

  LOG_INFO("BehaviorOnboardingCoordinator.TerminateOnboarding.OnboardingStatus",
           "Terminating onboarding");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::SaveToDisk(OnboardingStages stage) const
{
  Json::Value toSave;
  toSave[kOnboardingStageKey] = OnboardingStagesToString( stage );
  // time of completion is used by How Old Are You
  if (stage == OnboardingStages::Complete) {
    const auto tse = std::chrono::system_clock::now().time_since_epoch();
    toSave[kOnboardingTimeKey] = std::chrono::duration_cast<std::chrono::seconds>(tse).count(); // write out as seconds since the epoch
  }
  const std::string filename = _iConfig.saveFolder + kOnboardingFilename;
  Util::FileUtils::WriteFile( filename, toSave.toStyledString() );
  LOG_INFO("BehaviorOnboardingCoordinator.SaveToDisk",
           "Marking onboarding as: %s", EnumToString(stage));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::FixStimAtMax()
{
  if( GetBEI().HasMoodManager() && !_dVars.isStimMaxed ) {
    auto& moodManager = GetBEI().GetMoodManager();
    moodManager.TriggerEmotionEvent("OnboardingStarted");
    moodManager.SetEmotionFixed( EmotionType::Stimulated, true );
    _dVars.isStimMaxed = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::UnFixStim()
{
  if( GetBEI().HasMoodManager() && _dVars.isStimMaxed ) {
    auto& moodManager = GetBEI().GetMoodManager();
    // since this behavior is always at the base of the stack (todo: unit test this), just set it fixed==false
    moodManager.SetEmotionFixed( EmotionType::Stimulated, false );
    _dVars.isStimMaxed = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorOnboardingCoordinator::ShouldExitDueToTimeout()
{
  float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  bool shouldTimeOut = false;
  std::string reason;

  if( ( _dVars.globalExitTime_s > 0.0f ) && ( _dVars.globalExitTime_s < currTime_s ) ){
    auto timeoutType = _dVars.onboardingStarted ? "DELAYED_COMPLETION" : "DELAYED_START";
    LOG_INFO( "BehaviorOnboardingCoordinator.TimeoutExpired",
              "Onboarding aborted due to %s timeout.",
              timeoutType );
    reason = timeoutType;
    shouldTimeOut = true;
  }
  else if( ( _dVars.phaseExitTime_s > 0.0f ) && ( _dVars.phaseExitTime_s < currTime_s ) ){
    std::string timeoutType(EnumToString(_dVars.currentPhase));
    std::transform(timeoutType.begin(), timeoutType.end(), timeoutType.begin(), ::toupper);
    timeoutType += "_PHASE_TIMEOUT";
    LOG_INFO( "BehaviorOnboardingCoordinator.TimeoutExpired",
              "Onboarding aborted due to expired %s.",
              timeoutType.c_str() );
    reason = timeoutType;
    shouldTimeOut = true;
  }
  else if( ( _dVars.appDisconnectExitTime_s > 0.0f ) && ( _dVars.appDisconnectExitTime_s < currTime_s ) ){
    LOG_INFO( "BehaviorOnboardingCoordinator.TimeoutExpired",
              "Onboarding aborted due to expired AppDisconnected timeout while waiting to reconnect.");
    reason = "APP_RECONNECT_TIMEOUT";
    shouldTimeOut = true;
  }

  if(shouldTimeOut){
    DASMSG(onboarding_timeout, "onboarding.timeout", "Onboarding timed out and why");
    DASMSG_SET(s1, reason, "The reason onboarding timed out")
    DASMSG_SEND();
  }

  return shouldTimeOut;
}

} // namespace Vector
} // namespace Anki
