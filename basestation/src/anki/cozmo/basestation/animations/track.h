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
class Track {
public:
  static const size_t MAX_FRAMES_PER_TRACK = 100;

  void Init();

  Result AddKeyFrame(const FRAME_TYPE& keyFrame);
  Result AddKeyFrame(const Json::Value& jsonRoot);

  // Return the Streaming message for the current KeyFrame if it is time,
  // nullptr otherwise. Also returns nullptr if there are no KeyFrames
  // left in the track.
  RobotInterface::EngineToRobot* GetCurrentStreamingMessage(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms);

  // Get a reference to the current KeyFrame in the track
  FRAME_TYPE& GetCurrentKeyFrame() { return *_frameIter; }

  void MoveToNextKeyFrame();

  bool HasFramesLeft() const { return _frameIter != _frames.end(); }

  bool IsEmpty() const { return _frames.empty(); }

  void Clear() { _frames.clear(); _frameIter = _frames.end(); }

private:

  // returns false if frame buffer is full
  bool CheckFrameCount(const char* className);

  using FrameList = std::list<FRAME_TYPE>;
  FrameList                    _frames;
  typename FrameList::iterator _frameIter;
}; // class Animation::Track


template<typename FRAME_TYPE>
Result Animations::Track<FRAME_TYPE>::AddKeyFrame(const FRAME_TYPE& keyFrame)
{
  if(!CheckFrameCount(keyFrame.GetClassName().c_str())) {
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

// Specialization for ProceduralFace track because it needs look-back for interpolation
template<>
RobotInterface::EngineToRobot* Animations::Track<ProceduralFaceKeyFrame>::GetCurrentStreamingMessage(TimeStamp_t startTime_ms, TimeStamp_t currTime_ms)
{
  RobotInterface::EngineToRobot* msg = nullptr;

  if(HasFramesLeft()) {
    ProceduralFaceKeyFrame& currentKeyFrame = GetCurrentKeyFrame();
    if(currentKeyFrame.IsTimeToPlay(startTime_ms, currTime_ms))
    {
      if(currentKeyFrame.IsLive()) {
        // The AnimationStreamer will take care of interpolation for live
        // live streaming
        // TODO: Maybe we could also do it here somehow?
        msg = currentKeyFrame.GetStreamMessage();

      } else {
        auto nextIter = _frameIter;
        ++nextIter;
        if(nextIter != _frames.end()) {
          // If we have another frame coming, use it to interpolate.
          // This will only be "done" once
          msg = currentKeyFrame.GetInterpolatedStreamMessage(*nextIter);
        } else {
          // Otherwise, we'll just send the last frame
          msg = currentKeyFrame.GetStreamMessage();
        }
      }

      if(currentKeyFrame.IsDone()) {
        MoveToNextKeyFrame();
      }
    }
  }

  return msg;
}




template<typename FRAME_TYPE>
Result Animations::Track<FRAME_TYPE>::AddKeyFrame(const Json::Value &jsonRoot)
{
  Result lastResult = AddKeyFrame(FRAME_TYPE());
  if(RESULT_OK != lastResult) {
    return lastResult;
  }

  lastResult = _frames.back().DefineFromJson(jsonRoot);

  if(lastResult == RESULT_OK) {
    if(_frames.size() > 1) {
      auto nextToLastFrame = _frames.rbegin();
      ++nextToLastFrame;

      if(_frames.back().GetTriggerTime() <= nextToLastFrame->GetTriggerTime()) {
        //PRINT_NAMED_ERROR("Animation.Track.AddKeyFrame.BadTriggerTime", "New keyframe must be after the last keyframe.");
        _frames.pop_back();
        lastResult = RESULT_FAIL;
      }
    }
  }

  return lastResult;
}


} // end namespace Animations
} // end namespace Cozmo
} // end namespace Anki


#endif //__Anki_Cozmo_Basestation_Animations_Track_H__
