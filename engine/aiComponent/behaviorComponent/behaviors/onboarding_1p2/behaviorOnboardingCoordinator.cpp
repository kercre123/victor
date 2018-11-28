/**
 * File: BehaviorOnboardingCoordinator.h
 *
 * Author: Sam Russell
 * Created: 2018-10-26
 *
 * Description: Modular App driven onboarding extension of Hail Mary(1p0)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/behaviors/onboarding_1p2/behaviorOnboardingCoordinator.h"

#include "clad/types/onboardingPhase.h"
#include "clad/types/onboardingPhaseState.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding_1p0/behaviorOnboarding1p0.h"
#include "engine/aiComponent/behaviorComponent/behaviors/onboarding_1p2/phases/iOnboardingPhaseWithProgress.h"
#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/beiRobotInfo.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/cozmoContext.h"
#include "engine/externalInterface/cladProtoTypeTranslator.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/moodSystem/moodManager.h"

#include "util/console/consoleInterface.h"

#define LOG_CHANNEL "Behaviors"

namespace Anki {
namespace Vector {

const std::string BehaviorOnboardingCoordinator::kOnboardingFolder   = "onboarding";
const std::string BehaviorOnboardingCoordinator::kOnboardingFilename = "onboardingState.json";
const std::string BehaviorOnboardingCoordinator::kOnboardingStageKey = "onboardingStage";

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
              "A valid 'Default' phase must be specified in Onboarding1p2.json!");
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
, lastPhase(OnboardingPhase::InvalidPhase)
, nextTimeSendBattInfo_s(0.0f)
, globalExitTime_s(0.0f)
, phaseExitTime_s(0.0f)
, appDisconnectExitTime_s(0.0f)
, onboardingStarted(false)
, waitingForAppReconnect(false)
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
  SubscribeToAppTags({
    AppToEngineTag::kOnboardingWakeUpRequest,
    AppToEngineTag::kOnboardingWakeUpStartedRequest,
    AppToEngineTag::kOnboardingSetPhaseRequest,
    AppToEngineTag::kOnboardingPhaseProgressRequest,
    AppToEngineTag::kOnboardingChargeInfoRequest,
    AppToEngineTag::kOnboardingSkipOnboarding,
    AppToEngineTag::kOnboardingMarkCompleteAndExit,
    AppToEngineTag::kAppDisconnected
  });
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
  modifiers.visionModesForActiveScope->insert({ VisionMode::DetectingFaces, EVisionUpdateFrequency::Med });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.behaviorPowerOff.get());
  delegates.insert(_iConfig.behaviorOnboarding1p0.get());
  for(auto& pair : _iConfig.OnboardingPhaseMap){
    delegates.insert( pair.second.behavior.get() );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();

  BC.FindBehaviorByIDAndDowncast( BEHAVIOR_ID(Onboarding),
                                  BEHAVIOR_CLASS(Onboarding1p0),
                                  _iConfig.behaviorOnboarding1p0);
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
    for(auto& phaseDataPair : _iConfig.OnboardingPhaseMap){
      OnboardingPhase phase = phaseDataPair.first;
      auto setPhaseFunc = [this, phase](ConsoleFunctionContextRef context){
        if(!IsActivated()){
          return;
        }
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

    auto simulateDisconnectFunc = [this](ConsoleFunctionContextRef context){
      if(!IsActivated()){
        return;
      }
      external_interface::GatewayWrapper wrapper;
      wrapper.set_allocated_app_disconnected(new external_interface::AppDisconnected);
      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      gi->Broadcast(wrapper);
    };
    _iConfig.consoleFuncs.emplace_front("Simulate App Disconnect",
                                        std::move(simulateDisconnectFunc),
                                        "OnboardingCoordinator",
                                        "");

    auto terminateIncompleteFunc = [this](ConsoleFunctionContextRef context){
      if(!IsActivated()){
        return;
      }
      external_interface::GatewayWrapper wrapper;
      wrapper.set_allocated_onboarding_skip_onboarding(new external_interface::OnboardingSkipOnboarding);
      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      gi->Broadcast(wrapper);
    };
    _iConfig.consoleFuncs.emplace_front("Terminate - Skip",
                                        std::move(terminateIncompleteFunc),
                                        "OnboardingCoordinator",
                                        "");

    auto terminateCompleteFunc = [this](ConsoleFunctionContextRef context){
      if(!IsActivated()){
        return;
      }
      external_interface::GatewayWrapper wrapper;
      wrapper.set_allocated_onboarding_mark_complete_and_exit(new external_interface::OnboardingMarkCompleteAndExit);
      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      gi->Broadcast(wrapper);
    };
    _iConfig.consoleFuncs.emplace_front("Terminate - Mark Complete",
                                        std::move(terminateCompleteFunc),
                                        "OnboardingCoordinator",
                                        "");

    auto terminateDevDoNothingFunc = [this](ConsoleFunctionContextRef context){
      if(!IsActivated()){
        return;
      }
      TerminateOnboarding();
      SaveToDisk(OnboardingStages::DevDoNothing);
    };
    _iConfig.consoleFuncs.emplace_front("Terminate - DevDoNothing",
                                        std::move(terminateDevDoNothingFunc),
                                        "OnboardingCoordinator",
                                        "");

    auto hailMaryWakeUpFunc = [this](ConsoleFunctionContextRef context){
      if(!IsActivated()){
        return;
      }
      external_interface::GatewayWrapper wrapper;
      wrapper.set_allocated_onboarding_wake_up_request(new external_interface::OnboardingWakeUpRequest);
      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      gi->Broadcast(wrapper);
    };
    _iConfig.consoleFuncs.emplace_front("Hail Mary: Send Wake Up Request",
                                        std::move(hailMaryWakeUpFunc),
                                        "OnboardingCoordinator",
                                        "");

    auto hailMaryWakeUpStartedFunc = [this](ConsoleFunctionContextRef context){
      if(!IsActivated()){
        return;
      }
      external_interface::GatewayWrapper wrapper;
      wrapper.set_allocated_onboarding_wake_up_started_request(new external_interface::OnboardingWakeUpStartedRequest);
      auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
      gi->Broadcast(wrapper);
    };
    _iConfig.consoleFuncs.emplace_front("Hail Mary: Send Wake Up Started Request",
                                        std::move(hailMaryWakeUpStartedFunc),
                                        "OnboardingCoordinator",
                                        "");
  }// Dev console utilities
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::OnBehaviorActivated()
{
  // reset dynamic variables
  _dVars = DynamicVariables();

  float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _dVars.globalExitTime_s = (_iConfig.startTimeout_s > 0.0f) ? (currTime_s + _iConfig.startTimeout_s) : 0.0f;

  // init battery info, but don't msg the app
  const bool sendEvent = false;
  UpdateBatteryInfo(sendEvent);

  FixStimAtMax();

  TransitionToPhase(OnboardingPhase::Default);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::HandleWhileActivated(const AppToEngineEvent& event)
{
  // Disable message handling if we've delegated to the Hail Mary behavior for back compat. The delegate
  // will handle App messaging as appropriate
  if( _iConfig.behaviorOnboarding1p0->IsActivated() ){
    return;
  }

  switch( event.GetData().GetTag() ){
    // kOnboardingWakeUpRequest and kOnboardingWakeUpStartedRequest are maintained for backward compatiblity
    // with older app versions which use "hail mary" onboarding sequence
    case AppToEngineTag::kOnboardingWakeUpRequest:
    case AppToEngineTag::kOnboardingWakeUpStartedRequest:
    {
      if( _iConfig.behaviorOnboarding1p0->WantsToBeActivated() ){
        CancelDelegates(false);
        DelegateIfInControl(_iConfig.behaviorOnboarding1p0.get());
        _iConfig.behaviorOnboarding1p0->FwdEventToHandleWhileActivated(event);
        // Disable timeouts, Hail Mary does its own thing here
        _dVars.globalExitTime_s = 0.0f;
        _dVars.phaseExitTime_s = 0.0f;
        _dVars.appDisconnectExitTime_s = 0.0f;
      }
      else{
        TerminateOnboarding();
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
      TerminateOnboarding();
      // Skipping is treated just like it never started, and should overwrite previous timeouts
      SaveToDisk(OnboardingStages::NotStarted);
      break;
    }
    case AppToEngineTag::kOnboardingMarkCompleteAndExit:
    {
      TerminateOnboarding();
      SaveToDisk(OnboardingStages::Complete);
      break;
    }
    case AppToEngineTag::kAppDisconnected:
    {
      const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
      const bool appDisconnectTimeOutSet = _iConfig.appDisconnectTimeout_s > 0.0f;
      _dVars.appDisconnectExitTime_s = appDisconnectTimeOutSet ? (currTime_s + _iConfig.appDisconnectTimeout_s) : 0.0f;
      // This particular message should NOT reset appDisconnect params; return early
      return;
    }
    default:
    {
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
  if( !IsActivated() || _dVars.poweringOff ) {
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
    SaveToDisk(OnboardingStages::TimedOut);
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
void BehaviorOnboardingCoordinator::TransitionToPhase(const OnboardingPhase& phase, bool appCommandedTransition)
{
  // App Authoritative. Abandon anything we're currently doing without callbacks
  CancelDelegates(false);

  const auto currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  if( !_dVars.onboardingStarted ){
    _dVars.globalExitTime_s = (_iConfig.completionTimeout_s > 0.0f) ? (currTime_s + _iConfig.completionTimeout_s) : 0.0f;
    _dVars.onboardingStarted = true;
  }

  auto iter = _iConfig.OnboardingPhaseMap.find(phase); 
  if( _iConfig.OnboardingPhaseMap.end() != iter ){
    ICozmoBehaviorPtr behavior = iter->second.behavior;
    if( behavior->WantsToBeActivated() ){

      DelegateIfInControl(behavior.get(),
        [this, phase](){
          OnPhaseComplete(phase);
        });

      const bool phaseHasTimeout = iter->second.timeout_s > 0.0f;
      _dVars.phaseExitTime_s = phaseHasTimeout ? (currTime_s + iter->second.timeout_s) : 0.0f;
      _dVars.lastPhase = iter->first;
      _dVars.shouldCheckPowerOff = iter->second.allowPowerOff;

      if( appCommandedTransition ){
        _dVars.lastSetPhaseState = OnboardingPhaseState::PhaseInProgress;
      }
    }
    else {
      LOG_ERROR("BehaviorOnboardingCooordinator.TransitionToPhase.PhaseBehaviorFailure",
                "Phase %s behavior did not want to be activated. Setting Default phase if available",
                EnumToString(phase));
      if( OnboardingPhase::Default != phase ){
        TransitionToPhase(OnboardingPhase::Default);
      }
      else{
        // If we can't get to the default phase, exit onboarding
        TerminateOnboarding();
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
      // If the "default" phase is invalid, exit onboarding
      TerminateOnboarding();
    }

    if( appCommandedTransition ){
      _dVars.lastSetPhaseState = OnboardingPhaseState::PhaseInvalid;
    }
  }

  // Send the SetPhaseResponse back to the App
  if( appCommandedTransition ){
    auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
    if(nullptr != gi){
      auto* msg = new external_interface::OnboardingSetPhaseResponse;
      msg->set_phase(CladProtoTypeTranslator::ToProtoEnum(phase));
      msg->set_phase_state(CladProtoTypeTranslator::ToProtoEnum(_dVars.lastSetPhaseState));
      gi->Broadcast( ExternalMessageRouter::WrapResponse(msg) );
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::OnPhaseComplete(const OnboardingPhase& phase)
{
  LOG_INFO("BehaviorOnboardingCoordinator.OnPhaseComplete",
           "Completed onboarding phase %s. Idling.",
           EnumToString(phase));
  const auto currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  const bool hasIdleTimeout = _iConfig.idleTimeout_s > 0.0f;
  _dVars.phaseExitTime_s = hasIdleTimeout ? (currTime_s + _iConfig.idleTimeout_s) : 0.0f;
  _dVars.shouldCheckPowerOff = _iConfig.allowPowerOffFromIdle;
  _dVars.lastPhase = OnboardingPhase::InvalidPhase;
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
    if( OnboardingPhase::InvalidPhase != _dVars.lastPhase ){
      TransitionToPhase(_dVars.lastPhase);
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
void BehaviorOnboardingCoordinator::SendChargeInfoResponseToApp()
{
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if( nullptr != gi ) {
    auto* msg = new external_interface::OnboardingChargeInfoResponse;
    msg->set_on_charger( _dVars.batteryInfo.onCharger );
    msg->set_needs_to_charge( _dVars.batteryInfo.needsToCharge );
    msg->set_required_charge_time( _dVars.batteryInfo.chargerTime_s );
    gi->Broadcast( ExternalMessageRouter::WrapResponse(msg) );
    const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    _dVars.nextTimeSendBattInfo_s = (kBattInfoUpdateTime_s > 0.0f) ? (currTime_s + kBattInfoUpdateTime_s) : 0.0f;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorOnboardingCoordinator::SendPhaseProgressResponseToApp()
{
  auto* gi = GetBEI().GetRobotInfo().GetGatewayInterface();
  if(nullptr != gi){
    auto* msg = new external_interface::OnboardingPhaseProgressResponse;
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
void BehaviorOnboardingCoordinator::TerminateOnboarding()
{
  CancelDelegates(false);
  _dVars.waitingForTermination = true;

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

  if( ( _dVars.globalExitTime_s > 0.0f ) && ( _dVars.globalExitTime_s < currTime_s ) ){
    LOG_INFO( "BehaviorOnboardingCoordinator.TimeoutExpired",
              "Onboarding aborted due to expired %s timeout. Onboarding will repeat on reboot",
              _dVars.onboardingStarted ? "Completion" : "Start" );
    // TODO:(STR) reimplement onboarding DAS messaging
    return true;
  }

  if( ( _dVars.phaseExitTime_s > 0.0f ) && ( _dVars.phaseExitTime_s < currTime_s ) ){
    LOG_INFO( "BehaviorOnboardingCoordinator.TimeoutExpired",
              "Onboarding aborted due to expired %s phase timeout. Onboarding will repeat on reboot",
              EnumToString(_dVars.lastPhase));
    // TODO:(STR) reimplement onboarding DAS messaging
    return true;
  }

  if( ( _dVars.appDisconnectExitTime_s > 0.0f ) && ( _dVars.appDisconnectExitTime_s < currTime_s ) ){
    LOG_INFO( "BehaviorOnboardingCoordinator.TimeoutExpired",
              "Onboarding aborted due to expired AppDisconnected timeout. Onboarding will repeat on reboot");
    // TODO:(STR) reimplement onboarding DAS messaging
    return true;
  }

  return false;
}

} // namespace Vector
} // namespace Anki
