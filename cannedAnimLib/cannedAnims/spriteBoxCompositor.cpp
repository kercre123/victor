/**
 * File: spriteBoxCompositor.cpp
 *
 * Authors: Sam Russell
 * Created: 2019-03-01
 *
 * Description:
 *    Class for storing SpriteBoxKeyFrames and constructing CompositeImages from them
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "cannedAnimLib/cannedAnims/spriteBoxCompositor.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "cannedAnimLib/baseTypes/cozmo_anim_generated.h"
#include "coretech/common/engine/jsonTools.h"
#include "coretech/vision/shared/compositeImage/compositeImage.h"
#include "coretech/vision/shared/spriteCache/spriteCache.h"
#include "coretech/vision/shared/spriteSequence/spriteSequenceContainer.h"

#define LOG_CHANNEL "Animations"

namespace Anki{
namespace Vector{
namespace Animations{

namespace{
const char* kClearSpriteBox  = "clear_sprite_box";
const char* kEmptySpriteBox  = "empty_sprite_box";

const char* kSpriteBoxNameKey = "spriteBoxName";
const char* kTriggerTimeKey   = "triggerTime_ms";
const char* kAssetNameKey     = "assetName";
const char* kLayerNameKey     = "layer";
const char* kRenderMethodKey  = "renderMethod";
const char* kSpriteSeqEndKey  = "spriteSeqEndType";
const char* kAlphaKey         = "alpha";
const char* kXPosKey          = "xPos";
const char* kYPosKey          = "yPos";
const char* kWidthKey         = "width";
const char* kHeightKey        = "height";

// Fwd declare local helper
bool GetFrameFromSpriteSequenceHelper(const Vision::SpriteSequence& sequence,
                                      const uint16_t frameIdx,
                                      const Vision::SpriteSeqEndType& spriteSeqEndType,
                                      Vision::SpriteHandle& outSpriteHandle);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SpriteBoxKeyFrame
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteBoxKeyFrame::SpriteBoxKeyFrame()
: assetName(kClearSpriteBox)
, triggerTime_ms(0)
, alpha(100.0f)
, xPos(0)
, yPos(0)
, width(0)
, height(0)
, layer(Vision::LayerName::Layer_1)
, renderMethod(Vision::SpriteRenderMethod::RGBA)
, spriteSeqEndType(Vision::SpriteSeqEndType::Hold)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteBoxKeyFrame::SpriteBoxKeyFrame(const CozmoAnim::SpriteBox* spriteBox)
: assetName(spriteBox->assetName()->str())
, triggerTime_ms((u32)spriteBox->triggerTime_ms())
, alpha(spriteBox->alpha())
, xPos(spriteBox->xPos())
, yPos(spriteBox->yPos())
, width(spriteBox->width())
, height(spriteBox->height())
, layer(Vision::LayerNameFromString(spriteBox->layer()->str()))
, renderMethod(Vision::SpriteRenderMethodFromString(spriteBox->renderMethod()->str()))
, spriteSeqEndType(Vision::SpriteSeqEndTypeFromString(spriteBox->spriteSeqEndType()->str()))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteBoxKeyFrame::SpriteBoxKeyFrame(const Json::Value& json, const std::string& animName)
: assetName(JsonTools::ParseString(json, kAssetNameKey, animName))
, triggerTime_ms(JsonTools::ParseInt32(json, kTriggerTimeKey, animName))
, alpha(JsonTools::ParseInt32(json, kAlphaKey, animName))
, xPos(JsonTools::ParseInt32(json, kXPosKey, animName))
, yPos(JsonTools::ParseInt32(json, kYPosKey, animName))
, width(JsonTools::ParseInt32(json, kWidthKey, animName))
, height(JsonTools::ParseInt32(json, kHeightKey, animName))
, layer(Vision::LayerNameFromString(JsonTools::ParseString(json, kLayerNameKey, animName)))
, renderMethod(Vision::SpriteRenderMethodFromString(JsonTools::ParseString(json, kRenderMethodKey, animName)))
, spriteSeqEndType(Vision::SpriteSeqEndTypeFromString(JsonTools::ParseString(json, kSpriteSeqEndKey, animName)))
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SpriteBoxCompositor
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteBoxCompositor::SpriteBoxCompositor(const SpriteBoxCompositor& other)
: _lastKeyFrameTime_ms(other._lastKeyFrameTime_ms)
, _advanceTime_ms(0)
{
  if(nullptr != other._spriteBoxMap){
    _spriteBoxMap = std::make_unique<SpriteBoxMap>(*other._spriteBoxMap);
  } else {
    _spriteBoxMap.reset();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteBoxCompositor::SpriteBoxCompositor()
: _lastKeyFrameTime_ms(0)
, _advanceTime_ms(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result SpriteBoxCompositor::AddKeyFrame(const CozmoAnim::SpriteBox* spriteBox)
{
  const std::string& spriteBoxName(spriteBox->spriteBoxName()->str());
  SpriteBoxKeyFrame newKeyFrame(spriteBox);
  return AddKeyFrameInternal(spriteBoxName, std::move(newKeyFrame));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result SpriteBoxCompositor::AddKeyFrame(const Json::Value& json, const std::string& animName)
{
  const std::string spriteBoxName = JsonTools::ParseString(json, kSpriteBoxNameKey, animName);
  SpriteBoxKeyFrame newKeyFrame(json, animName);
  return AddKeyFrameInternal(spriteBoxName, std::move(newKeyFrame));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result SpriteBoxCompositor::AddKeyFrameInternal(const std::string& spriteBoxName, SpriteBoxKeyFrame&& spriteBox)
{
  if(nullptr == _spriteBoxMap){
    _spriteBoxMap = std::make_unique<SpriteBoxMap>();
  }

  // Grab a copy to simplify the _lastKeyFrameTime_ms check since insertion operations
  // will invoke std::move
  const TimeStamp_t triggerTime_ms = spriteBox.triggerTime_ms;

  auto iter = _spriteBoxMap->find(spriteBoxName);
  if(_spriteBoxMap->end() == iter){
    // This is the first keyframe for this SpriteBoxName. Add a new track.
    SpriteBoxTrack newTrack;
    newTrack.InsertKeyFrame(std::move(spriteBox));
    _spriteBoxMap->insert({spriteBoxName, newTrack});
  } else {
    auto& track = iter->second;
    if(!track.InsertKeyFrame(std::move(spriteBox))){
      LOG_ERROR("SpriteBoxCompositor.AddKeyFrame.DuplicateKeyFrame",
                "Attempted to add overlapping keyframe for SpriteBoxName: %s at time: %d ms",
                spriteBoxName.c_str(),
                triggerTime_ms);
      return Result::RESULT_FAIL;
    }
  }

  if(triggerTime_ms > _lastKeyFrameTime_ms){
    _lastKeyFrameTime_ms = triggerTime_ms;
  }

  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteBoxCompositor::AddSpriteBoxRemap(const Vision::SpriteBoxName spriteBox, const std::string& remappedAssetName)
{
  if(kClearSpriteBox == remappedAssetName){
    LOG_WARNING("SpriteBoxCompositor.AddSpriteBoxRemap.InvalidRemap",
                "kClearSpriteBox is not valid from engine, use kEmptySpriteBox to override an SB to display nothing");
    return;
  }

  const std::string& spriteBoxName = Vision::SpriteBoxNameToString(spriteBox);
  if(IsEmpty()){
    LOG_ERROR("SpriteBoxCompositor.AddSpriteBoxRemap.EmptyCompositor",
              "Attempted to add remap for SpriteBox %s with remapped asset name %s",
              spriteBoxName.c_str(),
              remappedAssetName.c_str());
    return;
  }

  auto iter = _spriteBoxMap->find(spriteBoxName);
  if(_spriteBoxMap->end() == iter){
    LOG_ERROR("SpriteBoxCompositor.AddSpriteBoxRemap.InvalidSpriteBox",
              "Attempted to add remap for invalid SpriteBox %s with remapped asset name %s",
              spriteBoxName.c_str(),
              remappedAssetName.c_str());
    return;
  }

  iter->second.SetAssetRemap(remappedAssetName);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteBoxCompositor::CacheInternalSprites(Vision::SpriteCache* spriteCache)
{
  // TODO: Implement this if we try to enable pre-caching
  LOG_WARNING("SpriteBoxCompositor.CacheInternalSprites.CachingNotSupported",
              "Caching of internal sprites from the SpriteBoxCompositor is currently unsupported");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteBoxCompositor::AppendTracks(const SpriteBoxCompositor& other, const TimeStamp_t animOffset_ms)
{
  if(nullptr != other._spriteBoxMap){
    for(const auto& spriteBoxTrackIter : *other._spriteBoxMap){
      for(const auto& spriteBoxKeyFrame : spriteBoxTrackIter.second.GetKeyFrames()){
        SpriteBoxKeyFrame newKeyFrame = spriteBoxKeyFrame;
        newKeyFrame.triggerTime_ms += animOffset_ms;
        AddKeyFrameInternal(spriteBoxTrackIter.first, std::move(newKeyFrame));
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteBoxCompositor::Clear()
{
  if(nullptr != _spriteBoxMap){
    _spriteBoxMap->clear();
    _spriteBoxMap.reset();
  }
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpriteBoxCompositor::IsEmpty() const
{
  return ( (nullptr == _spriteBoxMap) || _spriteBoxMap->empty());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteBoxCompositor::AdvanceTrack(const TimeStamp_t& toTime_ms)
{
  _advanceTime_ms = toTime_ms;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteBoxCompositor::MoveToStart()
{
  _advanceTime_ms = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteBoxCompositor::ClearUpToCurrent()
{
  if(!IsEmpty()){
    auto spriteBoxTrackIter = _spriteBoxMap->begin();
    while(_spriteBoxMap->end() != spriteBoxTrackIter){
      auto& track = spriteBoxTrackIter->second;
      track.ClearUpToTime(_advanceTime_ms);
      if(track.IsEmpty()){
        spriteBoxTrackIter = _spriteBoxMap->erase(spriteBoxTrackIter);
      } else {
        ++spriteBoxTrackIter;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpriteBoxCompositor::HasFramesLeft() const
{
  // TODO(str): VIC-13519 Linearize Face Rendering
  // Keep tabs on this when working in AnimationStreamer. It currently has to use '<=' in
  // order to have the final frame displayed due to calling locations in AnimationStreamer,
  // but that feels odd since it ends up requiring that we run one frame beyond the last
  // frame of the animation before AnimationStreamer cleans up the animation.
  return (_advanceTime_ms <= _lastKeyFrameTime_ms);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TimeStamp_t SpriteBoxCompositor::CompareLastFrameTime(const TimeStamp_t lastFrameTime_ms) const
{
  return (lastFrameTime_ms > _lastKeyFrameTime_ms) ? lastFrameTime_ms : _lastKeyFrameTime_ms;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpriteBoxCompositor::PopulateCompositeImage(Vision::SpriteCache& spriteCache,
                                                 Vision::SpriteSequenceContainer& spriteSeqContainer,
                                                 TimeStamp_t timeSinceAnimStart_ms,
                                                 Vision::CompositeImage& outCompImg) const
{
  if(IsEmpty()){
    return false;
  }

  bool addedImage = false;
  for(auto& trackPair : *_spriteBoxMap){
    auto& track = trackPair.second;

    SpriteBoxKeyFrame currentKeyFrame;
    if(!track.GetCurrentKeyFrame(timeSinceAnimStart_ms, currentKeyFrame)){
      // Nothing to render for this track. Skip to the next
      continue;
    }

    // Get a SpriteHandle to the image we want to display
    Vision::SpriteHandle spriteHandle;
    if(spriteSeqContainer.IsValidSpriteSequenceName(currentKeyFrame.assetName)){
      const uint16_t frameIdx = (timeSinceAnimStart_ms - currentKeyFrame.triggerTime_ms) / ANIM_TIME_STEP_MS;
      const Vision::SpriteSequence& sequence = *spriteSeqContainer.GetSpriteSequence(currentKeyFrame.assetName);
      if(!GetFrameFromSpriteSequenceHelper(sequence, frameIdx, currentKeyFrame.spriteSeqEndType, spriteHandle)){
        // The spriteSequence has nothing to draw. Skip to the next track
        continue;
      }
    } else {
      spriteHandle = spriteCache.GetSpriteHandleForNamedSprite(currentKeyFrame.assetName);
    }

    const Vision::SpriteBoxName currentSpriteBoxName = Vision::SpriteBoxNameFromString(trackPair.first);
    outCompImg.AddImage(spriteHandle, currentSpriteBoxName, currentKeyFrame.layer, currentKeyFrame.renderMethod,
                        currentKeyFrame.xPos, currentKeyFrame.yPos, currentKeyFrame.width, currentKeyFrame.height);
    addedImage = true;
  }

  return addedImage;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Local Helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace{
bool GetFrameFromSpriteSequenceHelper(const Vision::SpriteSequence& sequence,
                                      const uint16_t absFrameIdx,
                                      const Vision::SpriteSeqEndType& spriteSeqEndType,
                                      Vision::SpriteHandle& outSpriteHandle)
{
  uint16_t relFrameIdx = absFrameIdx;
  switch(spriteSeqEndType){
    case Vision::SpriteSeqEndType::Loop:
    {
      relFrameIdx = absFrameIdx % sequence.GetNumFrames();
      break;
    }
    case Vision::SpriteSeqEndType::Hold:
    {
      uint16_t maxIndex = sequence.GetNumFrames() - 1;
      relFrameIdx = (absFrameIdx > maxIndex) ? maxIndex : absFrameIdx;
      break;
    }
    case Vision::SpriteSeqEndType::Clear:
    {
      // Draw Nothing for this SpriteBox
      if(absFrameIdx >= sequence.GetNumFrames()){
        return false;
      }
    }
  }

  sequence.GetFrame(relFrameIdx, outSpriteHandle);
  return true;
}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SpriteBoxTrack
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteBoxTrack::SpriteBoxTrack()
: _lastAccessTime_ms(0)
, _firstKeyFrameTime_ms(std::numeric_limits<TimeStamp_t>::max())
, _lastKeyFrameTime_ms(0)
, _iteratorsAreValid(false)
, _remappedAssetName()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SpriteBoxTrack::SpriteBoxTrack(const SpriteBoxTrack& other)
:_track(other._track)
// last access should not copy
, _lastAccessTime_ms(0)
, _firstKeyFrameTime_ms(other._firstKeyFrameTime_ms)
, _lastKeyFrameTime_ms(other._lastKeyFrameTime_ms)
// Remaps and iterators do not copy
, _iteratorsAreValid(false)
, _remappedAssetName()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpriteBoxTrack::InsertKeyFrame(SpriteBoxKeyFrame&& spriteBox)
{
  _iteratorsAreValid = false;
  TimeStamp_t triggerTime_ms = spriteBox.triggerTime_ms;
  if(_track.insert(std::move(spriteBox)).second){
    if(_firstKeyFrameTime_ms > triggerTime_ms){
      _firstKeyFrameTime_ms = triggerTime_ms;
    }
    if (_lastKeyFrameTime_ms < triggerTime_ms){
      _lastKeyFrameTime_ms = triggerTime_ms;
    }
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SpriteBoxTrack::ClearUpToTime(const TimeStamp_t toTime_ms)
{
  if(_track.empty()){
    // Make sure we don't try to increment an end() iterator
    return;
  }

  // Find the current set of keyframes
  auto currentKeyFrameIter = _track.begin();
  auto nextKeyFrameIter = ++_track.begin();

  while( (_track.end() != nextKeyFrameIter) && (nextKeyFrameIter->triggerTime_ms <= toTime_ms) ){
    ++currentKeyFrameIter;
    ++nextKeyFrameIter;
  }

  bool modifiedTrack = false;
  if(_track.begin() != currentKeyFrameIter){
    currentKeyFrameIter = _track.erase(_track.begin(), currentKeyFrameIter);
    modifiedTrack = true;
    _firstKeyFrameTime_ms = _track.begin()->triggerTime_ms;
  }

  _iteratorsAreValid &= !modifiedTrack;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SpriteBoxTrack::GetCurrentKeyFrame(const TimeStamp_t timeSinceAnimStart_ms, SpriteBoxKeyFrame& outKeyFrame)
{
  if(_track.empty() || (timeSinceAnimStart_ms < _firstKeyFrameTime_ms) ){
    // Nothing to draw yet
    return false;
  }

  // Use the last access as a search start point (if appropriate) to optimize search for linear playback
  if(!_iteratorsAreValid || (timeSinceAnimStart_ms < _lastAccessTime_ms) ){
    // Rewind
    _currentKeyFrameIter = _track.begin();
    _nextKeyFrameIter = ++_track.begin();
    _iteratorsAreValid = true;
  }
  _lastAccessTime_ms = timeSinceAnimStart_ms;

  // Find the current set of keyframes
  while( (_track.end() != _nextKeyFrameIter) && (_nextKeyFrameIter->triggerTime_ms <= timeSinceAnimStart_ms) ){
    ++_currentKeyFrameIter;
    ++_nextKeyFrameIter;
  }
  outKeyFrame = *_currentKeyFrameIter;

  // Clear keyframes take precedence over remaps
  if(kClearSpriteBox == outKeyFrame.assetName){
    // Nothing to display
    return false;
  }

  if(!_remappedAssetName.empty()){
    outKeyFrame.assetName = _remappedAssetName;
  }

  // SpriteBoxes can be remapped to Empty from the engine
  if(kEmptySpriteBox == outKeyFrame.assetName){
    // Nothing to display
    return false;
  }

  if( (timeSinceAnimStart_ms == outKeyFrame.triggerTime_ms) ||
      (_track.end() == _nextKeyFrameIter) )
  {
    // No interpolation required/possible
    return true;
  }

  // Interpolate between keyframes as appropriate for timestamp
  const auto& currentKeyFrame = *_currentKeyFrameIter;
  const auto& nextKeyFrame = *_nextKeyFrameIter;

  const float interpRatio = ( (float)(timeSinceAnimStart_ms - currentKeyFrame.triggerTime_ms) /
                              (float)(nextKeyFrame.triggerTime_ms - currentKeyFrame.triggerTime_ms) );

  if(currentKeyFrame.alpha != nextKeyFrame.alpha){
    outKeyFrame.alpha = (1.0f - interpRatio) * currentKeyFrame.alpha + interpRatio * nextKeyFrame.alpha;
  }
  if(currentKeyFrame.xPos  != nextKeyFrame.xPos){
    outKeyFrame.xPos = (1.0f - interpRatio) * currentKeyFrame.xPos + interpRatio * nextKeyFrame.xPos;
  }
  if(currentKeyFrame.yPos  != nextKeyFrame.yPos){
    outKeyFrame.yPos = (1.0f - interpRatio) * currentKeyFrame.yPos + interpRatio * nextKeyFrame.yPos;
  }

  return true;
}

} // namespace Animations
} // namespace Vector
} // namespace Anki
