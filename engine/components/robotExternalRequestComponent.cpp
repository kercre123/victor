/**
 * File: robotExternalRequestComponent.cpp
 *
 * Author: Rakesh Ravi Shankar
 * Created: 7/16/18
 *
 * Description: Component to handle external protobuf message requests
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/buildVersion.h"
#include "engine/components/robotExternalRequestComponent.h"
#include "engine/components/battery/batteryComponent.h"
#include "engine/robotManager.h"

#include "osState/osState.h"
#include "proto/external_interface/shared.pb.h"
#include "util/transport/connectionStats.h"

namespace Anki {
namespace Vector {

  RobotExternalRequestComponent::RobotExternalRequestComponent()
    : IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::RobotExternalRequestComponent)
  {
  }

  void RobotExternalRequestComponent::Init(CozmoContext* context)
  {
    _context = context;
  }

  void RobotExternalRequestComponent::GetVersionState(const AnkiEvent<external_interface::GatewayWrapper>& event) {
    external_interface::VersionStateResponse* response = new external_interface::VersionStateResponse{nullptr, OSState::getInstance()->GetOSBuildVersion(),
                                                                                                      kBuildVersion};
    external_interface::GatewayWrapper wrapper;
    wrapper.set_allocated_version_state_response(response);

    Robot* robot = _context->GetRobotManager()->GetRobot();
    robot->GetGatewayInterface()->Broadcast(wrapper);  
  }

  void RobotExternalRequestComponent::GetNetworkState(const AnkiEvent<external_interface::GatewayWrapper>& event) {
    external_interface::NetworkStats* networkStats = new external_interface::NetworkStats{Util::gNetStat1NumConnections,
                                                                                          Util::gNetStat2LatencyAvg,
                                                                                          Util::gNetStat3LatencySD,
                                                                                          Util::gNetStat4LatencyMin,
                                                                                          Util::gNetStat5LatencyMax,
                                                                                          Util::gNetStat6PingArrivedPC,
                                                                                          Util::gNetStat7ExtQueuedAvg_ms,
                                                                                          Util::gNetStat8ExtQueuedMin_ms,
                                                                                          Util::gNetStat9ExtQueuedMax_ms,
                                                                                          Util::gNetStatAQueuedAvg_ms,
                                                                                          Util::gNetStatBQueuedMin_ms,
                                                                                          Util::gNetStatCQueuedMax_ms};
    external_interface::NetworkStateResponse* response = new external_interface::NetworkStateResponse{nullptr, networkStats};
    external_interface::GatewayWrapper wrapper;
    wrapper.set_allocated_network_state_response(response);
      
    Robot* robot = _context->GetRobotManager()->GetRobot();
    robot->GetGatewayInterface()->Broadcast(wrapper);
  }

  void RobotExternalRequestComponent::GetBatteryState(const AnkiEvent<external_interface::GatewayWrapper>& event) {
    Robot* robot = _context->GetRobotManager()->GetRobot();
    auto& batteryComponent = robot->GetBatteryComponent();
    external_interface::GatewayWrapper wrapper = batteryComponent.GetBatteryState(event.GetData().battery_state_request());
    robot->GetGatewayInterface()->Broadcast(wrapper);
  }

  void RobotExternalRequestComponent::SayText(const AnkiEvent<external_interface::GatewayWrapper>& event) {
    Robot* robot = _context->GetRobotManager()->GetRobot();
    external_interface::SayTextRequest request = event.GetData().say_text_request();
    AudioMetaData::SwitchState::Robot_Vic_External_Processing processingStyle;
    uint8_t utteranceID;
    UtteranceState utteranceState;

    if (request.use_vector_voice()) {
      processingStyle = AudioMetaData::SwitchState::Robot_Vic_External_Processing::Default_Processed;
    }
    else {
      processingStyle = AudioMetaData::SwitchState::Robot_Vic_External_Processing::Unprocessed;
    }
    
    utteranceID = robot->GetTextToSpeechCoordinator().CreateUtterance(request.text(), 
                                                                      UtteranceTriggerType::Immediate, 
                                                                      processingStyle,
                                                                      request.duration_scalar());
    
    utteranceState = robot->GetTextToSpeechCoordinator().GetUtteranceState(utteranceID);
    external_interface::SayTextResponse* response = new external_interface::SayTextResponse{nullptr,
                                                                                            (external_interface::SayTextResponse::UtteranceState)utteranceState};
    external_interface::GatewayWrapper wrapper;
    wrapper.set_allocated_say_text_response(response);
    robot->GetGatewayInterface()->Broadcast(wrapper);
  }

  void RobotExternalRequestComponent::ImageRequest(const AnkiEvent<external_interface::GatewayWrapper>& imageRequest) {
    Robot* robot = _context->GetRobotManager()->GetRobot();
    external_interface::ImageRequest request = imageRequest.GetData().image_request();
    ImageSendMode sendMode = ImageSendMode(request.mode());
    if (sendMode == ImageSendMode::Off) {
      robot->SetSDKRequestingImage(false);
    }
    else {
      robot->SetSDKRequestingImage(true);
    }
    robot->SetImageSendMode(sendMode);
  }

} // Cozmo namespace
} // Anki namespace
