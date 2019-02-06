/**
 * File: onboardingMessageHandler.cpp
 * 
 * Author: Sam Russell
 * Date:   12/8/2018
 * 
 * Description: Receives and responds to onboarding related App messages appropriately based on onboarding state
 * 
 * Copyright: Anki, Inc. 2018
 * 
 **/

#include "engine/aiComponent/behaviorComponent/onboardingMessageHandler.h"

#include "clad/types/onboardingPhaseState.h"
#include "engine/aiComponent/behaviorComponent/behaviorsBootLoader.h"
#include "engine/externalInterface/cladProtoTypeTranslator.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/robot.h"

#include "proto/external_interface/shared.pb.h"

#include "util/console/consoleInterface.h"

namespace Anki{
namespace Vector{

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
OnboardingMessageHandler::OnboardingMessageHandler()
: IDependencyManagedComponent(this, BCComponentID::OnboardingMessageHandler)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OnboardingMessageHandler::AdditionalInitAccessibleComponents(BCCompIDSet& components) const
{
  components.insert(BCComponentID::BehaviorsBootLoader);
  components.insert(BCComponentID::BehaviorContainer);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OnboardingMessageHandler::InitDependent( Robot* robot, const BCCompMap& dependentComps )
{
  _behaviorsBootLoader = &dependentComps.GetComponent<BehaviorsBootLoader>();
  _gatewayInterface = robot->GetGatewayInterface();
  _robot = robot;

  if(nullptr != _gatewayInterface){
    // Locally handled messages
    _eventHandles.push_back(_gatewayInterface->Subscribe(AppToEngineTag::kOnboardingStateRequest,
      std::bind(&OnboardingMessageHandler::HandleOnboardingStateRequest, this, std::placeholders::_1)));

    _eventHandles.push_back(_gatewayInterface->Subscribe(AppToEngineTag::kOnboardingCompleteRequest,
      std::bind(&OnboardingMessageHandler::HandleOnboardingCompleteRequest, this, std::placeholders::_1)));
  }

  // Onboarding dev console functionality
  if( ANKI_DEV_CHEATS ) {

    auto restartOnboarding = [this](ConsoleFunctionContextRef context) {
      external_interface::GatewayWrapper wrapper;
      wrapper.set_allocated_onboarding_restart(new external_interface::OnboardingRestart);
      if(nullptr != _gatewayInterface){
        _gatewayInterface->Broadcast(wrapper);
      }
    };
    _consoleFuncs.emplace_front( "Restart Onboarding",
                                 std::move(restartOnboarding),
                                 "OnboardingCoordinator", "" );

    auto simulateDisconnectFunc = [this](ConsoleFunctionContextRef context){
      external_interface::GatewayWrapper wrapper;
      wrapper.set_allocated_app_disconnected(new external_interface::AppDisconnected);
      if(nullptr != _gatewayInterface){
        _gatewayInterface->Broadcast(wrapper);
      }
    };
    _consoleFuncs.emplace_front("Simulate App Disconnect",
                                std::move(simulateDisconnectFunc),
                                "OnboardingCoordinator",
                                "");

    auto terminateIncompleteFunc = [this](ConsoleFunctionContextRef context){
      external_interface::GatewayWrapper wrapper;
      wrapper.set_allocated_onboarding_skip_onboarding(new external_interface::OnboardingSkipOnboarding);
      if(nullptr != _gatewayInterface){
        _gatewayInterface->Broadcast(wrapper);
      }
    };
    _consoleFuncs.emplace_front("Exit Onboarding - DO NOT Mark Complete",
                                std::move(terminateIncompleteFunc),
                                "OnboardingCoordinator",
                                "");

    auto terminateCompleteFunc = [this](ConsoleFunctionContextRef context){
      external_interface::GatewayWrapper wrapper;
      wrapper.set_allocated_onboarding_mark_complete_and_exit(new external_interface::OnboardingMarkCompleteAndExit);
      if(nullptr != _gatewayInterface){
        _gatewayInterface->Broadcast(wrapper);
      }
    };
    _consoleFuncs.emplace_front("Exit Onboarding - Mark Complete",
                                std::move(terminateCompleteFunc),
                                "OnboardingCoordinator",
                                "");

    auto hailMaryWakeUpFunc = [this](ConsoleFunctionContextRef context){
      external_interface::GatewayWrapper wrapper;
      wrapper.set_allocated_onboarding_wake_up_request(new external_interface::OnboardingWakeUpRequest);
      if(nullptr != _gatewayInterface){
        _gatewayInterface->Broadcast(wrapper);
      }
    };
    _consoleFuncs.emplace_front("Hail Mary: Send Wake Up Request",
                                std::move(hailMaryWakeUpFunc),
                                "OnboardingCoordinator",
                                "");

    auto hailMaryWakeUpStartedFunc = [this](ConsoleFunctionContextRef context){
      external_interface::GatewayWrapper wrapper;
      wrapper.set_allocated_onboarding_wake_up_started_request(new external_interface::OnboardingWakeUpStartedRequest);
      if(nullptr != _gatewayInterface){
        _gatewayInterface->Broadcast(wrapper);
      }
    };
    _consoleFuncs.emplace_front("Hail Mary: Send Wake Up Started Request",
                                std::move(hailMaryWakeUpStartedFunc),
                                "OnboardingCoordinator",
                                "");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OnboardingMessageHandler::ShowUrlFace(bool show)
{
  _robot->SendRobotMessage<RobotInterface::ShowUrlFace>(show);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OnboardingMessageHandler::RequestBleSessions() {
  _robot->Broadcast(ExternalInterface::MessageEngineToGame(SwitchboardInterface::HasBleKeysRequest()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OnboardingMessageHandler::UpdateDependent(const BCCompMap& dependentComps)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OnboardingMessageHandler::SubscribeToAppTags(std::set<AppToEngineTag>&& tags,
                                                  std::function<void(const AppToEngineEvent&)> messageHandler)
{
  if(nullptr != _gatewayInterface){
    for(auto& tag : tags){
      _eventHandles.push_back(_gatewayInterface->Subscribe(tag, messageHandler));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Messages handled only locally by OnboardingMessageHandler
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OnboardingMessageHandler::HandleOnboardingStateRequest(const AppToEngineEvent& event)
{
  if(nullptr != _gatewayInterface){
    if(nullptr != _behaviorsBootLoader){
      OnboardingStages stage = _behaviorsBootLoader->GetOnboardingStage();
      auto* onboardingState = new external_interface::OnboardingState{ CladProtoTypeTranslator::ToProtoEnum(stage) };
      _gatewayInterface->Broadcast(ExternalMessageRouter::WrapResponse(onboardingState));
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void OnboardingMessageHandler::HandleOnboardingCompleteRequest(const AppToEngineEvent& event)
{
  if(nullptr != _gatewayInterface){
    if(nullptr != _behaviorsBootLoader){
      OnboardingStages stage = _behaviorsBootLoader->GetOnboardingStage();
      const bool completed = (stage == OnboardingStages::Complete) || (stage == OnboardingStages::DevDoNothing);
      auto* onboardingComplete = new external_interface::OnboardingCompleteResponse{ completed };
      _gatewayInterface->Broadcast( ExternalMessageRouter::WrapResponse(onboardingComplete) );
    }
  }
}

} //namespace Vector
} //namespace Anki
