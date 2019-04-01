/**
 * File: sdkComponent
 *
 * Author: Michelle Sintov
 * Created: 05/25/2018
 *
 * Description: Component that serves as a mediator between external SDK requests and any instances of SDK behaviors,
 * such as SDKDefault.
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
#include "engine/components/settingsManager.h"
#include "engine/components/animationComponent.h"
#include "engine/components/visionComponent.h"
#include "engine/components/visionScheduleMediator/visionScheduleMediator.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotManager.h"
#include "engine/robotInterface/messageHandler.h"
#include "engine/robotEventHandler.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "proto/external_interface/shared.pb.h"

#include "util/logging/logging.h"

#define LOG_CHANNEL "SdkComponent"

namespace Anki {
namespace Vector {

SDKComponent::SDKComponent()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::SDK)
{
  _sdkControlLevel = external_interface::ControlRequest_Priority_UNKNOWN;
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
    auto callback = std::bind(&SDKComponent::HandleProtoMessage, this, std::placeholders::_1);

    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kControlRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kControlRelease, callback));

    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kGoToPoseRequest,      callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kDockWithCubeRequest,  callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kDriveStraightRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kTurnInPlaceRequest,   callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kSetLiftHeightRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kSetHeadAngleRequest,  callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kTurnTowardsFaceRequest,  callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kGoToObjectRequest,  callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kRollObjectRequest,  callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kPopAWheelieRequest,  callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kPickupObjectRequest,  callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kPlaceObjectOnGroundHereRequest,  callback));

    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kAudioSendModeRequest, callback));

    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kEnableMarkerDetectionRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kEnableFaceDetectionRequest,   callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kEnableMotionDetectionRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kEnableMirrorModeRequest,      callback));
    // _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kCaptureSingleImageRequest,    callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kEnableImageStreamingRequest,  callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kIsImageStreamingEnabledRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kSayTextRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kSetEyeColorRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kListAnimationTriggersRequest, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kExternalAudioStreamPrepare, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kExternalAudioStreamChunk, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kExternalAudioStreamComplete, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kExternalAudioStreamCancel, callback));
    _signalHandles.push_back(gi->Subscribe(external_interface::GatewayWrapperTag::kMasterVolumeRequest, callback));
  }

  auto* context = _robot->GetContext();
  if(context != nullptr && context->GetExternalInterface() != nullptr)
  {
    using namespace ExternalInterface;
    auto helper = MakeAnkiEventUtil(*context->GetExternalInterface(), *this, _signalHandles);

    helper.SubscribeEngineToGame<MessageEngineToGameTag::RobotProcessedImage>();
  }

  // Disable/Unsubscribe from MirrorMode when we enter pairing.
  // This is to prevent SDK requested MirrorMode from drawing over the pairing screens
  auto setConnectionStatusCallback = [this](const GameToEngineEvent& event) {
     const auto& msg = event.GetData().Get_SetConnectionStatus();
     if (msg.status != SwitchboardInterface::ConnectionStatus::NONE &&
         msg.status != SwitchboardInterface::ConnectionStatus::COUNT &&
         msg.status != SwitchboardInterface::ConnectionStatus::END_PAIRING)
     {
       DisableMirrorMode();
     }
   };

  _signalHandles.push_back(_robot->GetExternalInterface()->Subscribe(GameToEngineTag::SetConnectionStatus,
                                                                     setConnectionStatusCallback));

  //subscribe to messages back from audio playback
  auto* messageHandler = robot->GetRobotMessageHandler();
  _signalHandles.push_back(messageHandler->Subscribe(RobotInterface::RobotToEngineTag::audioStreamStatusEvent, [this](const AnkiEvent<RobotInterface::RobotToEngine>& event) {
      auto streamEvent = event.GetData().Get_audioStreamStatusEvent();
      HandleStreamStatusEvent(streamEvent.streamResultID, streamEvent.audioReceived,
                  streamEvent.audioPlayed);
  }));                                                                     
}

void SDKComponent::HandleStreamStatusEvent(SDKAudioStreamingState streamStatusId, int audioFramesReceived, int audioFramesPlayed) {
  auto* gi = _robot->GetGatewayInterface();
  LOG_INFO("SDKComponent::HandleStreamStatusEvent","Received audio playback stream state %u received %u, played %u", 
            (int)streamStatusId, audioFramesReceived, audioFramesPlayed);
  switch (streamStatusId) {
    case SDKAudioStreamingState::Completed:
    {
      //we're done, send message back to Gateway
      auto* msg = new external_interface::ExternalAudioStreamResponse(new external_interface::ExternalAudioStreamPlaybackComplete());
      gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
    }
    break;

    case SDKAudioStreamingState::BufferOverflow:
    {
      //failed due to potential buffer overrun situation (sending audio too fast) send message back to Gateway
      auto* msg = new external_interface::ExternalAudioStreamResponse(new external_interface::ExternalAudioStreamBufferOverrun());
      gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
    }
    break;

    case SDKAudioStreamingState::PrepareFailed:
    case SDKAudioStreamingState::PostFailed:
    case SDKAudioStreamingState::AddAudioFailed:
    {
      //failed due to potential buffer overrun situation (sending audio too fast) send message back to Gateway
      auto* msg = new external_interface::ExternalAudioStreamResponse(new external_interface::ExternalAudioStreamPlaybackFailure());
      gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
    }
    break;

    default:
      break;
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

// Determine whether or not image streaming is enabled on the robot
void SDKComponent::IsImageStreamingEnabledRequest(
  const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  auto* gi = _robot->GetGatewayInterface();
  if (gi == nullptr) {
    return;
  }

  const bool is_enabled = _robot->GetVisionComponent().IsSendingSDKImageChunks();
  external_interface::IsImageStreamingEnabledResponse* isEnabledResponse =
    new external_interface::IsImageStreamingEnabledResponse();
  isEnabledResponse->set_is_image_streaming_enabled(is_enabled);

  gi->Broadcast(ExternalMessageRouter::WrapResponse(isEnabledResponse));
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

//keep ID for multiple exclusive user attempts
void SDKComponent::SetBehaviorLock(uint64_t controlId)
{
  if (_sdkWantsLock && (_sdkLockConnId != controlId)) {
    //grabbing control from another connection
    DispatchBehaviorLockLostResult();
    LOG_INFO("SDKComponent.SetBehaviorLock","Connection_id %llu control reservation LOST", _sdkLockConnId);
  }        
  
  LOG_INFO("SDKComponent.SetBehaviorLock","Connection_id %llu reserving control", controlId);
  _sdkLockConnId = controlId;
  _sdkWantsLock = true;
  DispatchSDKActivationResult(true, controlId);
}

void SDKComponent::HandleProtoMessage(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  using namespace external_interface;
  auto* gi = _robot->GetGatewayInterface();
  const auto id = event.GetData().connection_id();

  switch(event.GetData().GetTag()) {
    // Receives a message that external SDK wants an SDK behavior to be activated.
    case external_interface::GatewayWrapperTag::kControlRequest:
      {
        auto & control_req = event.GetData().control_request(); 
        _sdkControlLevel = control_req.priority();
        LOG_INFO("SDKComponent.HandleProtoMessage", "SDK requested control connection_id %llu", id);
        if (!ANKI_VERIFY(_sdkControlLevel, "SDKComponent::HandleProtoMessage", "Invalid _sdkControlLevel 0 (UNKNOWN)")) {
          return;
        }

        if (event.GetData().control_request().priority() == event.GetData().control_request().RESERVE_CONTROL) {
          //user wants long-running control
          SetBehaviorLock(id);
          return;
        }

        _sdkWantsControl = true;
        _sdkControlConnId =  id;
        
        LOG_INFO("SDKComponent.HandleProtoMessage","SDK requested control priority %s (%u)", 
                  control_req.Priority_Name(control_req.priority()).c_str(), _sdkControlLevel); 

        if (_sdkBehaviorActivated) {
          LOG_INFO("SDKComponent.HandleMessageBehaviorActivated", "SDK already has control");
          // SDK wants control and and the SDK Behavior is already running. Send response that SDK behavior is activated.
          DispatchSDKActivationResult(_sdkBehaviorActivated, _sdkControlConnId);
          return;
        }
      }
      break;

    case external_interface::GatewayWrapperTag::kControlRelease:
      LOG_INFO("SDKComponent.HandleProtoMessage", "Releasing SDK control connection_id %llu", id);
      if (id == _sdkLockConnId) {
        DispatchSDKActivationResult(false, _sdkLockConnId);
        LOG_INFO("SDKComponent.HandleProtoMessage", "ControlRelease Releasing control");
        _sdkWantsLock = false;
        _sdkLockConnId = 0;
      }
      _sdkWantsControl = false;
      _sdkControlConnId = 0;
      break;

    case external_interface::GatewayWrapperTag::kAudioSendModeRequest:
      LOG_INFO("SDKComponent.HandleMessageRelease", "Changing audio mode");
      OnSendAudioModeRequest(event);
      break;

    // All of the vision mode requests are gated behind the sdk behavior being activated
    // to prevent enabling of unnecessary modes that may have performance impact.
    // The modes are automatically removed when the behavior is deactivated.
    // Except for Viz since the SDK still wants to receive images while the robot is
    // doing his normal things
    #define SEND_FORBIDDEN(RESPONSE_TYPE) {                                     \
        ResponseStatus* status = new ResponseStatus(ResponseStatus::FORBIDDEN); \
        auto* msg = new RESPONSE_TYPE();                                        \
        msg->set_allocated_status(status);                                      \
        gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));                \
      }
      
    case external_interface::GatewayWrapperTag::kEnableMarkerDetectionRequest:
      {
        if(_sdkBehaviorActivated)
        {
          const auto& enable = event.GetData().enable_marker_detection_request().enable();
          SubscribeToVisionMode(enable, VisionMode::Markers);
          SubscribeToVisionMode(enable, VisionMode::Markers_FullFrame);
        }
        else
        {
          SEND_FORBIDDEN(EnableMarkerDetectionResponse);
        }
      }
      break;

    case external_interface::GatewayWrapperTag::kEnableFaceDetectionRequest:
      {
        if(_sdkBehaviorActivated)
        {
          const auto& msg = event.GetData().enable_face_detection_request();
          SubscribeToVisionMode(msg.enable(), VisionMode::Faces);
          SubscribeToVisionMode(msg.enable_smile_detection(), VisionMode::Faces_Smile);
          SubscribeToVisionMode(msg.enable_expression_estimation(), VisionMode::Faces_Expression);
          SubscribeToVisionMode(msg.enable_blink_detection(), VisionMode::Faces_Blink);
          SubscribeToVisionMode(msg.enable_gaze_detection(), VisionMode::Faces_Gaze);
        }
        else
        {
          SEND_FORBIDDEN(EnableFaceDetectionResponse);
        }
      }
      break;

    case external_interface::GatewayWrapperTag::kEnableMotionDetectionRequest:
      {
        if(_sdkBehaviorActivated)
        {
          const auto& enable = event.GetData().enable_motion_detection_request().enable();
          SubscribeToVisionMode(enable, VisionMode::Motion);
        }
        else
        {
          SEND_FORBIDDEN(EnableMotionDetectionResponse);
        }
      }
      break;

    case external_interface::GatewayWrapperTag::kEnableMirrorModeRequest:
      {
        if(_sdkBehaviorActivated)
        {
          const auto& enable = event.GetData().enable_mirror_mode_request().enable();
          SubscribeToVisionMode(enable, VisionMode::MirrorMode);
        }
        else
        {
          SEND_FORBIDDEN(EnableMirrorModeResponse);
        }
      }
      break;

    case external_interface::GatewayWrapperTag::kEnableImageStreamingRequest:
      {
        // Allowed to be controlled even when the behavior is not active
        const auto& enable = event.GetData().enable_image_streaming_request().enable();
        SubscribeToVisionMode(enable, VisionMode::Viz);
        _robot->GetVisionComponent().EnableSendingSDKImageChunks(enable);
      }
      break;

    case external_interface::GatewayWrapperTag::kIsImageStreamingEnabledRequest:
      {
        // Determine if the image streaming is enabled on the robot
        IsImageStreamingEnabledRequest(event);
      }
      break;

    case external_interface::GatewayWrapperTag::kSayTextRequest:
      {
        if (_sdkBehaviorActivated) 
        {
          SayText(event);
        }
        else
        {
          SEND_FORBIDDEN(SayTextResponse);          
        }
        
      }
      break;

    case external_interface::GatewayWrapperTag::kSetEyeColorRequest:
      {
        if (_sdkBehaviorActivated) 
        {
          SetEyeColor(event);
        }
        else
        {
          SEND_FORBIDDEN(SetEyeColorResponse);          
        }
        
      }
      break;

    case external_interface::GatewayWrapperTag::kListAnimationTriggersRequest:
      {
        ListAnimationTriggers(event);
      }
      break;

    case external_interface::GatewayWrapperTag::kExternalAudioStreamPrepare:
      {
        HandleAudioStreamPrepareRequest(event);
      }
      break;

    case external_interface::GatewayWrapperTag::kExternalAudioStreamChunk:
      {
        HandleAudioStreamChunkRequest(event);
      }
      break;

    case external_interface::GatewayWrapperTag::kExternalAudioStreamComplete:
      {
        HandleAudioStreamCompleteRequest(event);
      }
      break;

    case external_interface::GatewayWrapperTag::kExternalAudioStreamCancel:
      {
        HandleAudioStreamCancelRequest(event);
      }
      break;

    case external_interface::GatewayWrapperTag::kMasterVolumeRequest:
      {
        SetMasterVolume(event);
      }
      break;

    default:
      _robot->GetRobotEventHandler().HandleMessage(event);
      break;
  }

  #undef SEND_FORBIDDEN
}

template<>
void SDKComponent::HandleMessage(const ExternalInterface::RobotProcessedImage& msg)
{
  // For all of the vision modes we are waiting to change, check if they have appeared or disappeared
  // from the RobotProcessedImage result. If they did what we were expecting then send a Response message
  // for that mode
  // Note: There is one frame of lag between when a VisionMode Request is made, when the detection is sent, and when
  // the Response is sent. The RobotProcessedImage message comes from VisionComponent after any messages
  // for things actually detected in that frame. So if an SDK program is waiting for a Response message,
  // they could miss the detections from the first frame.
  auto waitingIter = _visionModesWaitingToChange.begin();
  while(waitingIter != _visionModesWaitingToChange.end())
  {
    const auto& iter = std::find(msg.visionModes.begin(), msg.visionModes.end(), waitingIter->first);
    const bool modeWasProcessed = (iter != msg.visionModes.end()); 

    if(modeWasProcessed == waitingIter->second)
    {
      using namespace external_interface;
      auto* gi = _robot->GetGatewayInterface();

      bool eraseFromSet = true;
      ResponseStatus* status = new ResponseStatus(ResponseStatus::OK);
      switch(waitingIter->first)
      {
        case VisionMode::Markers:
          {
            auto* msg = new EnableMarkerDetectionResponse();
            msg->set_allocated_status(status);
            gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
          }
          break;
        case VisionMode::Faces:
          {
            auto* msg = new EnableFaceDetectionResponse();
            msg->set_allocated_status(status);
            gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
          }
          break;
        case VisionMode::Motion:
          {
            auto* msg = new EnableMotionDetectionResponse();
            msg->set_allocated_status(status);
            gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
          }
          break;
        case VisionMode::MirrorMode:
          {
            auto* msg = new EnableMirrorModeResponse();
            msg->set_allocated_status(status);
            gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
          }
          break;
        case VisionMode::Viz:
          {
            auto* msg = new EnableImageStreamingResponse();
            msg->set_allocated_status(status);
            gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
          }
          break;
        
        default:
          eraseFromSet = false;
          Util::SafeDelete(status);
          break;
      }

      if(eraseFromSet)
      {
        waitingIter = _visionModesWaitingToChange.erase(waitingIter);
      }
      else
      {
        waitingIter++;
      }
    }
    else
    {
      waitingIter++;
    }
  }
}

int SDKComponent::SDKControlLevel()
{
  ANKI_VERIFY(_sdkWantsControl, "SDKComponent::SDKControlLevel", "_sdkWantsControl not set when accessing _sdkControlLevel");  
    
  return _sdkControlLevel;
}

bool SDKComponent::SDKWantsControl()
{
  return _sdkWantsControl;
}

bool SDKComponent::SDKWantsBehaviorLock()
{
  return _sdkWantsLock;
}

void SDKComponent::SDKBehaviorActivation(bool enabled)
{
  _sdkBehaviorActivated = enabled;
  DispatchSDKActivationResult(_sdkBehaviorActivated, _sdkControlConnId);

  // If sdk behavior is being deactivated...
  if(!_sdkBehaviorActivated)
  {
    // ...unsubscribe from MirrorMode. This is to make sure the robot
    // will display eyes when the sdk no longer has control.
    DisableMirrorMode();

    // Remove all other vision modes except for Viz in order
    // to allow SDK users to still receive images when the SDK does not have
    // control.
    VisionModeSet modes;
    modes.InsertAllModes();
    modes.Remove(VisionMode::Viz);
    _robot->GetVisionScheduleMediator().RemoveVisionModeSubscriptions(this, modes.GetSet());    

    auto* gi = _robot->GetGatewayInterface();
    auto* msg = new external_interface::Event(new external_interface::VisionModesAutoDisabled());
    gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
  }
}

void SDKComponent::HandleAudioStreamPrepareRequest(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  u32 rate, volume;
  RobotInterface::ExternalAudioPrepare msg;

  rate = event.GetData().external_audio_stream_prepare().audio_frame_rate();
  volume = event.GetData().external_audio_stream_prepare().audio_volume();

  msg.audio_volume = (u16) volume;
  msg.audio_rate = (u16) rate;
  LOG_INFO("SDKComponent.HandleAudioStreamPrepareRequest", "SDK Passing prepare audio stream frame rate %d volume %d", rate, volume); 

  // Send request to animation process
  const Result result = _robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
  if (RESULT_OK != result) {
    LOG_ERROR("SDKComponent.HandleAudioStreamPrepareRequest", "SDK Send Prepare Audio Streaming Message to Robot failed");
    return;
  }
}

void SDKComponent::HandleAudioStreamChunkRequest(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  const std::string samplesStr = event.GetData().external_audio_stream_chunk().audio_chunk_samples();
  RobotInterface::ExternalAudioChunk msg;
  msg.audio_chunk_size = (u16)event.GetData().external_audio_stream_chunk().audio_chunk_size_bytes();
  LOG_INFO("SDKComponent.HandleAudioStreamChunkRequest", "SDKPassing audio_chunk_size %d", msg.audio_chunk_size);

  if (!ANKI_VERIFY(msg.audio_chunk_size <= 1024, 
          "SDKComponent.HandleAudioStreamChunkRequest","Invalid audio playback chunk size %u sent", msg.audio_chunk_size)) {
      auto* gi = _robot->GetGatewayInterface();
      auto* msg = new external_interface::ExternalAudioStreamResponse(new external_interface::ExternalAudioStreamPlaybackFailure());
      gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
      return;
  }

  if (!ANKI_VERIFY(samplesStr.length() >= msg.audio_chunk_size, 
          "SDKComponent.HandleAudioStreamChunkRequest","Invalid audio chunk data size %lu, message specified %u", 
          (long)samplesStr.length(), msg.audio_chunk_size)) {
      auto* gi = _robot->GetGatewayInterface();
      auto* msg = new external_interface::ExternalAudioStreamResponse(new external_interface::ExternalAudioStreamPlaybackFailure());
      gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
      return;
  }

  std::memcpy( msg.audio_chunk_data.data(), samplesStr.c_str(), msg.audio_chunk_size ); //TODO:  do we have to copy?

  // Send request to animation process
  const Result result = _robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
  if (RESULT_OK != result) {
    LOG_ERROR("SDKComponent.HandleAudioStreamChunkRequest", "SDK Send Audio Stream Chunk Message to Robot failed");
    return;
  }
}

void SDKComponent::HandleAudioStreamCompleteRequest(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  RobotInterface::ExternalAudioComplete msg;

  // Send request to animation process
  const Result result = _robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
  if (RESULT_OK != result) {
    LOG_ERROR("SDKComponent.HandleAudioStreamCompleteRequest", "SDK Send Audio Stream Complete Message to Robot failed");
    return;
  }
}

void SDKComponent::HandleAudioStreamCancelRequest(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  RobotInterface::ExternalAudioCancel msg;

  // Send request to animation process
  const Result result = _robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
  if (RESULT_OK != result) {
    LOG_ERROR("SDKComponent.HandleAudioStreamCancelRequest", "SDK Send Audio Stream Cancel Message to Robot failed");
    return;
  }
}

void SDKComponent::SetMasterVolume(const AnkiEvent<external_interface::GatewayWrapper>& event)
{
  auto* gi = _robot->GetGatewayInterface();
  external_interface::MasterVolumeRequest request = event.GetData().master_volume_request();

  //set the volume level
  unsigned int desiredVolume = (unsigned int) request.volume_level();
  if (desiredVolume <= 4) {
    desiredVolume += 1; //we don't allow MUTE, so our SDK enum is 1 lower
    auto* settings = _robot->GetComponentPtr<SettingsManager>();
    bool ignoredDueToNoChange;
    const bool success = settings->SetRobotSetting(external_interface::RobotSetting::master_volume,
                                                   desiredVolume,
                                                   true,
                                                   ignoredDueToNoChange);
    if (success || ignoredDueToNoChange) {
      auto* msg = new external_interface::MasterVolumeResponse(new external_interface::ResponseStatus(external_interface::ResponseStatus::OK));
      gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
      return;
    } 
  } 

  //Bad volume or failed to set:  Send result
  LOG_ERROR("SDKComponent::SetMasterVolume","Failed to change volume.");
  auto* msg = new external_interface::MasterVolumeResponse(new external_interface::ResponseStatus(external_interface::ResponseStatus::FORBIDDEN));
  gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
}

void SDKComponent::DispatchSDKActivationResult(bool enabled, uint64_t connectionId) {
  auto* gi = _robot->GetGatewayInterface();
  if (enabled) {
    LOG_INFO("SDKComponent::DispatchSDKActivationResult","Dispatching SDK enabled activation %llu", connectionId);
    //TODO: better naming, more readable, and logging
    auto* msg = new external_interface::BehaviorControlResponse(new external_interface::ControlGrantedResponse());
    external_interface::GatewayWrapper wrapper = ExternalMessageRouter::WrapResponse(msg, connectionId);
    gi->Broadcast(std::move(wrapper));
  }
  else {
    LOG_INFO("SDKComponent::DispatchSDKActivationResult","Dispatching SDK disabled/lost activation %llu", connectionId);
    auto* msg = new external_interface::BehaviorControlResponse(new external_interface::ControlLostResponse());
    external_interface::GatewayWrapper wrapper = ExternalMessageRouter::WrapResponse(msg, connectionId);
    gi->Broadcast(std::move(wrapper));
  }
}

void SDKComponent::DispatchBehaviorLockLostResult() {
  LOG_INFO("SDKComponent::DispatchBehaviorLockLostResult","Dispatching SDK control lost %llu", _sdkLockConnId);
  auto* gi = _robot->GetGatewayInterface();
  auto* msg = new external_interface::BehaviorControlResponse(new external_interface::ReservedControlLostResponse());
  external_interface::GatewayWrapper wrapper = ExternalMessageRouter::WrapResponse(msg, _sdkLockConnId);
  gi->Broadcast(std::move(wrapper));
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

      ActionCompletedUnion completionInfo = (ActionCompletedUnion)msg.completionInfo;
      AnimationCompleted animationCompleted = completionInfo.Get_animationCompleted();
      external_interface::Animation* animation = new external_interface::Animation(animationCompleted.animationName);
      response->set_allocated_animation(animation);

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
        { RobotActionType::ALIGN_WITH_OBJECT,   ConstructActionResponseMessage<external_interface::DockWithCubeResponse> },
        { RobotActionType::TURN_TOWARDS_FACE,  ConstructActionResponseMessage<external_interface::TurnTowardsFaceResponse> },
        { RobotActionType::DRIVE_TO_OBJECT,  ConstructActionResponseMessage<external_interface::GoToObjectResponse> },
        { RobotActionType::ROLL_OBJECT_LOW,  ConstructActionResponseMessage<external_interface::RollObjectResponse> },
        { RobotActionType::POP_A_WHEELIE,  ConstructActionResponseMessage<external_interface::PopAWheelieResponse> },
        { RobotActionType::PICKUP_OBJECT_LOW,  ConstructActionResponseMessage<external_interface::PickupObjectResponse> },
        { RobotActionType::PLACE_OBJECT_LOW,  ConstructActionResponseMessage<external_interface::PlaceObjectOnGroundHereResponse> }
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

bool SDKComponent::SubscribeToVisionMode(bool subscribe, VisionMode mode, bool updateWaitingToChangeSet)
{
  bool res = false;
  if(subscribe)
  {
    _robot->GetVisionScheduleMediator().AddAndUpdateVisionModeSubscriptions(this,
                                                                            {{.mode = mode,
                                                                              .frequency = EVisionUpdateFrequency::High}});
    res = true;
  }
  else
  {
    res = _robot->GetVisionScheduleMediator().RemoveVisionModeSubscriptions(this, {mode});
  }

  if(updateWaitingToChangeSet)
  {
    _visionModesWaitingToChange.insert({mode, subscribe});
  }
  
  return res;
}

void SDKComponent::DisableMirrorMode()
{
  const bool subscriptionUpdated = SubscribeToVisionMode(false, VisionMode::MirrorMode);
  if(subscriptionUpdated)
  {
    auto* gi = _robot->GetGatewayInterface();
    auto* msg = new external_interface::Event(new external_interface::MirrorModeDisabled());
    gi->Broadcast(ExternalMessageRouter::WrapResponse(msg));
  }
}

void SDKComponent::SayText(const AnkiEvent<external_interface::GatewayWrapper>& event) 
{
  external_interface::SayTextRequest request = event.GetData().say_text_request();
  AudioMetaData::SwitchState::Robot_Vic_External_Processing processingStyle;

  if (request.use_vector_voice()) {
    processingStyle = AudioMetaData::SwitchState::Robot_Vic_External_Processing::Default_Processed;
  }
  else {
    processingStyle = AudioMetaData::SwitchState::Robot_Vic_External_Processing::Unprocessed;
  }
  auto ttsCallback = [this](const UtteranceState& state) {
    external_interface::SayTextResponse* response = 
      new external_interface::SayTextResponse{nullptr, (external_interface::SayTextResponse::UtteranceState) state};
    external_interface::GatewayWrapper wrapper;
    wrapper.set_allocated_say_text_response(response);
    this->_robot->GetGatewayInterface()->Broadcast(wrapper);
  };
  _robot->GetTextToSpeechCoordinator().CreateUtterance(request.text(),
                                                       UtteranceTriggerType::Immediate, 
                                                       processingStyle,
                                                       request.duration_scalar(),
                                                       ttsCallback);
}

void SDKComponent::SetEyeColor(const AnkiEvent<external_interface::GatewayWrapper>& event) 
{
  external_interface::SetEyeColorRequest request = event.GetData().set_eye_color_request();

  const float hue = request.hue();
  const float saturation = request.saturation();

  _robot->SendRobotMessage<RobotInterface::SetFaceHue>(hue);
  _robot->SendRobotMessage<RobotInterface::SetFaceSaturation>(saturation);
}

void SDKComponent::ListAnimationTriggers(const AnkiEvent<external_interface::GatewayWrapper>& event) 
{
  auto* gi = _robot->GetGatewayInterface();
  if (gi == nullptr) return;

  external_interface::ListAnimationTriggersRequest request = event.GetData().list_animation_triggers_request();

  // Note that the last item in AnimationTriggerNumEntries is a Count value, so -1 to not include that (pre-existing hackiness).
  for( size_t i=0; i < AnimationTriggerNumEntries - 1; ++i ) {
    const char* result = EnumToString( static_cast<AnimationTrigger>(i) );
    external_interface::ListAnimationTriggersResponse* list_animation_triggers_response = new external_interface::ListAnimationTriggersResponse;
    std::string animTriggerName = result;
    list_animation_triggers_response->add_animation_trigger_names()->set_name(animTriggerName);
    gi->Broadcast( ExternalMessageRouter::WrapResponse(list_animation_triggers_response));
  }

  // Send "EndOfListAnimationTriggersResponses". vic-gateway recipient depends upon it.
  external_interface::ListAnimationTriggersResponse* end_of_list_animation_triggers_response = new external_interface::ListAnimationTriggersResponse;
  end_of_list_animation_triggers_response->add_animation_trigger_names()->set_name("EndOfListAnimationsResponses");
  gi->Broadcast( ExternalMessageRouter::WrapResponse(end_of_list_animation_triggers_response));
}

} // namespace Vector
} // namespace Anki
