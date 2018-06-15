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
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/robotManager.h"
#include "engine/robotInterface/messageHandler.h"

#include "clad/types/animationTypes.h"

#include "cannedAnimLib/cannedAnims/animation.h"
#include "cannedAnimLib/cannedAnims/cannedAnimationContainer.h"
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"
#include "coretech/vision/shared/compositeImage/compositeImage.h"
#include "coretech/vision/shared/rgb565Image/rgb565ImageBuilder.h"

#include "json/json.h"

namespace Anki {
namespace Cozmo {
 
namespace {
  static const char* kLogChannelName = "Animations";

  const u32 kMaxNumAvailableAnimsToReportPerTic = 50;

  static const u32 kNumImagePixels     = FACE_DISPLAY_HEIGHT * FACE_DISPLAY_WIDTH;
  static const u32 kNumHalfImagePixels = kNumImagePixels / 2;
}
  
  
AnimationComponent::AnimationComponent()
: IDependencyManagedComponent(this, RobotComponentID::Animation)
, _isInitialized(false)
, _tagCtr(0)
, _isDolingAnims(false)
, _nextAnimToDole("")
, _currPlayingAnim("")
, _lockedTracks(0)
, _isAnimating(false)
, _currAnimName("")
, _currAnimTag(0)
, _oledImageBuilder(new Vision::RGB565ImageBuilder)
, _compositeImageID(0)
{

}

void AnimationComponent::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  _dataAccessor = dependentComponents.GetBasePtr<DataAccessorComponent>();
  const CozmoContext* context = _robot->GetContext();
  _animationGroups = std::make_unique<AnimationGroupWrapper>(*(context->GetDataLoader()->GetAnimationGroups()));
  if (context) {
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
      helper.SubscribeGameToEngine<MessageGameToEngineTag::EnableKeepFaceAlive>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::SetKeepFaceAliveParameters>();
      helper.SubscribeGameToEngine<MessageGameToEngineTag::ReadAnimationFile>();

    }
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
  // Open manifest file
  static const std::string manifestFile = "assets/anim_manifest.json";
  Json::Value jsonManifest;
  const bool success = _robot->GetContext()->GetDataPlatform()->readAsJson(Util::Data::Scope::Resources, manifestFile, jsonManifest);
  if (!success) {
    PRINT_NAMED_ERROR("AnimationComponent.Init.ManifestNotFound", "");
    return;
  }

  // Process animations in manifest
  _availableAnims.clear();
  for (int i=0; i<jsonManifest.size(); ++i) {
    const Json::Value& jsonAnim = jsonManifest[i];
    
    static const char* kNameField = "name";
    static const char* kLengthField = "length_ms";

    if (!jsonAnim.isMember(kNameField)) {
      PRINT_NAMED_ERROR("AnimationComponent.Init.MissingJsonField", "%s", kNameField);
      continue;
    }

    if (!jsonAnim.isMember(kLengthField)) {
      PRINT_NAMED_ERROR("AnimationComponent.Init.MissingJsonField", "%s", kLengthField);
      continue;
    }

    _availableAnims[jsonAnim[kNameField].asCString()].length_ms = jsonAnim[kLengthField].asInt();
  }
  PRINT_CH_INFO(kLogChannelName, "AnimationComponent.Init.ManifestRead", "%zu animations loaded", _availableAnims.size());

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
      PRINT_NAMED_WARNING("AnimationComponent.Update.AnimTimedOut", "Anim: %s", it->second.animName.c_str());
      _robot->SendRobotMessage<RobotInterface::AbortAnimation>();
      it->second.ExecuteCallback(AnimResult::Timedout);
      it = _callbackMap.erase(it);
    }
    else {
      ++it;
    }
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
      PRINT_CH_INFO(kLogChannelName, "DoleAvailableAnimations.Done", "");
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


const std::string& AnimationComponent::GetAnimationNameFromGroup(const std::string& name, bool strictCooldown) const
{
  const AnimationGroup* group = _animationGroups->_container.GetAnimationGroup(name);
  if(group != nullptr && !group->IsEmpty()) {
    return group->GetAnimationName(_robot->GetMoodManager(),  _animationGroups->_container, 
                                   _robot->GetComponent<FullRobotPose>().GetHeadAngle(), strictCooldown);
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
  auto it = _availableAnims.find(animName);
  if (it == _availableAnims.end()) {
    PRINT_NAMED_WARNING("AnimationComponent.PlayAnimByName.AnimNotFound", "%s", animName.c_str());
    return RESULT_FAIL;
  }
  
  PRINT_CH_DEBUG(kLogChannelName, "AnimationComponent.PlayAnimByName.PlayingAnim", "%s", it->first.c_str());

  // Check that a valid actionTag was specified if there is non-empty callback
  if (callback != nullptr && actionTag == 0) {
    PRINT_NAMED_WARNING("AnimationComponent.PlayAnimByName.MissingActionTag", "");
    return RESULT_FAIL;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    PRINT_CH_INFO(kLogChannelName, "AnimationComponent.PlayAnimByName.WontInterruptCurrentAnim", "");
    return RESULT_FAIL;
  }

  const Tag currTag = GetNextTag();
  if (_robot->SendRobotMessage<RobotInterface::PlayAnim>(numLoops, currTag, animName) == RESULT_OK) {
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
    PRINT_NAMED_WARNING("AnimationComponent.PlayCompositeAnimation.Uninitialized", "");
    return RESULT_FAIL;
  }

  // Check that animName is valid
  auto it = _availableAnims.find(animName);
  if (it == _availableAnims.end()) {
    PRINT_NAMED_WARNING("AnimationComponent.PlayCompositeAnimation.AnimNotFound", "%s", animName.c_str());
    return RESULT_FAIL;
  }
  
  PRINT_CH_DEBUG(kLogChannelName, "AnimationComponent.PlayCompositeAnimation.PlayingAnim", "%s", it->first.c_str());
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    PRINT_CH_INFO(kLogChannelName, "AnimationComponent.PlayCompositeAnimation.WontInterruptCurrentAnim", "");
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
    PRINT_NAMED_WARNING("AnimationComponent.PlayCompositeAnimation.AnimationNotFoundInContainer", 
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
      PRINT_CH_DEBUG(kLogChannelName, "AnimationComponent.StopAnimByName.AbortingAnim", "%s", animName.c_str());
      return _robot->SendRobotMessage<RobotInterface::AbortAnimation>();
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
  _robot->SendRobotMessage<RobotInterface::LockAnimTracks>(_lockedTracks);
}

void AnimationComponent::UnlockAllTracks()
{
  if (_lockedTracks != 0) {
    _lockedTracks = 0;
    _robot->SendRobotMessage<RobotInterface::LockAnimTracks>(_lockedTracks);
  }
}

// Disables only the specified tracks. 
// Status of other tracks remain unchanged.
void AnimationComponent::LockTracks(u8 tracks)
{
  _lockedTracks |= tracks;
  _robot->SendRobotMessage<RobotInterface::LockAnimTracks>(_lockedTracks);
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
    PRINT_CH_INFO(kLogChannelName, "AnimationComponent.DisplayFaceImageBinary.WontInterruptCurrentAnim", "");
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
    PRINT_NAMED_WARNING("AnimationComponent.DisplayFaceImage.Uninitialized", "");
    return RESULT_FAIL;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    PRINT_CH_INFO(kLogChannelName, "AnimationComponent.DisplayFaceImage.WontInterruptCurrentAnim", "");
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
                                              float timeout_sec)
{
  // Check if tag already exists in callback map.
  // If so, trigger callback with Stale
  {
    auto it = _callbackMap.find(currTag);
    if (it != _callbackMap.end()) {
      PRINT_NAMED_WARNING("AnimationComponent.SetAnimationCallback.StaleTag", "%d", currTag);
      it->second.ExecuteCallback(AnimResult::Stale);
      _callbackMap.erase(it);
    }
  }
  const float abortTime_sec = (numLoops > 0 ? BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + timeout_sec : 0);
  _callbackMap.emplace(std::piecewise_construct,
                        std::forward_as_tuple(currTag),
                        std::forward_as_tuple(animName, callback, actionTag, abortTime_sec));
}


Result AnimationComponent::DisplayFaceImage(const Vision::CompositeImage& compositeImage, 
                                            u32 frameInterval_ms, u32 duration_ms, 
                                            bool interruptRunning, bool emptySpriteBoxesAreValid)
{
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !interruptRunning) {
    PRINT_CH_INFO(kLogChannelName, "AnimationComponent.DisplayFaceImage.WontInterruptCurrentAnim", "");
    return RESULT_FAIL;
  }

  // Send the image to the animation process in chunks
  const std::vector<Vision::CompositeImageChunk> imageChunks = compositeImage.GetImageChunks(emptySpriteBoxesAreValid);
  for(const auto& chunk: imageChunks){
    _robot->SendRobotMessage<RobotInterface::DisplayCompositeImageChunk>(_compositeImageID, frameInterval_ms, duration_ms, chunk);
  }
  
  if(imageChunks.empty()){
    PRINT_NAMED_WARNING("AnimationComponent.DisplayFaceImage.NoImageChunksToSend", "");
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
                                                                     layerName, 
                                                                     entry.GetSpriteName());
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
                                                                 layerName, 
                                                                 entry.GetSpriteName());
}

Result AnimationComponent::EnableKeepFaceAlive(bool enable, u32 disableTimeout_ms) const
{
  return _robot->SendRobotMessage<RobotInterface::EnableKeepFaceAlive>(disableTimeout_ms, enable);
}

Result AnimationComponent::SetDefaultKeepFaceAliveParameters() const
{
  return _robot->SendRobotMessage<RobotInterface::SetDefaultKeepFaceAliveParameters>();
}

Result AnimationComponent::SetKeepFaceAliveParameter(KeepFaceAliveParameter param, f32 value) const
{
  return _robot->SendRobotMessage<RobotInterface::SetKeepFaceAliveParameter>(value, param, false);
}
  
Result AnimationComponent::SetKeepFaceAliveParameterToDefault(KeepFaceAliveParameter param) const
{
  return _robot->SendRobotMessage<RobotInterface::SetKeepFaceAliveParameter>(0.f, param, true);
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

// ================ Game message handlers ======================
template<>
void AnimationComponent::HandleMessage(const ExternalInterface::RequestAvailableAnimations& msg)
{
  PRINT_CH_INFO("AnimationComponent", "RequestAvailableAnimations.Recvd", "");
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
    PRINT_NAMED_WARNING("AnimationComponent.DisplayProceduralFace.Uninitialized", "");
    return;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !msg.interruptRunning) {
    PRINT_CH_INFO(kLogChannelName, "AnimationComponent.DisplayProceduralFace.WontInterruptCurrentAnim", "");
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
    PRINT_NAMED_WARNING("AnimationComponent.HandleDisplayFaceImageBinaryChunk.Uninitialized", "");
    return;
  }
  
  // TODO: Is this what interruptRunning should mean?
  //       Or should it queue on anim process side and optionally interrupt currently executing anim?
  if (IsPlayingAnimation() && !msg.interruptRunning) {
    PRINT_CH_INFO(kLogChannelName, "AnimationComponent.HandleDisplayFaceImage.WontInterruptCurrentAnim", "");
    return;
  }

  // Convert ExternalInterface version of DisplayFaceImage to RobotInterface version and send
  _robot->SendRobotMessage<RobotInterface::DisplayFaceImageBinaryChunk>(msg.duration_ms, msg.faceData, msg.imageId, msg.chunkIndex);
}

template<>
void AnimationComponent::HandleMessage(const ExternalInterface::DisplayFaceImageRGBChunk& msg)
{
  if (!_isInitialized) {
    PRINT_NAMED_WARNING("AnimationComponent.DisplayFaceImageRGB.Uninitialized", "");
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
void AnimationComponent::HandleMessage(const ExternalInterface::EnableKeepFaceAlive& msg)
{
  EnableKeepFaceAlive(msg.enable, msg.disableTimeout_ms);
}

template<>
void AnimationComponent::HandleMessage(const ExternalInterface::SetKeepFaceAliveParameters& msg)
{
  if (msg.setUnspecifiedToDefault) {
    SetDefaultKeepFaceAliveParameters();
  }
  
  if(ANKI_VERIFY(msg.paramNames.size() == msg.paramValues.size(), "AnimationComponent.HandleSetKeepFaceAliveParameters.NameValuePairMismatch", ""))
  {
    for (int i=0; i<msg.paramNames.size(); ++i) {
      SetKeepFaceAliveParameter( msg.paramNames.at(i), msg.paramValues.at(i) );
    }
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
  PRINT_CH_INFO("AnimationComponent", "HandleAnimAdded", "name=%s length=%d", payload.animName.c_str(), payload.animLength);
  _availableAnims[payload.animName].length_ms = payload.animLength;
}

void AnimationComponent::HandleAnimStarted(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animStarted();
  auto it = _callbackMap.find(payload.tag);
  if (it != _callbackMap.end()) {
    PRINT_CH_INFO("AnimationComponent", "AnimStarted.Tag", "name=%s, tag=%d", payload.animName.c_str(), payload.tag);
  } else if (payload.animName != EnumToString(AnimConstants::PROCEDURAL_ANIM) ) {
    PRINT_NAMED_WARNING("AnimationComponent.AnimStarted.UnexpectedTag", "name=%s, tag=%d", payload.animName.c_str(), payload.tag);
    return;
  }

  _isAnimating = true;
  _currAnimName = payload.animName;
  _currAnimTag = payload.tag;

  _robot->GetContext()->GetVizManager()->SendCurrentAnimation(_currAnimName, _currAnimTag);
}

void AnimationComponent::HandleAnimEnded(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  const auto & payload = message.GetData().Get_animEnded();
  
  // Verify that expected animation completed and execute callback
  auto it = _callbackMap.find(payload.tag);
  if (it != _callbackMap.end()) {
    PRINT_CH_INFO("AnimationComponent", "AnimEnded.Tag", "name=%s, tag=%d", payload.animName.c_str(), payload.tag);
    it->second.ExecuteCallback(payload.wasAborted ? AnimResult::Aborted : AnimResult::Completed);
    _callbackMap.erase(it);
  } else if (payload.animName != EnumToString(AnimConstants::PROCEDURAL_ANIM) ) {
    PRINT_NAMED_WARNING("AnimationComponent.AnimEnded.UnexpectedTag", "name=%s, tag=%d", payload.animName.c_str(), payload.tag);
    return;
  }

  _isAnimating = false;
  DEV_ASSERT_MSG(_currAnimName == payload.animName, "AnimationComponent.AnimEnded.UnexpectedName", "Got %s, expected %s", payload.animName.c_str(), _currAnimName.c_str());
  DEV_ASSERT_MSG(_currAnimTag == payload.tag, "AnimationComponent.AnimEnded.UnexpectedTag", "Got %d, expected %d", payload.tag, _currAnimTag);

  _currAnimName = "";
  _currAnimTag = kNotAnimatingTag;

  _robot->GetContext()->GetVizManager()->SendCurrentAnimation(_currAnimName, _currAnimTag);
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
    _robot->GetExternalInterface()->BroadcastToGame<ExternalInterface::AnimationEvent>(std::move(msg));
  }
}
  
void AnimationComponent::HandleAnimState(const AnkiEvent<RobotInterface::RobotToEngine>& message)
{
  _animState = message.GetData().Get_animState();
}
  
  
} // namespace Cozmo
} // namespace Anki
