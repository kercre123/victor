/**
 * File: sdkComponent
 *
 * Author: Michelle Sintov
 * Created: 05/25/2018
 *
 * Description: Component that serves as a mediator between external SDK requests and any instances of SDK behaviors,
 * such as SDK0.
 * 
 * The sdkComponent does the following, in this order:
 *     - The sdkComponent will receive a message from the external SDK, requesting that the SDK behavior be activated,
 *       so that the SDK can make the robot do something (which could be a behavior, action or low level motor control)
 *     - The SDK behavior will check the sdkComponent regularly to see if the SDK wants control.
 *       If so, the SDK behavior will say it wants to be activated.
 *     - When the SDK behavior is activated, it will inform sdkComponent that the SDK has control.
 *     - The sdkComponent sends a message to the external SDK to inform that it has control.
 *     - Any other state changes from the SDK behavior (e.g., lost control, etc.) are also communicated
 *       through the sdkComponent.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "coretech/common/engine/robotTimeStamp.h"

#include "engine/ankiEventUtil.h"
#include "engine/components/sdkComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotEventHandler.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"

#include "util/logging/logging.h"

#define LOG_CHANNEL "SdkComponent"

namespace Anki {
namespace Vector {

SDKComponent::SDKComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::SDK)
{
}


SDKComponent::~SDKComponent()
{
}


void SDKComponent::GetInitDependencies( RobotCompIDSet& dependencies ) const
{
  // TODO
}


void SDKComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  // TODO It's preferred, where possible, to use dependentComps rather than caching robot
  // directly (makes it much easier to write unit tests)
  _robot = robot;
  auto* gi = robot->GetGatewayInterface();
  if (gi != nullptr)
  {
    auto callback = std::bind(&SDKComponent::HandleMessage, this, std::placeholders::_1);

    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kControlRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kControlRelease, callback));

    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kGoToPoseRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kDockWithCubeRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kDriveStraightRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kTurnInPlaceRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kSetLiftHeightRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kSetHeadAngleRequest, callback));

    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kAudioSendModeRequest, callback));
  }
}
// @TODO: JMRivas - Delete this static and replace with a better way to store whether the audio processing mode on the
// robot/animProcess is considered to be under SDK control.
static bool _SDK_WANTS_AUDIO = false;
void SDKComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  // @TODO: JMRivas - Remove all of this code and replace with a subscription to the true audio feed.
  //        Should be similarly formatted into chunks of SAMPLE_COUNTS_PER_SDK_MESSAGE samples, with
  //        audio metadata redundantly submitted with each packet.
  //        (Note: only the final message's metadata will be used)

  static RobotTimeStamp_t _SDK_AUDIO_BROADCAST_TIME = 0;
  static u32 _SDK_SAMPLE_GROUP_ID = 0;

  RobotTimeStamp_t now = _robot->GetLastMsgTimestamp();

  // The robot will send audio data in chunks of 1600 samples, and the animProcess operates
  // at a sampling rate of 16000.  The rate at which the robot generates these chunks (and the
  // minimum latency of the audiostream) is 100ms on the golden path.
  if( _SDK_WANTS_AUDIO && now > _SDK_AUDIO_BROADCAST_TIME + 100 )
  {
    auto* gi = _robot->GetGatewayInterface();
    if (gi == nullptr) return;

    _SDK_AUDIO_BROADCAST_TIME = now;
    // temporary 1 kHz sine wave generator to test over the wire transfer
    std::vector<u8> power_array(external_interface::SAMPLE_COUNTS_PER_SDK_MESSAGE*2);
    std::vector<u8> direction_array(external_interface::MIC_DETECTION_DIRECTIONS, 0);
    for( u32 i = 0; i < external_interface::SAMPLE_COUNTS_PER_SDK_MESSAGE; i++ ) {
      // sin(pi*i/8) will cycle every 16 samples, the robot's sample rate is 16,000
      // arbitrarily using amplitude of '1000' rather than max_int16 because its less painful to listen to.
      //
      // the robot's audio data is in the form of 16 bit signed integers, but the proto
      // stores it as a byte array
      f32 theta = (f32)i * (3.1415f/8.0f);
      s16 powerLevel = (s16)(sin(theta) * 1000.0);
      power_array[i*2+0] = reinterpret_cast<const u8* const>(&powerLevel)[0];
      power_array[i*2+1] = reinterpret_cast<const u8* const>(&powerLevel)[1];
    }

    std::string direction_data;
    direction_data.assign(direction_array.begin(), direction_array.begin() + external_interface::MIC_DETECTION_DIRECTIONS);

    // Break the audio group into chunks small enough to pass through to gateway.
    u32 chunks = external_interface::SAMPLE_COUNTS_PER_SDK_MESSAGE / external_interface::SAMPLE_COUNTS_PER_ENGINE_MESSAGE;
    for( u32 i=0; i<chunks; ++i ) {
      std::string power_data;
      u32 chunkSize = external_interface::SAMPLE_COUNTS_PER_ENGINE_MESSAGE*2;
      power_data.assign(power_array.begin() + (chunkSize * i), power_array.begin() + (chunkSize*(i+1)));

      // Send audio chunk to gateway
      external_interface::AudioChunk* audioChunk = new external_interface::AudioChunk();
      audioChunk->set_signal_power(std::move(power_data));
      audioChunk->set_direction_strengths(std::move(direction_data));
      audioChunk->set_group_id(_SDK_SAMPLE_GROUP_ID);
      audioChunk->set_chunk_id(i);
      audioChunk->set_audio_chunk_count(chunks);
      audioChunk->set_robot_time_stamp((::google::protobuf::uint32)now);
      audioChunk->set_source_direction(12);
      audioChunk->set_source_confidence(0);
      audioChunk->set_noise_floor_power(0);

      external_interface::GatewayWrapper wrapper;

      wrapper.set_allocated_audio_chunk(audioChunk);
      gi->Broadcast(wrapper);
      audioChunk = wrapper.release_audio_chunk();
      Util::SafeDelete(audioChunk);
    }
    ++_SDK_SAMPLE_GROUP_ID;
  }
}

void SDKComponent::OnSendAudioModeRequest(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  // @TODO: JMRivas - Replace this functionality with a call or message to whichever process controls
  //        the robot's audio processing mode, and lock it into an "SDK OWNED" state until receiving
  //        the "AUDIO_OFF" id.
  //
  //        Any internal change in the audioProcessing state should be broadcast in an AudioSendModeChanged
  //        message, whether a direct response to a request or incidental from underlying behavior.

  auto* gi = _robot->GetGatewayInterface();
  if (gi == nullptr) return;

  external_interface::AudioProcessingMode mode = event.GetData().audio_send_mode_request().mode();

  _SDK_WANTS_AUDIO = (mode != external_interface::AUDIO_OFF);

  // Signal audio mode changed to gateway
  external_interface::AudioSendModeChanged* changedEvent = new external_interface::AudioSendModeChanged();
  changedEvent->set_mode(mode);

  gi->Broadcast(ExternalMessageRouter::WrapResponse(changedEvent));
}

void SDKComponent::HandleMessage(const AnkiEvent<external_interface::GatewayWrapper>& event)
{

  switch(event.GetData().GetTag()){
    // Receives a message that external SDK wants an SDK behavior to be activated.
    case external_interface::GatewayWrapperTag::kControlRequest:
      LOG_INFO("SDKComponent.HandleMessageRequest", "SDK requested control");
      _sdkWantsControl = true;

      if (_sdkBehaviorActivated) {
        LOG_INFO("SDKComponent.HandleMessageBehaviorActivated", "SDK already has control");
        // SDK wants control and and the SDK Behavior is already running. Send response that SDK behavior is activated.
        DispatchSDKActivationResult(_sdkBehaviorActivated);
        return;
      }
      break;

    case external_interface::GatewayWrapperTag::kControlRelease:
      LOG_INFO("SDKComponent.HandleMessageRelease", "Releasing SDK control");
      _sdkWantsControl = false;
      break;

    case external_interface::GatewayWrapperTag::kAudioSendModeRequest:
      LOG_INFO("SDKComponent.HandleMessageRelease", "Changing audio mode");
      OnSendAudioModeRequest(event);
      break;

    default:
      _robot->GetRobotEventHandler().HandleMessage(event);
      break;
  }
}

bool SDKComponent::SDKWantsControl()
{
  // TODO What slot does the SDK want to run at? Currently only requesting at one slot, SDK0.
  return _sdkWantsControl;
}

void SDKComponent::SDKBehaviorActivation(bool enabled)
{
  _sdkBehaviorActivated = enabled;
  DispatchSDKActivationResult(_sdkBehaviorActivated);
}

void SDKComponent::DispatchSDKActivationResult(bool enabled) {
  auto* gi = _robot->GetGatewayInterface();
  if (enabled) {
    //TODO: better naming, more readable, and logging
    auto* msg = new external_interface::BehaviorControlResponse(new external_interface::ControlGrantedResponse());
    gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
  }
  else {
    auto* msg = new external_interface::BehaviorControlResponse(new external_interface::ControlLostResponse());
    gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
  }
}

template <typename MessageType>
external_interface::GatewayWrapper ConstructActionResponseMessage(const ActionResult& actionResult)
{
  MessageType* response = new MessageType();

  external_interface::ActionResult* actionStatus = new external_interface::ActionResult();
  actionStatus->set_code((external_interface::ActionResult_ActionResultCode)actionResult);

  response->set_allocated_result(actionStatus);
  return ExternalMessageRouter::WrapResponse(response);
}

void SDKComponent::OnActionCompleted(ExternalInterface::RobotCompletedAction msg)
{
  auto* gi = _robot->GetGatewayInterface();
  if (gi == nullptr) return;

  switch((RobotActionType)msg.actionType)
  {
    case RobotActionType::PLAY_ANIMATION:
    {
      auto* response = new external_interface::PlayAnimationResponse;
      response->set_result(external_interface::BehaviorResults::BEHAVIOR_COMPLETE_STATE);
      gi->Broadcast( ExternalMessageRouter::WrapResponse(response) );
    }
    break;

    default:
    {
      using ProtoActionResponseFactory = std::function<external_interface::GatewayWrapper(const ActionResult&)>;
      std::map<RobotActionType, ProtoActionResponseFactory> actionResponseFactories = {
        { RobotActionType::TURN_IN_PLACE,       ConstructActionResponseMessage<external_interface::TurnInPlaceResponse> },
        { RobotActionType::DRIVE_STRAIGHT,      ConstructActionResponseMessage<external_interface::DriveStraightResponse> },
        { RobotActionType::MOVE_HEAD_TO_ANGLE,  ConstructActionResponseMessage<external_interface::SetHeadAngleResponse> },
        { RobotActionType::MOVE_LIFT_TO_HEIGHT, ConstructActionResponseMessage<external_interface::SetLiftHeightResponse> },
        { RobotActionType::DRIVE_TO_POSE,       ConstructActionResponseMessage<external_interface::GoToPoseResponse> },
        { RobotActionType::ALIGN_WITH_OBJECT,   ConstructActionResponseMessage<external_interface::DockWithCubeResponse> }
      };

      if(actionResponseFactories.count((RobotActionType)msg.actionType) == 0) 
      {
          PRINT_NAMED_WARNING("SDKComponent.OnActionCompleted.NoMatch", "No match for action tag so no response sent: [Tag=%d]", msg.idTag);
      }
      else
      {
        auto& responseFactory = actionResponseFactories.at((RobotActionType)msg.actionType);
        gi->Broadcast( responseFactory(msg.result) );
      }
      return;
    }
  }
}

} // namespace Vector
} // namespace Anki
