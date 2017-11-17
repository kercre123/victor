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

#include "clad/types/animationTypes.h"

#include "anki/common/basestation/array2d_impl.h"
#include "anki/common/basestation/utils/timer.h"


namespace Anki {
namespace Cozmo {
 
namespace {
  const u32 kMaxNumAvailableAnimsToReportPerTic = 50;

  static const u32 kNumImagePixels     = FACE_DISPLAY_HEIGHT * FACE_DISPLAY_WIDTH;
  static const u32 kNumHalfImagePixels = kNumImagePixels / 2;
}
  
  
AnimationComponent::AnimationComponent(Robot& robot, const CozmoContext* context)
: _isInitialized(false)
, _tagCtr(0)
, _robot(robot)
, _animationGroups(context->GetRobotManager()->GetAnimationGroups())
, _isDolingAnims(false)
, _nextAnimToDole("")
, _currPlayingAnim("")
, _lockedTracks(0)
, _isAnimating(false)
, _currAnimId(0)
, _currAnimTag(0)
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
      helper.SubscribeGameToEngine<MessageGameToEngineTag::DisplayFaceImageBinaryChunk>();
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
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animState,             &AnimationComponent::HandleAnimState);
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
  
  // Check that animID is valid
  if (animID == static_cast<u32>(AnimConstants::PROCEDURAL_ANIM_ID)) {
    PRINT_NAMED_WARNING("AnimationCompnoent.PlayAnimByID.Invalid", "%d", animID);
    return RESULT_FAIL;
  }

  // Check that a valid actionTag was specified if there is non-empty callback
  if (callback != nullptr && actionTag == 0) {
    PRINT_NAMED_WARNING("AnimationComponent.PlayAnimByName.MissingActionTag", "");
    return RESULT_FAIL;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
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
void AnimationComponent::UnlockTracks(u8 tracks)
{
  _lockedTracks &= ~tracks;
  _robot.SendRobotMessage<RobotInterface::LockAnimTracks>(_lockedTracks);
}

void AnimationComponent::UnlockAllTracks()
{
  if (_lockedTracks != 0) {
    _lockedTracks = 0;
    _robot.SendRobotMessage<RobotInterface::LockAnimTracks>(_lockedTracks);
  }
}

// Disables only the specified tracks. 
// Status of other tracks remain unchanged.
void AnimationComponent::LockTracks(u8 tracks)
{
  _lockedTracks |= tracks;
  _robot.SendRobotMessage<RobotInterface::LockAnimTracks>(_lockedTracks);
}


Result AnimationComponent::DisplayFaceImageBinary(const Vision::Image& img, u32 duration_ms, bool interruptRunning)
{
  if (!_isInitialized) {
    PRINT_NAMED_WARNING("AnimationComponent.DisplayFaceImageBinary.Uninitialized", "");
    return RESULT_FAIL;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    PRINT_NAMED_WARNING("AnimationComponent.DisplayFaceImageBinary.WontInterruptCurrentAnim", "");
    return RESULT_FAIL;
  }

  // Verify that image is expected size
  const bool imageIsValidSize = (img.GetNumRows() == FACE_DISPLAY_HEIGHT) && 
                                (img.GetNumCols() == FACE_DISPLAY_WIDTH) &&
                                (img.GetNumChannels() == 1) &&
                                img.IsContinuous();
  
  if (!ANKI_VERIFY(imageIsValidSize, 
                   "AnimationComponent.DisplayFaceImageBinary.InvalidImageSize", 
                   "%d x %d (continuous: %d), expected %d x %d", 
                   img.GetNumCols(), img.GetNumRows(), img.IsContinuous(), FACE_DISPLAY_WIDTH, FACE_DISPLAY_HEIGHT)) {
    return RESULT_FAIL;
  }

  // Convert image into bit images (top half and bottom half)
  const u8* imageData_i = img.GetDataPointer();

  for (int halfIdx = 0; halfIdx < 2; ++halfIdx) {

    RobotInterface::DisplayFaceImageBinaryChunk msg;
    static const u32 kFaceArraySize = sizeof(msg.faceData);
    static_assert(8 * kFaceArraySize == kNumHalfImagePixels, "AnimationComponent.DisplayFaceImageBinary.WrongFaceDataSize");
  
    msg.imageId = 0;
    msg.chunkIndex = halfIdx;
    msg.duration_ms = duration_ms;

    u8* byte = msg.faceData.data();
    for (int i=0; i < kFaceArraySize; ++i) {
      *byte = 0;
      for (int b = 7; b >= 0; --b){
        if (*imageData_i > 0) {
          *byte |= 1 << b;
        }
        ++imageData_i;
      }
      ++byte;
    }
    _robot.SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
  }

  return RESULT_OK;
}

Result AnimationComponent::DisplayFaceImage(const std::array<u16, FACE_DISPLAY_NUM_PIXELS>& imgRGB565, u32 duration_ms, bool interruptRunning)
{
  if (!_isInitialized) {
    PRINT_NAMED_WARNING("AnimationComponent.DisplayFaceImage.Uninitialized", "");
    return RESULT_FAIL;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    PRINT_NAMED_WARNING("AnimationComponent.DisplayFaceImage.WontInterruptCurrentAnim", "");
    return RESULT_FAIL;
  }

  static const int kMaxPixelsPerMsg = RobotInterface::DisplayFaceImageRGBChunk().faceData.size();
  
  int chunkCount = 0;
  int pixelsLeftToSend = FACE_DISPLAY_NUM_PIXELS;
  auto startIt = imgRGB565.begin();
  while (pixelsLeftToSend > 0) {
    RobotInterface::DisplayFaceImageRGBChunk msg;
    msg.duration_ms = duration_ms;
    msg.imageId = 0;
    msg.chunkIndex = chunkCount++;
    msg.numPixels = std::min(kMaxPixelsPerMsg, pixelsLeftToSend);

    std::copy_n(startIt, msg.numPixels, std::begin(msg.faceData));

    pixelsLeftToSend -= msg.numPixels;
    std::advance(startIt, msg.numPixels);

    _robot.SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
  }

  static const int kExpectedNumChunks = static_cast<int>(std::ceilf( (f32)FACE_DISPLAY_NUM_PIXELS / kMaxPixelsPerMsg ));
  DEV_ASSERT_MSG(chunkCount == kExpectedNumChunks, "AnimationComponent.DisplayFaceImage.UnexpectedNumChunks", "%d", chunkCount);

  return RESULT_OK;
}

static void ConvertRGB24toRGB565(const Vision::ImageRGB& faceImg, std::array<u16, FACE_DISPLAY_NUM_PIXELS>& faceImg565)
{
  DEV_ASSERT(faceImg.IsContinuous(), "AnimationComponent.ConvertRGB24toRGB565.FaceImgNotContinuous");
 
  const Vision::PixelRGB* faceImg_i = faceImg.get_CvMat_().ptr<Vision::PixelRGB>(0);
  
  for(int j = 0; j < FACE_DISPLAY_NUM_PIXELS; ++j)
  {
    const Vision::PixelRGB& pixRGB24 = faceImg_i[j];
    u16& pixRGB565 = faceImg565[j];
    
    // Convert to RGB565
    pixRGB565 = (((int)(pixRGB24.r() >> 3) << 11) |
                 ((int)(pixRGB24.g() >> 2) << 5)  |
                 ((int)(pixRGB24.b() >> 3) << 0));
  }
}

Result AnimationComponent::DisplayFaceImage(const Vision::ImageRGB& img, u32 duration_ms, bool interruptRunning)
{
  std::array<u16, FACE_DISPLAY_NUM_PIXELS> img565;
  ConvertRGB24toRGB565(img, img565);
  return DisplayFaceImage(img565, duration_ms, interruptRunning);
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
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
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

template<>
void AnimationComponent::HandleMessage(const ExternalInterface::DisplayFaceImageBinaryChunk& msg)
{
  if (!_isInitialized) {
    PRINT_NAMED_WARNING("AnimationComponent.HandleDisplayFaceImageBinaryChunk.Uninitialized", "");
    return;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !msg.interruptRunning) {
    PRINT_NAMED_WARNING("AnimationComponent.HandleDisplayFaceImage.WontInterruptCurrentAnim", "");
    return;
  }

  // Convert ExternalInterface version of DisplayFaceImage to RobotInterface version and send
  _robot.SendRobotMessage<RobotInterface::DisplayFaceImageBinaryChunk>(msg.duration_ms, msg.faceData, msg.imageId, msg.chunkIndex);
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
  } else if (payload.id != static_cast<u32>(AnimConstants::PROCEDURAL_ANIM_ID)) {
    PRINT_NAMED_WARNING("AnimationComponent.AnimStarted.UnexpectedTag", "id=%d, tag=%d", payload.id, payload.tag);
    return;
  }

  _isAnimating = true;
  _currAnimId = payload.id;
  _currAnimTag = payload.tag;
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
  } else if (payload.id != static_cast<u32>(AnimConstants::PROCEDURAL_ANIM_ID)) {
    PRINT_NAMED_WARNING("AnimationComponent.AnimEnded.UnexpectedTag", "id=%d, tag=%d", payload.id, payload.tag);
    return;
  }

  _isAnimating = false;
  DEV_ASSERT_MSG(_currAnimId == payload.id, "AnimationComponent.AnimEnded.UnexpectedId", "Got %d, expected %d", payload.id, _currAnimId);
  DEV_ASSERT_MSG(_currAnimTag == payload.tag, "AnimationComponent.AnimEnded.UnexpectedTag", "Got %d, expected %d", payload.tag, _currAnimTag);
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
  
void AnimationComponent::HandleAnimState(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  _animState = message.GetData().Get_animState();
}
  
  
} // namespace Cozmo
} // namespace Anki
