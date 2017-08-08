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
#include "engine/components/animationComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotManager.h"
#include "engine/robotInterface/messageHandler.h"


namespace Anki {
namespace Cozmo {
 
namespace {
  const u32 kMaxNumAvailableAnimsToReportPerTic = 50;
}
  
  
AnimationComponent::AnimationComponent(Robot& robot, const CozmoContext* context)
: _isInitialized(false)
, _robot(robot)
, _isDolingAnims(false)
, _nextAnimToDole("")
{
  if (context) {
    // Setup game message handlers
    IExternalInterface *extInterface = context->GetExternalInterface();
    if (extInterface != nullptr) {
      
      auto helper = MakeAnkiEventUtil(*extInterface, *this, GetSignalHandles());
  
      using namespace ExternalInterface;
      helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestAvailableAnimations>();
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
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animationAvailable,             &AnimationComponent::HandleAnimationAvailable);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animStarted,                    &AnimationComponent::HandleAnimStarted);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animEnded,                      &AnimationComponent::HandleAnimEnded);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::endOfMessage,                   &AnimationComponent::HandleEndOfMessage);
  
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
}
 

void AnimationComponent::DoleAvailableAnimations()
{
  // If not already doling, dole animations
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
  
  
Result AnimationComponent::PlayAnimByName(const std::string& animName)
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
  return PlayAnimByID(it->second);
}
  
  
Result AnimationComponent::PlayAnimByID(u32 animID)
{
  if (!_isInitialized) {
    PRINT_NAMED_WARNING("AnimationComponent.PlayAnimByID.Uninitialized", "");
    return RESULT_FAIL;
  }
  
  return _robot.SendRobotMessage<RobotInterface::PlayAnim>(animID);
}
  
  
  
  
// ================ Game messsage handlers ======================
template<>
void AnimationComponent::HandleMessage(const ExternalInterface::RequestAvailableAnimations& msg)
{
  PRINT_CH_INFO("AnimationComponent", "RequestAvailableAnimations.Recvd", "");
  _isDolingAnims = true;
}
  
  
// ================ Robot message handlers ======================
  
void AnimationComponent::HandleAnimationAvailable(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animationAvailable();
  PRINT_CH_INFO("AnimationComponent", "AnimationAvailable", "%d: %s", payload.id, payload.name.c_str());
  
  _animNameToID[payload.name] = payload.id;
}

void AnimationComponent::HandleAnimStarted(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animStarted();
  PRINT_CH_INFO("AnimationComponent", "AnimStarted.Tag", "%d", payload.tag);
}

void AnimationComponent::HandleAnimEnded(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animEnded();
  PRINT_CH_INFO("AnimationComponent", "AnimEnded.Tag", "%d", payload.tag);
}
  
void AnimationComponent::HandleEndOfMessage(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_endOfMessage();
  if (payload.messageType == MessageType::AnimationAvailable) {
    PRINT_CH_INFO("AnimationComponent", "EndOfMessage.AnimationAvailable", "");
    _isInitialized = true;
  }
}
  
  
} // namespace Cozmo
} // namespace Anki
