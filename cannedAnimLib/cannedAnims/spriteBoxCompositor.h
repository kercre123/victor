/**
 * File: spriteBoxCompositor.h
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

#ifndef __Anki_Vector_SpriteBoxCompositor_H__
#define __Anki_Vector_SpriteBoxCompositor_H__

#include "clad/types/compositeImageTypes.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/shared/types.h"

#include <set>
#include <unordered_map>

// Fwd Decl
namespace CozmoAnim {
  struct SpriteBox;
}

namespace Anki{

// Fwd Decl
namespace Vision{
  class CompositeImage;
  class SpriteCache;
  class SpriteSequenceContainer;
}

namespace Vector{
namespace Animations{

// Fwd Decl
struct SpriteBoxKeyFrame;
class SpriteBoxTrack;

class SpriteBoxCompositor
{
public:
  SpriteBoxCompositor();
  SpriteBoxCompositor(const SpriteBoxCompositor& other); // Copy ctor
  ~SpriteBoxCompositor() {}

  Result AddKeyFrame(const CozmoAnim::SpriteBox* spriteBoxKeyFrame);
  Result AddKeyFrame(const Json::Value& jsonRoot, const std::string& animName);
  Result AddKeyFrame(const std::string& spriteBoxName, Animations::SpriteBoxKeyFrame&& keyFrame) {
    return AddKeyFrameInternal(spriteBoxName, std::move(keyFrame));
  }

  void AddSpriteBoxRemap(const Vision::SpriteBoxName spriteBox, const std::string& remappedAssetName);

  void CacheInternalSprites(Vision::SpriteCache* spriteCache);

  void AppendTracks(const SpriteBoxCompositor& other, const TimeStamp_t animOffset_ms);
  void Clear();

  // Return true if there are no SpriteBoxKeyFrames in this compositor
  bool IsEmpty() const;

  // Sets the reference time for future calls to time-relative functions
  void AdvanceTrack(const TimeStamp_t& toTime_ms);
  void MoveToStart();

  void ClearUpToCurrent();
  bool HasFramesLeft() const;

  TimeStamp_t CompareLastFrameTime(const TimeStamp_t lastFrameTime_ms) const;

  bool PopulateCompositeImage(Vision::SpriteCache& spriteCache,
                              Vision::SpriteSequenceContainer& spriteSeqContainer,
                              TimeStamp_t timeSinceAnimStart_ms,
                              Vision::CompositeImage& outCompImg) const;

private:
  SpriteBoxCompositor(SpriteBoxCompositor&& other); // Move ctor
  SpriteBoxCompositor& operator=(SpriteBoxCompositor&& other); // Move assignment
  SpriteBoxCompositor& operator=(const SpriteBoxCompositor& other); // Copy assignment

  Result AddKeyFrameInternal(const std::string& spriteBoxName, SpriteBoxKeyFrame&& spriteBox);

  SpriteBoxKeyFrame InterpolateKeyFrames(const SpriteBoxKeyFrame& thisKeyFrame,
                                         const SpriteBoxKeyFrame& nextKeyFrame,
                                         TimeStamp_t timeSinceAnimStart_ms) const;

  TimeStamp_t _lastKeyFrameTime_ms;
  TimeStamp_t _advanceTime_ms;

  // Map from SpriteBoxName to set of keyframes ordered by triggerTime_ms
  using SpriteBoxMap = std::unordered_map<std::string, SpriteBoxTrack>;
  std::unique_ptr<SpriteBoxMap> _spriteBoxMap;

};

struct SpriteBoxKeyFrame
{
  SpriteBoxKeyFrame();
  SpriteBoxKeyFrame(const CozmoAnim::SpriteBox* spriteBox);
  SpriteBoxKeyFrame(const Json::Value& json, const std::string& animName);
  std::string assetName;
  TimeStamp_t triggerTime_ms;
  float alpha;
  int xPos;
  int yPos;
  int width;
  int height;
  Vision::LayerName layer;
  Vision::SpriteRenderMethod renderMethod;
  Vision::SpriteSeqEndType spriteSeqEndType;
  // Sort SpriteBoxKeyFrames by triggerTime_ms within a set. This also implies that
  // for a given SpriteBoxName, two KeyFrames are considered duplicates if they have
  // the same triggerTime_ms. Ergo, multiple keyframes for the same SBName and trigger
  // time are not allowed.
  bool operator<(const SpriteBoxKeyFrame& other) const{
    return triggerTime_ms < other.triggerTime_ms;
  }
};

class SpriteBoxTrack
{
  using TrackIterator = std::set<SpriteBoxKeyFrame>::iterator;
public:
  SpriteBoxTrack();
  SpriteBoxTrack(const SpriteBoxTrack& other);

  bool InsertKeyFrame(SpriteBoxKeyFrame&& spriteBox);
  bool IsEmpty() const { return _track.empty(); }
  void ClearUpToTime(const TimeStamp_t toTime_ms);

  bool GetCurrentKeyFrame(const TimeStamp_t timeSinceAnimStart_ms, SpriteBoxKeyFrame& outKeyFrame);
  const std::set<SpriteBoxKeyFrame>& GetKeyFrames() const { return _track; }

  void SetAssetRemap(const std::string& remappedAssetName){ _remappedAssetName = remappedAssetName; }
  void ClearAssetRemap(){ _remappedAssetName.clear(); }

private:
  SpriteBoxTrack(SpriteBoxTrack&& other); // Move ctor
  SpriteBoxTrack& operator=(SpriteBoxTrack&& other); // Move assignment
  SpriteBoxTrack& operator=(const SpriteBoxTrack& other); // Copy assignment

  std::set<SpriteBoxKeyFrame> _track;

  TimeStamp_t _lastAccessTime_ms;
  TimeStamp_t _firstKeyFrameTime_ms;
  TimeStamp_t _lastKeyFrameTime_ms;

  bool _iteratorsAreValid;
  TrackIterator _currentKeyFrameIter;
  TrackIterator _nextKeyFrameIter;

  std::string _remappedAssetName;
};

} // namespace Animations
} // namespace Vector
} // namespace Anki

#endif //__Anki_Vector_SpriteBoxCompositor_H__
