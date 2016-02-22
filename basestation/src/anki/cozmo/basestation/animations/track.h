/**
* File: track
*
* Author: damjan stulic
* Created: 9/16/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/


#ifndef __Anki_Cozmo_Basestation_Animations_Track_H__
#define __Anki_Cozmo_Basestation_Animations_Track_H__

#include "anki/cozmo/basestation/keyframe.h"
#include "anki/common/types.h"
#include "json/json-forwards.h"
#include <stdint.h>
#include <list>


namespace Anki {
namespace Cozmo {

// Forward declaration
namespace RobotInterface {
class EngineToRobot;
  enum class EngineToRobotTag : uint8_t;
}
class Robot;

namespace Animations {

// templated class for storing/accessing various "tracks", which
// hold different types of KeyFrames.
template<class FRAME_TYPE>
class Track
{
public:
  static const size_t MAX_FRAMES_PER_TRACK = 1000;

  void Init();
  
  // If set to true, keyframes in this track will be deleted after they are played
  void SetIsLive(bool isLive) { _isLive = isLive; }
  bool IsLive() const { return _isLive; }

  Result AddKeyFrameToBack(const FRAME_TYPE& keyFrame);

  // Define from json. Second argument is used to print nicer debug strings if something goes wrong
  Result AddKeyFrameToBack(const Json::Value& jsonRoot, const std::string& animNameDebug = "");
  
  Result AddKeyFrameByTime(const FRAME_TYPE& keyFrame);

  // Return the Streaming message for the current KeyFrame if it is time,
  // nullptr otherwise. Also returns nullptr if there are no KeyFrames
  // left in the track.
  RobotInterface::EngineToRobot* GetCurrentStreamingMessage(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms);

  // Get a reference to the current KeyFrame in the track
  FRAME_TYPE& GetCurrentKeyFrame() { return *_frameIter; }
  
  // Get pointer to next keyframe. Returns nullptr if the track is on the last frame.
  const FRAME_TYPE* GetNextKeyFrame() const;
  
  // Get a pointer to the first KeyFrame in the track. Returns nullptr if track is empty
  const FRAME_TYPE* GetFirstKeyFrame() const;
  
  // Get a pointer to the last KeyFrame in the track. Returns nullptr if track is empty
  const FRAME_TYPE* GetLastKeyFrame() const;
  
  // Move to next frame and delete the current one if it's marked "live".
  // Will not advance past end.
  void MoveToNextKeyFrame();
  
  // Move to previous frame. Will not rewind before beginning.
  void MoveToPrevKeyFrame();

  bool HasFramesLeft() const { return _frameIter != _frames.end(); }

  bool IsEmpty() const { return _frames.empty(); }

  void Clear() { _frames.clear(); _frameIter = _frames.end(); }
  
  // Clear all frames up to, but not including, the current one
  void ClearUpToCurrent();

private:
  
  using FrameList = std::list<FRAME_TYPE>;
  FrameList _frames;
  typename FrameList::iterator _frameIter;
  
  bool _isLive = false;
  
}; // class Track


template<typename FRAME_TYPE>
void Track<FRAME_TYPE>::Init()
{
  _frameIter = _frames.begin();
}

template<typename FRAME_TYPE>
void Track<FRAME_TYPE>::MoveToNextKeyFrame()
{
  if(_isLive) {
    // Live frames get removed from the track once played
    _frameIter = _frames.erase(_frameIter);
  } else {
    // For canned frames, we just move to the next one in the track
    if(_frameIter != _frames.end()) {
      ++_frameIter;
    }
  }
}
  
template<typename FRAME_TYPE>
void Track<FRAME_TYPE>::MoveToPrevKeyFrame()
{
  if(_frameIter != _frames.begin()) {
    --_frameIter;
  }
}
  
template<typename FRAME_TYPE>
const FRAME_TYPE* Track<FRAME_TYPE>::GetNextKeyFrame() const
{
  ASSERT_NAMED(_frameIter != _frames.end(), "Frame iterator should not be at end.");
  
  auto nextIter = _frameIter;
  ++nextIter;
  
  if(nextIter == _frames.end()) {
    return nullptr;
  } else {
    return &(*nextIter);
  }
}

template<typename FRAME_TYPE>
const FRAME_TYPE* Track<FRAME_TYPE>::GetFirstKeyFrame() const
{
  if(_frames.empty()) {
    return nullptr;
  } else {
    return &(_frames.front());
  }
}

template<typename FRAME_TYPE>
const FRAME_TYPE* Track<FRAME_TYPE>::GetLastKeyFrame() const
{
  if(_frames.empty()) {
    return nullptr;
  } else {
    return &(_frames.back());
  }
}
  
template<typename FRAME_TYPE>
Result Animations::Track<FRAME_TYPE>::AddKeyFrameToBack(const FRAME_TYPE& keyFrame)
{
  if(_frames.size() > MAX_FRAMES_PER_TRACK) {
    PRINT_NAMED_ERROR("Animation.Track.AddKeyFrameToBack.TooManyFrames",
      "There are already %lu frames in %s track. Refusing to add more.",
      _frames.size(), keyFrame.GetClassName().c_str());
    return RESULT_FAIL;
  }

  _frames.emplace_back(keyFrame);

  // If we just added the first keyframe (e.g. after deleting the last remaining
  // keyframe in a "Live" track), we need to reset the frameIter to point
  // back to the beginning.
  if(_frames.size() == 1) {
    _frameIter = _frames.begin();
  }

  return RESULT_OK;
}
  
template<typename FRAME_TYPE>
Result Animations::Track<FRAME_TYPE>::AddKeyFrameByTime(const FRAME_TYPE& keyFrame)
{
  if(_frames.size() > MAX_FRAMES_PER_TRACK) {
    PRINT_NAMED_ERROR("Animation.Track.AddKeyFrameToBack.TooManyFrames",
      "There are already %lu frames in %s track. Refusing to add more.",
      _frames.size(), keyFrame.GetClassName().c_str());
    return RESULT_FAIL;
  }

  auto desiredTrigger = keyFrame.GetTriggerTime();
  
  auto framePlaceIter = _frames.begin();
  while (framePlaceIter != _frames.end() && framePlaceIter->GetTriggerTime() <= desiredTrigger)
  {
    // Don't put another key frame at the same time as an existing one
    if (framePlaceIter->GetTriggerTime() == desiredTrigger)
    {
      PRINT_NAMED_ERROR("Animation.Track.AddKeyFrameToBack.DuplicateTime",
                        "There is already a frame at time %u in %s track.",
                        desiredTrigger, keyFrame.GetClassName().c_str());
      return RESULT_FAIL;
    }
    ++framePlaceIter;
  }
  
  _frames.insert(framePlaceIter, keyFrame);

  // If we just added the first keyframe (e.g. after deleting the last remaining
  // keyframe in a "Live" track), we need to reset the frameIter to point
  // back to the beginning.
  if(_frames.size() == 1) {
    _frameIter = _frames.begin();
  }

  return RESULT_OK;
}

template<typename FRAME_TYPE>
RobotInterface::EngineToRobot* Animations::Track<FRAME_TYPE>::GetCurrentStreamingMessage(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms)
{
  RobotInterface::EngineToRobot* msg = nullptr;

  if(HasFramesLeft()) {
    FRAME_TYPE& currentKeyFrame = GetCurrentKeyFrame();
    if(currentKeyFrame.IsTimeToPlay(startTime_ms, currTime_ms))
    {
      msg = currentKeyFrame.GetStreamMessage();
      if(currentKeyFrame.IsDone()) {
        MoveToNextKeyFrame();
      }
    }
  }

  return msg;
}


template<typename FRAME_TYPE>
Result Animations::Track<FRAME_TYPE>::AddKeyFrameToBack(const Json::Value &jsonRoot, const std::string& animNameDebug)
{
  Result lastResult = AddKeyFrameToBack(FRAME_TYPE());
  if(RESULT_OK != lastResult) {
    return lastResult;
  }

  lastResult = _frames.back().DefineFromJson(jsonRoot, animNameDebug);

  if(lastResult == RESULT_OK) {
    if(_frames.size() > 1) {
      auto nextToLastFrame = _frames.rbegin();
      ++nextToLastFrame;

      if(_frames.back().GetTriggerTime() <= nextToLastFrame->GetTriggerTime()) {
        //PRINT_NAMED_ERROR("Animation.Track.AddKeyFrameToBack.BadTriggerTime", "New keyframe must be after the last keyframe.");
        _frames.pop_back();
        lastResult = RESULT_FAIL;
      }
    }
  }

  return lastResult;
}

template<typename FRAME_TYPE>
void Animations::Track<FRAME_TYPE>::ClearUpToCurrent()
{
  auto iter = _frames.begin();
  while(iter != _frameIter) {
    iter = _frames.erase(iter);
  }
}
  
} // end namespace Animations
} // end namespace Cozmo
} // end namespace Anki


#endif //__Anki_Cozmo_Basestation_Animations_Track_H__
