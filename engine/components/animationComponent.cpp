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
#include "engine/components/dataAccessorComponent.h"
#include "engine/components/movementComponent.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotManager.h"
#include "engine/robotInterface/messageHandler.h"

#include "clad/types/animationTypes.h"

#include "cannedAnimLib/cannedAnims/animation.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "coretech/common/shared/array2d_impl.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/shared/compositeImage/compositeImage.h"
#include "coretech/vision/shared/rgb565Image/rgb565ImageBuilder.h"

#include "json/json.h"

#include "util/string/stringUtils.h"

#define LOG_CHANNEL "Animations"

namespace Anki {
namespace Vector {
 
namespace {
  
  static const AnimationComponent::Tag kInvalidAnimationTag = 0;

  const u32 kMaxNumAvailableAnimsToReportPerTic = 1000;

  static const u32 kNumImagePixels     = FACE_DISPLAY_HEIGHT * FACE_DISPLAY_WIDTH;
  static const u32 kNumHalfImagePixels = kNumImagePixels / 2;

  static const int kMaxAnimGroupRecursionDepth = 100;
}
  
CONSOLE_VAR(f32, kEyeDartFocusValue_pix, "Animation", 1.0f);
  
AnimationComponent::AnimationComponent()
: IDependencyManagedComponent(this, RobotComponentID::Animation)
, _isInitialized(false)
, _tagCtr(kInvalidAnimationTag)
, _isDolingAnims(false)
, _nextAnimToDole("")
, _currPlayingAnim("")
, _isAnimating(false)
, _currAnimName("")
, _currAnimTag(0)
, _oledImageBuilder(new Vision::RGB565ImageBuilder)
, _tagForTriggerWordGetInCallbacks(GetNextTag())
, _tagForAlexaListening(GetNextTag())
, _tagForAlexaThinking(GetNextTag())
, _tagForAlexaSpeaking(GetNextTag())
, _tagForAlexaError(GetNextTag())
, _compositeImageID(0)
{

}

void AnimationComponent::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
  _dataAccessor = dependentComps.GetComponentPtr<DataAccessorComponent>();
  _movementComponent = dependentComps.GetComponentPtr<MovementComponent>();
  const CozmoContext* context = _robot->GetContext();
  _animationGroups = std::make_unique<AnimationGroupWrapper>(*(context->GetDataLoader()->GetAnimationGroups()));

  // Setup game message handlers
  IExternalInterface *extInterface = context->GetExternalInterface();
  if (extInterface != nullptr) {
    
    auto helper = MakeAnkiEventUtil(*extInterface, *this, GetSignalHandles());

    using namespace ExternalInterface;
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestAvailableAnimations>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::RequestAvailableAnimationGroups>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DisplayProceduralFace>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::SetFaceHue>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DisplayFaceImageBinaryChunk>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::DisplayFaceImageRGBChunk>();
    helper.SubscribeGameToEngine<MessageGameToEngineTag::ReadAnimationFile>();
  }
  
  // Setup robot message handlers
  RobotInterface::MessageHandler *messageHandler = _robot->GetContext()->GetRobotManager()->GetMsgHandler();

  // Subscribe to RobotToEngine messages
  using localHandlerType = void(AnimationComponent::*)(const AnkiEvent<RobotInterface::RobotToEngine>&);
  // Create a helper lambda for subscribing to a tag with a local handler
  auto doRobotSubscribe = [this, messageHandler] (RobotInterface::RobotToEngineTag tagType, localHandlerType handler)
  {
    GetSignalHandles().push_back(messageHandler->Subscribe(tagType, std::bind(handler, this, std::placeholders::_1)));
  };
  
  // bind to specific handlers
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animAdded,             &AnimationComponent::HandleAnimAdded);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animStarted,           &AnimationComponent::HandleAnimStarted);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animEnded,             &AnimationComponent::HandleAnimEnded);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animEvent,             &AnimationComponent::HandleAnimationEvent);
  doRobotSubscribe(RobotInterface::RobotToEngineTag::animState,             &AnimationComponent::HandleAnimState);
}


void AnimationComponent::Init()
{
  if(_isInitialized)
  {
    LOG_INFO("AnimationComponent.Init.AlreadyInited","");
    return;
  }
  
  // Open manifest file
  static const std::string manifestFile = "assets/anim_manifest.json";
  Json::Value jsonManifest;
  const bool success = _robot->GetContext()->GetDataPlatform()->readAsJson(Util::Data::Scope::Resources, manifestFile, jsonManifest);
  if (!success) {
    LOG_ERROR("AnimationComponent.Init.ManifestNotFound", "");
    return;
  }

  // Process animations in manifest
  _availableAnims.clear();
  for (int i=0; i<jsonManifest.size(); ++i) {
    const Json::Value& jsonAnim = jsonManifest[i];
    
    static const char* kNameField = "name";
    static const char* kLengthField = "length_ms";

    if (!jsonAnim.isMember(kNameField)) {
      LOG_ERROR("AnimationComponent.Init.MissingJsonField", "%s", kNameField);
      continue;
    }

    if (!jsonAnim.isMember(kLengthField)) {
      LOG_ERROR("AnimationComponent.Init.MissingJsonField", "%s", kLengthField);
      continue;
    }

    _availableAnims[jsonAnim[kNameField].asCString()].length_ms = jsonAnim[kLengthField].asInt();
  }
  LOG_INFO("AnimationComponent.Init.ManifestRead", "%zu animations loaded", _availableAnims.size());

  if( ANKI_DEVELOPER_CODE ) {
    // now that we loaded the animations, go check the animation whitelist and make sure everything in there is
    // a valid animation prefix
    const auto& whitelistedPrefixes = _robot->GetContext()->GetDataLoader()->GetAllWhitelistedChargerAnimationPrefixes();
    for( const auto& prefix : whitelistedPrefixes ) {
      // Ensure that at least one available animation has the given prefix
      bool hasMatchingAnim = false;
      for (const auto& availableAnim : _availableAnims) {
        if (Util::StringStartsWith(availableAnim.first, prefix)) {
          hasMatchingAnim = true;
          break;
        }
      }
      if (!hasMatchingAnim) {
        LOG_WARNING("AnimationComponent.AnimWhitelistInvalid",
                    "Anim whitelist in RobotDataLoader contains prefix '%s' for which there is no valid clip name",
                    prefix.c_str());
      }
    }
  }

  _isInitialized = true;
}
  

void AnimationComponent::UpdateDependent(const RobotCompMap& dependentComps)
{
  if (_isInitialized) {
    DoleAvailableAnimations();
  }
  
  // Check for entries that have stayed in _callbackMap for too long
  const float currTime_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  auto it = _callbackMap.begin();
  while(it != _callbackMap.end()) {
    if (it->second.abortTime_sec != 0 && currTime_sec >= it->second.abortTime_sec) {
      LOG_WARNING("AnimationComponent.Update.AnimTimedOut", "Anim: %s", it->second.animName.c_str());
      _robot->SendRobotMessage<RobotInterface::AbortAnimation>(it->first);
      it->second.ExecuteCallback(AnimResult::Timedout, static_cast<u32>(it->second.abortTime_sec));
      it = _callbackMap.erase(it);
    }
    else {
      ++it;
    }
  }

  if( _desiredEnableKeepFaceAlive != _lastSentEnableKeepFaceAlive ) {
    SendEnableKeepFaceAlive(_desiredEnableKeepFaceAlive);
  }
}
 

Result AnimationComponent::GetAnimationMetaInfo(const std::string& animName, AnimationMetaInfo& metaInfo) const
{
  auto it = _availableAnims.find(animName);
  if (it != _availableAnims.end()) {
    metaInfo = it->second;
    return RESULT_OK;
  }
  
  return RESULT_FAIL;
}

// Doles animations (the max number that can be doled per tic) to game if requested
void AnimationComponent::DoleAvailableAnimations()
{
  if (_isDolingAnims) {
    u32 numAnimsDoledThisTic = 0;

    auto it = _nextAnimToDole.empty() ? _availableAnims.begin() : _availableAnims.find(_nextAnimToDole);
    for (; it != _availableAnims.end() && numAnimsDoledThisTic < kMaxNumAvailableAnimsToReportPerTic; ++it) {
      _robot->Broadcast(ExternalInterface::MessageEngineToGame(
        ExternalInterface::AnimationAvailable(it->first))
      );
      ++numAnimsDoledThisTic;
    }
    if (it == _availableAnims.end()) {
      LOG_INFO("DoleAvailableAnimations.Done", "");
      _isDolingAnims = false;
      _nextAnimToDole = "";
      _robot->Broadcast(ExternalInterface::MessageEngineToGame(
        ExternalInterface::EndOfMessage(ExternalInterface::MessageType::AnimationAvailable))
      );
    } else {
      _nextAnimToDole = it->first;
    }
  }
}


const std::string& AnimationComponent::GetAnimationNameFromGroup(const std::string& name,
                                                                 bool strictCooldown,
                                                                 int recursionCount) const
{
  static const std::string empty("");

  if( !ANKI_VERIFY( recursionCount < kMaxAnimGroupRecursionDepth,
                    "AnimationComponent.GetAnimationNameFromGroup.ExceededMaxREcursionDepth",
                    "Recursion loop! Recursed %d times to group name '%s'",
                    recursionCount,
                    name.c_str()) ) {
    // just give up after max recursion depth
    return empty;
  }

  const AnimationGroup* group = _animationGroups->_container.GetAnimationGroup(name);
  if(group != nullptr && !group->IsEmpty()) {
    const std::string& selectedAnim = group->GetAnimationName(_robot->GetMoodManager(),
                                                              _animationGroups->_container,
                                                              _robot->GetComponent<FullRobotPose>().GetHeadAngle(),
                                                              strictCooldown);
    if( _animationGroups->_container.HasGroup( selectedAnim ) ) {
      LOG_INFO("GetAnimationName.SubGroupSelected",
               "Group %s returned sub-group %s, going deeper",
               name.c_str(),
               selectedAnim.c_str());
      return GetAnimationNameFromGroup(selectedAnim, strictCooldown, recursionCount+1);
    }
    else {
      return selectedAnim;
    }
  }
  return empty;
}
  
Result AnimationComponent::PlayAnimByName(const std::string& animName,
                                          int numLoops,
                                          bool interruptRunning,
                                          AnimationCompleteCallback callback,
                                          const u32 actionTag,
                                          float timeout_sec,
                                          u32 startAt_ms,
                                          bool renderInEyeHue)
{
  if (!_isInitialized) {
    LOG_WARNING("AnimationComponent.PlayAnimByName.Uninitialized", "");
    return RESULT_FAIL;
  }

  // Check that animName is valid
  auto it = _availableAnims.find(animName);
  if (it == _availableAnims.end()) {
    LOG_WARNING("AnimationComponent.PlayAnimByName.AnimNotFound", "%s", animName.c_str());
    return RESULT_FAIL;
  }
  
  LOG_DEBUG("AnimationComponent.PlayAnimByName.PlayingAnim", "%s", it->first.c_str());

  // Check that a valid actionTag was specified if there is non-empty callback
  if (callback != nullptr && actionTag == 0) {
    LOG_WARNING("AnimationComponent.PlayAnimByName.MissingActionTag", "");
    return RESULT_FAIL;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    LOG_INFO("AnimationComponent.PlayAnimByName.WontInterruptCurrentAnim", "");
    return RESULT_FAIL;
  }

  const Tag currTag = GetNextTag();
  if (_robot->SendRobotMessage<RobotInterface::PlayAnim>(numLoops, startAt_ms, currTag, renderInEyeHue, animName) == RESULT_OK) {
    SetAnimationCallback(animName, callback, currTag, actionTag, numLoops, timeout_sec);
  }
  
  return RESULT_OK;
}


Result AnimationComponent::PlayCompositeAnimation(const std::string& animName,
                                                  const Vision::CompositeImage& compositeImage, 
                                                  u32 frameInterval_ms,
                                                  int& outDuration_ms,
                                                  bool interruptRunning,
                                                  bool emptySpriteBoxesAreValid,
                                                  AnimationCompleteCallback callback)
{
  if (!_isInitialized) {
    LOG_WARNING("AnimationComponent.PlayCompositeAnimation.Uninitialized", "");
    return RESULT_FAIL;
  }

  // Check that animName is valid
  auto it = _availableAnims.find(animName);
  if (it == _availableAnims.end()) {
    LOG_WARNING("AnimationComponent.PlayCompositeAnimation.AnimNotFound", "%s", animName.c_str());
    return RESULT_FAIL;
  }
  
  LOG_DEBUG("AnimationComponent.PlayCompositeAnimation.PlayingAnim", "%s", it->first.c_str());
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    LOG_INFO("AnimationComponent.PlayCompositeAnimation.WontInterruptCurrentAnim", "");
    return RESULT_FAIL;
  }

  const Animation* anim = nullptr;
  bool gotAnim = false;
  if(_dataAccessor != nullptr){
    const auto* animContainer = _dataAccessor->GetCannedAnimationContainer();
    if(animContainer != nullptr){
      anim = animContainer->GetAnimation(animName);
      gotAnim = (anim != nullptr);
    }
  }
  
  if(!gotAnim){
    LOG_WARNING("AnimationComponent.PlayCompositeAnimation.AnimationNotFoundInContainer",
                "Animations need to be manually loaded on engine side - %s is not", animName.c_str());
    return RESULT_FAIL;
  }

  const Tag currTag = GetNextTag();
  _robot->SendRobotMessage<RobotInterface::PlayCompositeAnimation>(_compositeImageID, currTag, animName);
  if(callback != nullptr){
    SetAnimationCallback(animName, callback, currTag, 0, 0, 0);
  }

  outDuration_ms = anim->GetLastKeyFrameEndTime_ms();
  return DisplayFaceImage(compositeImage, frameInterval_ms, outDuration_ms, interruptRunning, emptySpriteBoxesAreValid);
}

  
AnimationComponent::Tag AnimationComponent::IsAnimPlaying(const std::string& animName)
{
  for (auto it = _callbackMap.begin(); it != _callbackMap.end(); ++it) {
    if (it->second.animName == animName) {
      return it->first;
    }
  }
  return kNotAnimatingTag;
}
  
  
Result AnimationComponent::StopAnimByName(const std::string& animName)
{
  // Verify that the animation is currently playing
  const auto it = _availableAnims.find(animName);
  if (it != _availableAnims.end()) {
    const Tag tag = IsAnimPlaying(animName);
    if (tag != kNotAnimatingTag) {
      LOG_DEBUG("AnimationComponent.StopAnimByName.AbortingAnim", "%s (%d)", animName.c_str(), tag);
      return _robot->SendRobotMessage<RobotInterface::AbortAnimation>(tag);
    }
    else {
      LOG_WARNING("AnimationComponent.StopAnimByName.AnimNotPlaying",
                  "%s", animName.c_str());
      return RESULT_OK;
    }
  }
  else {
    LOG_WARNING("AnimationComponent.StopAnimByName.InvalidName",
                "%s", animName.c_str());
    return RESULT_FAIL;
  }

}


void AnimationComponent::AlterStreamingAnimationAtTime(RobotInterface::EngineToRobot&& msg, 
                                                       TimeStamp_t relativeStreamTime_ms,  
                                                       bool applyBeforeTick,
                                                       MovementComponent* devSafetyCheck)
{
  RobotInterface::AlterStreamingAnimationAtTime alterMsg;
  alterMsg.relativeStreamTime_ms = relativeStreamTime_ms;
  alterMsg.applyBeforeTick = applyBeforeTick;
  alterMsg.internalTag = static_cast<uint8_t>(msg.GetTag());
  switch(msg.GetTag()){
    case RobotInterface::EngineToRobotTag::setFullAnimTrackLockState:
    {
      DEV_ASSERT(devSafetyCheck == _movementComponent, 
        "AnimationComponent.AlterStreamingAnimationAtTime.DirectlyQueueingMessageThatMustGoThroughMovementComponent");
      alterMsg.setFullAnimTrackLockState = std::move(msg.Get_setFullAnimTrackLockState());
      break;
    }
    case RobotInterface::EngineToRobotTag::postAudioEvent:
    {
      alterMsg.postAudioEvent = std::move(msg.Get_postAudioEvent());
      break;
    }
    case RobotInterface::EngineToRobotTag::textToSpeechPlay:
    {
      alterMsg.textToSpeechPlay = std::move(msg.Get_textToSpeechPlay());
      break;
    }

    default:
    {
      LOG_ERROR("AnimationComponent.AlterStreamingAnimationAtTime.UnsupportedMessageType",
                "Message Type %s is not currently implemented - update clad and anim process to support",
                RobotInterface::EngineToRobotTagToString(msg.GetTag()));
      break;
    }
  }
  
  RobotInterface::EngineToRobot wrapper(std::move(alterMsg));
  _robot->SendMessage(wrapper);
}


Result AnimationComponent::DisplayFaceImageBinary(const Vision::Image& img, u32 duration_ms, bool interruptRunning)
{
  if (!_isInitialized) {
    LOG_WARNING("AnimationComponent.DisplayFaceImageBinary.Uninitialized", "");
    return RESULT_FAIL;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    LOG_INFO("AnimationComponent.DisplayFaceImageBinary.WontInterruptCurrentAnim", "");
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
    _robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
  }

  return RESULT_OK;
}

Result AnimationComponent::DisplayFaceImage(const Vision::Image& img, u32 duration_ms, bool interruptRunning)
{
  return DisplayFaceImageHelper<RobotInterface::DisplayFaceImageGrayscaleChunk>(img, duration_ms, interruptRunning);
}
  
Result AnimationComponent::DisplayFaceImage(const Vision::ImageRGB565& imgRGB565, u32 duration_ms, bool interruptRunning)
{
  return DisplayFaceImageHelper<RobotInterface::DisplayFaceImageRGBChunk>(imgRGB565, duration_ms, interruptRunning);
}

Result AnimationComponent::DisplayFaceImage(const Vision::ImageRGB& img, u32 duration_ms, bool interruptRunning)
{
  static Vision::ImageRGB565 img565; // static to avoid repeatedly allocating this once it's used
  img565.SetFromImageRGB(img);
  return DisplayFaceImage(img565, duration_ms, interruptRunning);
}

template <typename MessageType, typename ImageType>
Result AnimationComponent::DisplayFaceImageHelper(const ImageType& img, u32 duration_ms, bool interruptRunning)
{
  // Make sure types logically match (e.g. message type should not be grayscale if image type is RGB)
  static_assert((std::is_same<MessageType, RobotInterface::DisplayFaceImageGrayscaleChunk>::value &&
                 std::is_same<ImageType,   Vision::Image>::value) ||
                (std::is_same<MessageType, RobotInterface::DisplayFaceImageRGBChunk>::value &&
                 std::is_same<ImageType,   Vision::ImageRGB565>::value),
                "invalid types");
  
  if (!_isInitialized) {
    LOG_WARNING("AnimationComponent.DisplayFaceImage.Uninitialized", "");
    return RESULT_FAIL;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    LOG_INFO("AnimationComponent.DisplayFaceImage.WontInterruptCurrentAnim", "");
    return RESULT_FAIL;
  }
  
  ASSERT_NAMED(img.IsContinuous(), "AnimationComponent.DisplayFaceImage.NotContinuous");
  ASSERT_NAMED(img.GetNumRows() == FACE_DISPLAY_HEIGHT, "AnimationComponent.DisplayFaceImage.IncorrectImageHeight");
  ASSERT_NAMED(img.GetNumCols() == FACE_DISPLAY_WIDTH,  "AnimationComponent.DisplayFaceImage.IncorrectImageWidth");
  
  MessageType msg;
  const int kMaxPixelsPerMsg = msg.faceData.size();
  
  int chunkCount = 0;
  int pixelsLeftToSend = FACE_DISPLAY_NUM_PIXELS;
  const auto* startIt = img.GetRawDataPointer();
  // TODO: Figure out how to make this assert work
  //static_assert(std::is_same<typename std::remove_pointer<decltype(startIt)>::type, typename decltype(msg.faceData)::value_type>::value, "wrong type");
  while (pixelsLeftToSend > 0) {
    msg.duration_ms = duration_ms;
    msg.imageId = 0;
    msg.chunkIndex = chunkCount++;
    msg.numPixels = std::min(kMaxPixelsPerMsg, pixelsLeftToSend);
    
    std::copy_n(startIt, msg.numPixels, std::begin(msg.faceData));
    
    pixelsLeftToSend -= msg.numPixels;
    std::advance(startIt, msg.numPixels);
    
    _robot->SendMessage(RobotInterface::EngineToRobot(MessageType(msg)));
  }
  
  static const int kExpectedNumChunks = static_cast<int>(std::ceilf( (f32)FACE_DISPLAY_NUM_PIXELS / kMaxPixelsPerMsg ));
  DEV_ASSERT_MSG(chunkCount == kExpectedNumChunks, "AnimationComponent.DisplayFaceImageHelper.UnexpectedNumChunks", "%d", chunkCount);
  
  return RESULT_OK;
}


void AnimationComponent::SetAnimationCallback(const std::string& animName,
                                              AnimationCompleteCallback callback,
                                              const u32 currTag,
                                              const u32 actionTag,
                                              int numLoops,
                                              float timeout_sec,
                                              bool callbackStillValidEvenIfTagIsNot)
{
  const float abortTime_sec = (numLoops > 0 ? BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + timeout_sec : 0);
  _callbackMap.emplace(std::piecewise_construct,
                        std::forward_as_tuple(currTag),
                        std::forward_as_tuple(animName, callback, actionTag, abortTime_sec, callbackStillValidEvenIfTagIsNot));
}


Result AnimationComponent::DisplayFaceImage(const Vision::CompositeImage& compositeImage, 
                                            u32 frameInterval_ms, u32 duration_ms, 
                                            bool interruptRunning, bool emptySpriteBoxesAreValid)
{
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    LOG_INFO("AnimationComponent.DisplayFaceImage.WontInterruptCurrentAnim", "");
    return RESULT_FAIL;
  }

  // Send the image to the animation process in chunks
  const std::vector<Vision::CompositeImageChunk> imageChunks = compositeImage.GetImageChunks(emptySpriteBoxesAreValid);
  for(const auto& chunk: imageChunks){
    _robot->SendRobotMessage<RobotInterface::DisplayCompositeImageChunk>(_compositeImageID, frameInterval_ms, duration_ms, chunk);
  }
  
  if(imageChunks.empty()){
    LOG_WARNING("AnimationComponent.DisplayFaceImage.NoImageChunksToSend", "");
    return RESULT_FAIL;
  }

  _compositeImageID++;
  return RESULT_OK;
}

void AnimationComponent::UpdateCompositeImage(const Vision::CompositeImage& compositeImage, u32 applyAt_ms)
{
  for(const auto& layoutPair : compositeImage.GetLayerLayoutMap()){
    Vision::LayerName layerName = layoutPair.first;
    for(const auto& spritePair : layoutPair.second.GetLayoutMap()){
      Vision::SerializedSpriteBox serializedSpriteBox = spritePair.second.Serialize();
      Vision::CompositeImageLayer::SpriteEntry entry;
      layoutPair.second.GetSpriteEntry(spritePair.second, entry);

      _robot->SendRobotMessage<RobotInterface::UpdateCompositeImage>(serializedSpriteBox, 
                                                                     applyAt_ms, 
                                                                     entry.GetAssetID(),
                                                                     layerName);
    }
  }
}


void AnimationComponent::ClearCompositeImageLayer(Vision::LayerName layerName, u32 applyAt_ms)
{
  // Setup empty sprite entry
  Vision::CompositeImageLayer::SpriteEntry entry;

  // Setup empty spriteBox
  Vision::SpriteRenderConfig renderConfig;
  renderConfig.renderMethod = Vision::SpriteRenderMethod::RGBA;
  Point2i topLeft(0,0);
  Vision::CompositeImageLayer::SpriteBox sb(Vision::SpriteBoxName::Count, renderConfig, topLeft, 0, 0);
  
  _robot->SendRobotMessage<RobotInterface::UpdateCompositeImage>(sb.Serialize(), 
                                                                 applyAt_ms, 
                                                                 entry.GetAssetID(),
                                                                 layerName); 
}

void AnimationComponent::AddKeepFaceAliveDisableLock(const std::string& lockName)
{
  _numKeepFaceAliveDisableLocks++;

  LOG_INFO("AnimationComponent.KeepFaceAlive.DisableLock.Locked",
           "Adding disable lock for '%s', count is %d locks",
           lockName.c_str(),
           _numKeepFaceAliveDisableLocks);

  if( _numKeepFaceAliveDisableLocks == 1 ) {
    // this is the first lock
    _desiredEnableKeepFaceAlive = false;
  }
}

void AnimationComponent::RemoveKeepFaceAliveDisableLock(const std::string& lockName)
{
  if( ANKI_VERIFY( _numKeepFaceAliveDisableLocks > 0,
                   "AnimationComponent.RemoveKeepFaceAliveDisableLock.NotLocked",
                   "Removing lock '%s', but no locks present",
                   lockName.c_str() ) ) {

    _numKeepFaceAliveDisableLocks--;

    LOG_INFO("AnimationComponent.KeepFaceAlive.DisableLock.Removed",
             "Removed disable lock for '%s', count is %d locks",
             lockName.c_str(),
             _numKeepFaceAliveDisableLocks);

    if( _numKeepFaceAliveDisableLocks == 0 ) {
      // was locked but not anymore, so enable
      _desiredEnableKeepFaceAlive = true;
    }
  }
}

Result AnimationComponent::SendEnableKeepFaceAlive(bool enable, u32 disableTimeout_ms)
{
  LOG_DEBUG("AnimationComponent.EnableKeepFaceAlive.SendMessage",
            "Sending message to %s procedural keep face alive",
            enable ? "ENABLE" : "DISABLE");

  const Result res = _robot->SendRobotMessage<RobotInterface::EnableKeepFaceAlive>(disableTimeout_ms, enable);

  if( res == RESULT_OK ) {
    _lastSentEnableKeepFaceAlive = enable;
  }
  
  return res;
}

Result AnimationComponent::AddOrUpdateEyeShift(const std::string& name, 
                                               f32 xPix,
                                               f32 yPix,
                                               TimeStamp_t duration_ms,
                                               f32 xMax,
                                               f32 yMax,
                                               f32 lookUpMaxScale,
                                               f32 lookDownMinScale,
                                               f32 outerEyeScaleIncrease)
{
  Result res = _robot->SendRobotMessage<RobotInterface::AddOrUpdateEyeShift>(xPix,
                                                                             yPix,
                                                                             duration_ms,
                                                                             xMax,
                                                                             yMax,
                                                                             lookUpMaxScale,
                                                                             lookDownMinScale,
                                                                             outerEyeScaleIncrease,
                                                                             name);
  if (res == RESULT_OK) {
    _activeEyeShiftLayers.insert(name);
  }
  return res;
}
  
Result AnimationComponent::RemoveEyeShift(const std::string& name, u32 disableTimeout_ms)
{
  if (IsEyeShifting(name)) {
    Result res = _robot->SendRobotMessage<RobotInterface::RemoveEyeShift>(disableTimeout_ms, name);
    if (res == RESULT_OK) {
      _activeEyeShiftLayers.erase(name);
    }
    return res;
  }
  return RESULT_OK;
}
  
Result AnimationComponent::AddSquint(const std::string& name, f32 squintScaleX, f32 squintScaleY, f32 upperLidAngle)
{
  Result res = _robot->SendRobotMessage<RobotInterface::AddSquint>(squintScaleX, squintScaleY, upperLidAngle, name);
  if (res == RESULT_OK) {
    _activeEyeSquintLayers.insert(name);
  }
  return res;
}

Result AnimationComponent::RemoveSquint(const std::string& name, u32 disableTimeout_ms)
{
  if (IsEyeSquinting(name)) {
    Result res = _robot->SendRobotMessage<RobotInterface::RemoveSquint>(disableTimeout_ms, name);
    if (res == RESULT_OK) {
      _activeEyeSquintLayers.erase(name);
    }
    return res;
  }
  return RESULT_OK;
}

Result AnimationComponent::SetFaceSaturation(float level)
{
  const Result res = _robot->SendRobotMessage<RobotInterface::SetFaceSaturation>(level);
  return res;
}

AnimationTag AnimationComponent::SetTriggerWordGetInCallback(std::function<void(bool)> callbackFunction)
{
  _triggerWordGetInCallbackFunction = callbackFunction;
  return _tagForTriggerWordGetInCallbacks;
}
  
std::array<AnimationTag,4> AnimationComponent::SetAlexaUXResponseCallback(std::function<void(unsigned int, bool)> callback)
{
  _alexaResponseCallback = callback;
  const std::array<AnimationTag,4> tags = {{_tagForAlexaListening, _tagForAlexaThinking, _tagForAlexaSpeaking, _tagForAlexaError}};
  return tags;
}


// ================ Game message handlers ======================
template<>
void AnimationComponent::HandleMessage(const ExternalInterface::RequestAvailableAnimations& msg)
{
  LOG_INFO("RequestAvailableAnimations.Recvd", "");
  _isDolingAnims = true;
}
  
template<>
void AnimationComponent::HandleMessage(const ExternalInterface::RequestAvailableAnimationGroups& msg)
{
  std::vector<std::string> animNames(_robot->GetContext()->GetDataLoader()->GetAnimationGroups()->GetAnimationGroupNames());
  for (std::vector<std::string>::iterator i=animNames.begin(); i != animNames.end(); ++i) {
    _robot->GetExternalInterface()->BroadcastToGame<ExternalInterface::AnimationGroupAvailable>(*i);
  }
}
  
template<>
void AnimationComponent::HandleMessage(const ExternalInterface::DisplayProceduralFace& msg)
{
  static_assert( std::tuple_size<decltype(msg.faceParams.leftEye)>::value == (size_t)ProceduralEyeParameter::NumParameters,
                "LeftEye parameter array is the wrong length");
  static_assert( std::tuple_size<decltype(msg.faceParams.rightEye)>::value == (size_t)ProceduralEyeParameter::NumParameters,
                "RightEye parameter array is the wrong length");
  
  if (!_isInitialized) {
    LOG_WARNING("AnimationComponent.DisplayProceduralFace.Uninitialized", "");
    return;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !msg.interruptRunning) {
    LOG_INFO("AnimationComponent.DisplayProceduralFace.WontInterruptCurrentAnim", "");
    return;
  }

  // Convert ExternalInterface version of DisplayProceduralFace to RobotInterface version and send
  _robot->SendRobotMessage<RobotInterface::DisplayProceduralFace>(msg.faceParams, msg.duration_ms);
}

template<>
void AnimationComponent::HandleMessage(const ExternalInterface::SetFaceHue& msg)
{
  _robot->SendRobotMessage<RobotInterface::SetFaceHue>(msg.hue);
}

template<>
void AnimationComponent::HandleMessage(const ExternalInterface::DisplayFaceImageBinaryChunk& msg)
{
  if (!_isInitialized) {
    LOG_WARNING("AnimationComponent.HandleDisplayFaceImageBinaryChunk.Uninitialized", "");
    return;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !msg.interruptRunning) {
    LOG_INFO("AnimationComponent.HandleDisplayFaceImage.WontInterruptCurrentAnim", "");
    return;
  }

  // Convert ExternalInterface version of DisplayFaceImage to RobotInterface version and send
  _robot->SendRobotMessage<RobotInterface::DisplayFaceImageBinaryChunk>(msg.duration_ms, msg.faceData, msg.imageId, msg.chunkIndex);
}

template<>
void AnimationComponent::HandleMessage(const ExternalInterface::DisplayFaceImageRGBChunk& msg)
{
  if (!_isInitialized) {
    LOG_WARNING("AnimationComponent.DisplayFaceImageRGB.Uninitialized", "");
    return;
  }

  _oledImageBuilder->AddDataChunk(msg.faceData, msg.chunkIndex, msg.numPixels);

  uint32_t fullMask = 0;
  for( int i=0; i<msg.numChunks; ++i )
  {
    fullMask |= (1L << i);
  }

  // did we recieve every chunk we need?
  if( (_oledImageBuilder->GetRecievedChunkMask() ^ fullMask) == 0 )
  {
    Vision::ImageRGB565 image(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH, _oledImageBuilder->GetAllData());
    DisplayFaceImage(image, msg.duration_ms, msg.interruptRunning);

    _oledImageBuilder->Clear();
  }
}

template<>
void AnimationComponent::HandleMessage(const ExternalInterface::ReadAnimationFile& msg)
{
  _robot->SendRobotMessage<RobotInterface::AddAnim>(msg.full_path);
}

// ================ Robot message handlers ======================

void AnimationComponent::HandleAnimAdded(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animAdded();
  LOG_INFO("HandleAnimAdded", "name=%s length=%d", payload.animName.c_str(), payload.animLength);
  _availableAnims[payload.animName].length_ms = payload.animLength;
}

void AnimationComponent::HandleAnimStarted(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animStarted();

  // the trigger word get-in is started in the anim process without our knowledge, so if we see it being started,
  // let's not complain about it since it's expected
  // note: we could have a "started" callback for this similar to _triggerWordGetInCallbackFunction
  //       this way UserIntentComponent knows exactly when the trigger word anim starts instead of just assuming
  const bool isTriggerWordGetIn = (payload.tag == _tagForTriggerWordGetInCallbacks);
  // same thing for alexa
  const bool isAlexa = TagIsAlexa( payload.tag );

  auto it = _callbackMap.find(payload.tag);
  if (it != _callbackMap.end() || isTriggerWordGetIn || isAlexa) {
    // LOG_INFO("AnimStarted.Tag", "name=%s, tag=%d", payload.animName.c_str(), payload.tag);
  } else if (payload.animName != EnumToString(AnimConstants::PROCEDURAL_ANIM)) {
    LOG_WARNING("AnimationComponent.AnimStarted.UnexpectedTag", "name=%s, tag=%d", payload.animName.c_str(), payload.tag);
    return;
  }

  _isAnimating = true;
  _currAnimName = payload.animName;
  _currAnimTag = payload.tag;
  
  if( payload.tag == _tagForTriggerWordGetInCallbacks ){
    const bool playing = true;
    _triggerWordGetInCallbackFunction(playing);
  } else if( TagIsAlexa(payload.tag) ) {
    const bool playing = true;
    SendAlexaCallback(payload.tag, playing);
  }

  _robot->GetContext()->GetVizManager()->SendCurrentAnimation(_currAnimName, _currAnimTag);
}

void AnimationComponent::HandleAnimEnded(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animEnded();

  _movementComponent->ApplyUpdatesForStreamTime(payload.streamTimeAnimEnded);
  
  // Verify that expected animation completed and execute callback
  bool atLeastOneCallback = false;
  auto it = _callbackMap.find(payload.tag);
  while(it != _callbackMap.end()){
    atLeastOneCallback = true;
    // LOG_INFO("AnimEnded.Tag", "name=%s, tag=%d", payload.animName.c_str(), payload.tag);
    it->second.ExecuteCallback(payload.wasAborted ? AnimResult::Aborted : AnimResult::Completed,
                               payload.streamTimeAnimEnded);
    _callbackMap.erase(it);
    it = _callbackMap.find(payload.tag);
  }

  // Special callback for the trigger word response that persists
  if(payload.tag == _tagForTriggerWordGetInCallbacks){
    // this wont be in our _callbackMap, so for debug's sake let's print this out
    // LOG_INFO("AnimEnded.Tag", "name=%s, tag=%d", payload.animName.c_str(), payload.tag);
    atLeastOneCallback = true;
    const bool playing = false;
    _triggerWordGetInCallbackFunction(playing);
  } else if( TagIsAlexa(payload.tag) ) {
    atLeastOneCallback = true;
    const bool playing = false;
    SendAlexaCallback(payload.tag, playing);
  }
    
  if (!atLeastOneCallback &&
      (payload.animName != EnumToString(AnimConstants::PROCEDURAL_ANIM))) {
    LOG_WARNING("AnimationComponent.AnimEnded.UnexpectedTag", "name=%s, tag=%d", payload.animName.c_str(), payload.tag);
    return;
  }

  _isAnimating = false;

  DEV_ASSERT_MSG(_currAnimName.empty() || _currAnimName == payload.animName, "AnimationComponent.AnimEnded.UnexpectedName", "Got %s, expected %s", payload.animName.c_str(), _currAnimName.c_str());
  DEV_ASSERT_MSG(_currAnimTag == kNotAnimatingTag || _currAnimTag == payload.tag, "AnimationComponent.AnimEnded.UnexpectedTag", "Got %d, expected %d", payload.tag, _currAnimTag);

  _currAnimName = "";
  _currAnimTag = kNotAnimatingTag;

  _robot->GetContext()->GetVizManager()->SendCurrentAnimation(_currAnimName, _currAnimTag);
}
  
void AnimationComponent::HandleAnimationEvent(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animEvent();
  auto it = _callbackMap.find(payload.tag);
  if (it != _callbackMap.end()) {
    LOG_INFO("HandleAnimationEvent", "%s", EnumToString(payload.event_id));
    ExternalInterface::AnimationEvent msg;
    msg.timestamp = payload.timestamp; // AnimTimeStamp_t
    msg.event_id = payload.event_id;
    _robot->GetExternalInterface()->BroadcastToGame<ExternalInterface::AnimationEvent>(std::move(msg));
  }
}
  
void AnimationComponent::HandleAnimState(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  _animState = message.GetData().Get_animState();
}

void AnimationComponent::AddKeepFaceAliveFocus(const std::string& name)
{
  if (_focusRequests.empty())
  {
    _robot->SendRobotMessage<RobotInterface::SetKeepFaceAliveFocus>(true);
  }
  _focusRequests.insert(name);
}

void AnimationComponent::RemoveKeepFaceAliveFocus(const std::string& name)
{
  _focusRequests.erase(name);
  if (_focusRequests.empty())
  {
    _robot->SendRobotMessage<RobotInterface::SetKeepFaceAliveFocus>(false);
  }
}

void AnimationComponent::AddAdditionalAnimationCallback(const std::string& name,
                                                        AnimationComponent::AnimationCompleteCallback callback,
                                                        bool callEvenIfAnimCanceled)
{
  // If we should only call the callback while the anim is active we need to look
  // up the action tag, otherwise just leave it as zero
  u32 actionTag = 0;
  u32 currTag = 0;
  for(const auto& entry: _callbackMap){
    if(entry.second.animName == name){
      currTag = entry.first;
      actionTag = entry.second.actionTag;
      break;
    }
  }
  
  if(actionTag == 0){
    LOG_ERROR("AnimationComponent.AddAdditionalAnimationCallback.NoActionTagFound",
              "Attempting to add an additional callback to animation %s, but it was not found in the callback map",
              name.c_str());
    return;
  }

  // Timeouts will be handled by the existing callback
  const u32 numLoops = 0;
  const float timeout_sec = 0;
  SetAnimationCallback(name, callback, currTag, actionTag, numLoops, timeout_sec, callEvenIfAnimCanceled);
}
  
AnimationComponent::Tag AnimationComponent::GetInvalidTag()
{
  return kInvalidAnimationTag;
}
  
bool AnimationComponent::TagIsAlexa( AnimationTag tag ) const
{
  const bool isAlexa = (tag == _tagForAlexaListening)
                       || (tag == _tagForAlexaThinking)
                       || (tag == _tagForAlexaSpeaking)
                       || (tag == _tagForAlexaError);
  return isAlexa;
}
  
void AnimationComponent::SendAlexaCallback( uint8_t tag, bool playing ) const
{
  if( _alexaResponseCallback ) {
    // must match order in clad file
    if( tag == _tagForAlexaListening ) {
      _alexaResponseCallback( 0, playing );
    } else if( tag == _tagForAlexaThinking ) {
      _alexaResponseCallback( 1, playing );
    } else if( tag == _tagForAlexaSpeaking ) {
      _alexaResponseCallback( 2, playing );
    } else if( tag == _tagForAlexaError ) {
      _alexaResponseCallback( 3, playing );
    }
  }
}


} // namespace Vector
} // namespace Anki
