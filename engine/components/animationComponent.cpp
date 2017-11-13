/**
 * File: animationComponent.cpp
 *
 * Author: Kevin Yoon
 * Created: 2017-08-01
 *
 * Description: Control interface for animation process to manage execution of 
 *              canned and idle animations
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/ankiEventUtil.h"
#include "engine/animations/animationGroup/animationGroupContainer.h"
#include "engine/components/animationComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotManager.h"
#include "engine/robotInterface/messageHandler.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {
 
namespace {
  const u32 kMaxNumAvailableAnimsToReportPerTic = 50;
}
  
  
AnimationComponent::AnimationComponent(Robot& robot, const CozmoContext* context)
: _isInitialized(false)
, _tagCtr(0)
, _robot(robot)
, _animationGroups(context->GetRobotManager()->GetAnimationGroups())
, _isDolingAnims(false)
, _nextAnimToDole("")
, _currPlayingAnim("")
, _disabledTracks(0)
{
  if (context) {
    // Setup game message handlers
    IExternalInterface *extInterface = context->GetExternalInterface();
    if (extInterface != nullptr) {
      
      auto helper = MakeAnkiEventUtil(*extInterface, *this, GetSignalHandles());
  
      using namespace ExternalInterface;
      helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestAvailableAnimations>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::DisplayProceduralFace>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SetFaceHue>();
    }
  }
  
  // Setup robot message handlers
  RobotInterface::MessageHandler *messageHandler = robot.GetContext()->GetRobotManager()->GetMsgHandler();
  RobotID_t robotId = robot.GetID();

  // Subscribe to RobotToEngine messages
  using localHandlerType = void(AnimationComponent::*)(const AnkiEvent<RobotInterface::RobotToEngine>&);
  // Create a helper lambda for subscribing to a tag with a local handler
  auto doRobotSubscribe = [this, robotId, messageHandler] (RobotInterface::RobotToEngineTag tagType, localHandlerType handler)
  {
    GetSignalHandles().push_back(messageHandler->Subscribe(robotId, tagType, std::bind(handler, this, std::placeholders::_1)));
  };
  
  // bind to specific handlers
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animationAvailable,    &AnimationComponent::HandleAnimationAvailable);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animStarted,           &AnimationComponent::HandleAnimStarted);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animEnded,             &AnimationComponent::HandleAnimEnded);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::endOfMessage,          &AnimationComponent::HandleEndOfMessage);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animEvent,             &AnimationComponent::HandleAnimationEvent);

}

void AnimationComponent::Init()
{
  _isInitialized = false;
  _animNameToID.clear();
  
  // Request list of available animations from animation process
  _robot.SendRobotMessage<RobotInterface::RequestAvailableAnimations>();
}
  
void AnimationComponent::Update()
{
  if (_isInitialized) {
    DoleAvailableAnimations();
  }
  
  // Check for entries that have stayed in _callbackMap for too long
  const float currTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  auto it = _callbackMap.begin();
  while(it != _callbackMap.end()) {
    if (it->second.abortTime_sec != 0 && currTime_sec >= it->second.abortTime_sec) {
      PRINT_NAMED_WARNING("AnimationComponent.Update.AnimTimedOut", "animID: %d", it->second.animID);
      _robot.SendRobotMessage<RobotInterface::AbortAnimation>(it->first);
      it->second.ExecuteCallback(AnimResult::Timedout);
      it = _callbackMap.erase(it);
    }
    else {
      ++it;
    }
  }
}
 

// Doles animations (the max number that can be doled per tic) to game if requested
void AnimationComponent::DoleAvailableAnimations()
{
  if (_isDolingAnims) {
    u32 numAnimsDoledThisTic = 0;

    auto it = _nextAnimToDole.empty() ? _animNameToID.begin() : _animNameToID.find(_nextAnimToDole);
    for (; it != _animNameToID.end() && numAnimsDoledThisTic < kMaxNumAvailableAnimsToReportPerTic; ++it) {
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(ExternalInterface::AnimationAvailable(it->first)));
      ++numAnimsDoledThisTic;
    }
    if (it == _animNameToID.end()) {
      PRINT_CH_INFO("AnimationComponent", "DoleAvailableAnimations.Done", "");
      _isDolingAnims = false;
      _nextAnimToDole = "";
      _robot.Broadcast(ExternalInterface::MessageEngineToGame(EndOfMessage(MessageType::AnimationAvailable)));
    } else {
      _nextAnimToDole = it->first;
    }
  }
}
  
  
const std::string& AnimationComponent::GetAnimationNameFromGroup(const std::string& name, const Robot& robot) const
{
  const AnimationGroup* group = _animationGroups.GetAnimationGroup(name);
  if(group != nullptr && !group->IsEmpty()) {
    return group->GetAnimationName(robot.GetMoodManager(), _animationGroups, robot.GetHeadAngle());
  }
  static const std::string empty("");
  return empty;
}

  
Result AnimationComponent::PlayAnimByName(const std::string& animName,
                                          int numLoops,
                                          bool interruptRunning,
                                          AnimationCompleteCallback callback,
                                          const u32 actionTag,
                                          float timeout_sec)
{
  if (!_isInitialized) {
    PRINT_NAMED_WARNING("AnimationComponent.PlayAnimByName.Uninitialized", "");
    return RESULT_FAIL;
  }
  
  // Check that animName is valid
  auto it = _animNameToID.find(animName);
  if (it == _animNameToID.end()) {
    PRINT_NAMED_WARNING("AnimationComponent.PlayAnimByName.AnimNotFound", "%s", animName.c_str());
    return RESULT_FAIL;
  }
  
  PRINT_NAMED_DEBUG("AnimationComponent.PlayAnimByName.PlayingAnim", "[%d] %s", it->second, animName.c_str());
  return PlayAnimByID(it->second, numLoops, interruptRunning, callback, actionTag, timeout_sec);
}
    
Result AnimationComponent::PlayAnimByID(const u32 animID,
                                        int numLoops,
                                        bool interruptRunning,
                                        AnimationCompleteCallback callback,
                                        const u32 actionTag,
                                        float timeout_sec)
{
  if (!_isInitialized) {
    PRINT_NAMED_WARNING("AnimationComponent.PlayAnimByID.Uninitialized", "");
    return RESULT_FAIL;
  }
  
  // Check that a valid actionTag was specified if there is non-empty callback
  if (callback != nullptr && actionTag == 0) {
    PRINT_NAMED_WARNING("AnimationComponent.PlayAnimByName.MissingActionTag", "");
    return RESULT_FAIL;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side an optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    PRINT_NAMED_WARNING("AnimationComponent.PlayAnimByID.WontInterruptCurrentAnim", "");
    return RESULT_FAIL;
  }
  
  const Tag currTag = GetNextTag();
  if (_robot.SendRobotMessage<RobotInterface::PlayAnim>(animID, numLoops, currTag) == RESULT_OK) {
    // Check if tag already exists in callback map.
    // If so, trigger callback with Stale
    {
      auto it = _callbackMap.find(currTag);
      if (it != _callbackMap.end()) {
        PRINT_NAMED_WARNING("AnimationComponent.PlayAnimByID.StaleTag", "%d", currTag);
        it->second.ExecuteCallback(AnimResult::Stale);
        _callbackMap.erase(it);
      }
    }
    const float abortTime_sec = (numLoops > 0 ? BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + timeout_sec : 0);
    _callbackMap.emplace(std::piecewise_construct,
                         std::forward_as_tuple(currTag),
                         std::forward_as_tuple(animID, callback, actionTag, abortTime_sec));
  }
  
  return RESULT_OK;
}
  
AnimationComponent::Tag AnimationComponent::IsAnimPlaying(u32 animID)
{
  for (auto it = _callbackMap.begin(); it != _callbackMap.end(); ++it) {
    if (it->second.animID == animID) {
      return it->first;
    }
  }
  return kNotAnimatingTag;
}
  
  
Result AnimationComponent::StopAnimByName(const std::string& animName)
{
  // TODO: This should be on a delay timer and should be smart about
  //       actually aborting the anim vs. playing the next anim down the stack.
  
  // TODO: Animation process handler for this message not hooked up yet!
  
  // Verify that the animation is currently playing
  const auto it = _animNameToID.find(animName);
  if (it != _animNameToID.end()) {
    const auto animID = it->second;
    
    const Tag tag = IsAnimPlaying(animID);
    if (tag != kNotAnimatingTag) {
      return _robot.SendRobotMessage<RobotInterface::AbortAnimation>(tag);
    }
    else {
      PRINT_NAMED_WARNING("AnimationComponent.StopAnimByName.AnimNotPlaying",
                          "%s", animName.c_str());
      return RESULT_OK;
    }
  }
  else {
    PRINT_NAMED_WARNING("AnimationComponent.StopAnimByName.InvalidName",
                        "%s", animName.c_str());
    return RESULT_FAIL;
  }

}

  
// Enables only the specified tracks. 
// Status of other tracks remain unchanged.
void AnimationComponent::EnableTracks(u8 tracks)
{
  _disabledTracks &= ~tracks;
  _robot.SendRobotMessage<RobotInterface::LockAnimTracks>(_disabledTracks);
}

void AnimationComponent::EnableAllTracks()
{
  if (_disabledTracks != 0) {
    _disabledTracks = 0;
    _robot.SendRobotMessage<RobotInterface::LockAnimTracks>(_disabledTracks);
  }
}

// Disables only the specified tracks. 
// Status of other tracks remain unchanged.
void AnimationComponent::DisableTracks(u8 tracks)
{
  _disabledTracks |= tracks;
  _robot.SendRobotMessage<RobotInterface::LockAnimTracks>(_disabledTracks);
}



// ================ Game messsage handlers ======================
template<>
void AnimationComponent::HandleMessage(const ExternalInterface::RequestAvailableAnimations& msg)
{
  PRINT_CH_INFO("AnimationComponent", "RequestAvailableAnimations.Recvd", "");
  _isDolingAnims = true;
}
  
template<>
void AnimationComponent::HandleMessage(const ExternalInterface::DisplayProceduralFace& msg)
{
  if (!_isInitialized) {
    PRINT_NAMED_WARNING("AnimationComponent.DisplayProceduralFace.Uninitialized", "");
    return;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side an optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !msg.interruptRunning) {
    PRINT_NAMED_WARNING("AnimationComponent.DisplayProceduralFace.WontInterruptCurrentAnim", "");
    return;
  }

  // Convert ExternalInterface version of DisplayProceduralFace to RobotInterface version and send
  _robot.SendRobotMessage<RobotInterface::DisplayProceduralFace>(msg.faceParams, msg.duration_ms);
}

template<>
void AnimationComponent::HandleMessage(const ExternalInterface::SetFaceHue& msg)
{
  _robot.SendRobotMessage<RobotInterface::SetFaceHue>(msg.hue);
}
  
// ================ Robot message handlers ======================
  
void AnimationComponent::HandleAnimationAvailable(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animationAvailable();
  //PRINT_CH_DEBUG("AnimationComponent", "AnimationAvailable", "%d: %s", payload.id, payload.name.c_str());
  _animNameToID[payload.name] = payload.id;
}

void AnimationComponent::HandleAnimStarted(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animStarted();
  auto it = _callbackMap.find(payload.tag);
  if (it != _callbackMap.end()) {
    PRINT_CH_INFO("AnimationComponent", "AnimStarted.Tag", "id=%d, tag=%d", payload.id, payload.tag);
  } else {
    PRINT_NAMED_WARNING("AnimationComponent.AnimStarted.UnexpectedTag", "id=%d, tag=%d", payload.id, payload.tag);
  }
}

void AnimationComponent::HandleAnimEnded(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animEnded();
  
  // Verify that expected animation completed and execute callback
  auto it = _callbackMap.find(payload.tag);
  if (it != _callbackMap.end()) {
    PRINT_CH_INFO("AnimationComponent", "AnimEnded.Tag", "id=%d, tag=%d", payload.id, payload.tag);
    it->second.ExecuteCallback(payload.wasAborted ? AnimResult::Aborted : AnimResult::Completed);
    _callbackMap.erase(it);
  } else {
    PRINT_NAMED_WARNING("AnimationComponent.AnimEnded.UnexpectedTag", "id=%d, tag=%d", payload.id, payload.tag);
  }
}
  
void AnimationComponent::HandleEndOfMessage(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_endOfMessage();
  if (payload.messageType == MessageType::AnimationAvailable) {
    PRINT_CH_INFO("AnimationComponent", "EndOfMessage.AnimationAvailable", "%zu animations received", _animNameToID.size());
    _isInitialized = true;
  }
}
  
void AnimationComponent::HandleAnimationEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animEvent();
  auto it = _callbackMap.find(payload.tag);
  if (it != _callbackMap.end()) {
    PRINT_CH_INFO("AnimationComponent", "HandleAnimationEvent", "%s", EnumToString(payload.event_id));
    ExternalInterface::AnimationEvent msg;
    msg.timestamp = payload.timestamp;
    msg.event_id = payload.event_id;
    _robot.GetExternalInterface()->BroadcastToGame<ExternalInterface::AnimationEvent>(std::move(msg));
  }
}
  
  
  
} // namespace Cozmo
} // namespace Anki
