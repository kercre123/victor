/*
 * File: streamingAnimation.cpp
 *
 * Author: Jordan Rivas
 * Created: 07/19/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#include "anki/cozmo/basestation/animations/streamingAnimation.h"
#include "anki/cozmo/basestation/animations/track.h"

#include "anki/cozmo/basestation/audio/audioDataTypes.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"

#include "AudioEngine/audioCallback.h"


#include "util/dispatchQueue/dispatchQueue.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/random/randomGenerator.h"


namespace Anki {
namespace Cozmo {
namespace RobotAnimation {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StreamingAnimation::StreamingAnimation(const Animation& origalAnimation)
: Anki::Cozmo::Animation(origalAnimation)
{
  // Get Animation Info
  _lastKeyframeTime_ms = GetLastKeyFrameTime_ms();
  
  // Reset all tracks
  Init();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result StreamingAnimation::Init()
{
  _playheadFrame = 0;
  _playheadTime_ms = 0;
  _audioPlayheadIt = _audioFrames.begin();
  return Animation::Init();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StreamingAnimation::CanPlayNextFrame() const
{
  bool isReady = false;
  if (_state == StreamingAnimation::BufferState::ReadyBuffering && _playheadFrame < _audioBufferedFrameCount) {
    isReady = true;
  }
  else if (_state == StreamingAnimation::BufferState::Completed) {
    isReady = true;
  }
  return isReady;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::Update()
{
  switch (_state) {
    case BufferState::None:
      // Do Nothing
      break;
      
    case BufferState::WaitBuffering:
      // Check if there is more buffer to consume
      
// FIXME: Don't like this if statmeent might be a better way to test this, why are we being set to this state?
      
      if ((_audioBuffer->HasAudioBufferStream() && _audioBuffer->GetFrontAudioBufferStream()->HasAudioFrame())
          || (GetCompletedEventCount() >= _animationAudioEvents.size())) {
        _state = BufferState::ReadyBuffering;
      }
      break;
      
    case BufferState::ReadyBuffering:
      // Check if there is more buffer to consume
      CopyAudioBuffer();
      // Check if animation is completely buffered?
      if (IsAnimationDone()) {
        _state = BufferState::Completed;
      }
      break;
      
    case BufferState::Completed:
      // Done!!! Do something?
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AddKeyframeNonNullToList(const IKeyFrame* keyframe, KeyframeList& out_keyframeList)
{
  if (keyframe != nullptr) {
    out_keyframeList.push_back((IKeyFrame*)keyframe);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::TickPlayhead(KeyframeList& out_keyframeList, const Audio::AudioFrameData*& out_audioFrame)
{
  // Don't advance past next frame
  DEV_ASSERT(CanPlayNextFrame(), "StreamingAnimation.TickPlayhead.NextFrameIsNotReady");
  
  //////////////////////////////
  // Audio
  // Get Audio Frame animation
  out_audioFrame = *_audioPlayheadIt;
  
  
  //////////////////////////////
  // Audio Events
  // Is it time to play device audio? (using actual basestation time)
  auto& deviceAudioTrack = GetTrack<DeviceAudioKeyFrame>();
  if(deviceAudioTrack.HasFramesLeft() &&
     deviceAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_playheadTime_ms)) {
    //      deviceAudioTrack.GetCurrentKeyFrame().PlayOnDevice();
    deviceAudioTrack.MoveToNextKeyFrame();
  }
  
  // Don't send audio event track
  auto& robotAudioTrack  = GetTrack<RobotAudioKeyFrame>();
  if (robotAudioTrack.HasFramesLeft() && robotAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_playheadTime_ms)) {
    robotAudioTrack.MoveToNextKeyFrame();
  }
  
  //////////////////////////////
  // Face
  auto& proceduralFaceTrack = GetTrack<ProceduralFaceKeyFrame>();
  if (proceduralFaceTrack.HasFramesLeft() &&
      proceduralFaceTrack.GetCurrentKeyFrame().IsTimeToPlay(_playheadTime_ms)) {
    // FIXME: No proceduer face
    // Don't send procedure face
    proceduralFaceTrack.MoveToNextKeyFrame();
  }
  
  // Face Images
  //    bool streamedFaceAnimImage = false;
  auto& faceAnimTrack    = GetTrack<FaceAnimationKeyFrame>();
  const FaceAnimationKeyFrame* kf = faceAnimTrack.GetCurrentKeyframe(_playheadTime_ms);
  AddKeyframeNonNullToList(kf, out_keyframeList);
  
  
  //////////////////////////////
  // Motors
  auto& headTrack        = GetTrack<HeadAngleKeyFrame>();
  auto headMsg = headTrack.GetCurrentKeyframe(_playheadTime_ms);
  AddKeyframeNonNullToList(headMsg, out_keyframeList);
  
  auto& liftTrack        = GetTrack<LiftHeightKeyFrame>();
  auto liftMsg = liftTrack.GetCurrentKeyframe(_playheadTime_ms);
  AddKeyframeNonNullToList(liftMsg, out_keyframeList);
  
  auto& bodyTrack        = GetTrack<BodyMotionKeyFrame>();
  auto bodyMsg = bodyTrack.GetCurrentKeyframe(_playheadTime_ms);
  AddKeyframeNonNullToList(bodyMsg, out_keyframeList);
  
  
  //////////////////////////////
  // Event
  auto& eventTrack       = GetTrack<EventKeyFrame>();
  auto eventMsg = eventTrack.GetCurrentKeyframe(_playheadTime_ms);
  AddKeyframeNonNullToList(eventMsg, out_keyframeList);
  
  
  //////////////////////////////
  // Backpack Lights
  auto& backpackLedTrack = GetTrack<BackpackLightsKeyFrame>();
  auto backpackMsg = backpackLedTrack.GetCurrentKeyframe(_playheadFrame);
  AddKeyframeNonNullToList(backpackMsg, out_keyframeList);

  
  //////////////////////////////
  // Advance playheads
  ++_playheadFrame;
  _playheadTime_ms += IKeyFrame::SAMPLE_LENGTH_MS;
  ++_audioPlayheadIt;
  
  if (_playheadFrame == _audioBufferedFrameCount ||  _audioPlayheadIt == _audioFrames.end()) {
    PRINT_NAMED_WARNING("StreamingAnimation.TickPlayhead", "AUDIO FRAME END");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::GenerateAudioEventList(Util::RandomGenerator& randomGenerator)
{
  // Loop through tracks
  // Prep animation audio events
  Animations::Track<RobotAudioKeyFrame>& audioTrack = GetTrack<RobotAudioKeyFrame>();
  
  AnimationEvent::AnimationEventId eventId = AnimationEvent::kInvalidAnimationEventId;
  while (audioTrack.HasFramesLeft()) {
    const RobotAudioKeyFrame& aFrame = audioTrack.GetCurrentKeyFrame();
    const RobotAudioKeyFrame::AudioRef& audioRef = aFrame.GetAudioRef();
    const Audio::GameEvent::GenericEvent event = audioRef.audioEvent;
    if (Audio::GameEvent::GenericEvent::Invalid != event) {
      
      // Apply random weight
      bool playEvent = Util::IsFltNear(audioRef.probability, 1.0f);
      if (!playEvent) {
        playEvent = audioRef.probability >= randomGenerator.RandDbl(1.0);
        _hasRandomEvents = true;
      }
      else {
        // No random generator
        playEvent = true;
      }
      
      if (playEvent) {
        // Add Event to queue
        _animationAudioEvents.emplace_back(++eventId, event, aFrame.GetTriggerTime(), audioRef.volume);
        // Check if buffer should only play once
        if (!_hasAltEventAudio && audioRef.audioAlts) {
          // Don't replay buffer
          _hasAltEventAudio = true;
        }
      }
    }
    
    // TODO: Does this matter???? -> Don't advance before we've had a chance to use aFrame, because it could be "live"
    audioTrack.MoveToNextKeyFrame();
  }
  
  if (_animationAudioEvents.empty()) {
    _state = BufferState::Completed;
    return;
  }
  
  // Sort events by start time
  std::sort(_animationAudioEvents.begin(),
            _animationAudioEvents.end(),
            [] (const AnimationEvent& lhs, const AnimationEvent& rhs) { return lhs.time_ms < rhs.time_ms; });
  
  // Logs
  if (DEBUG_ROBOT_ANIMATION_AUDIO) {
    PRINT_NAMED_INFO("RobotAudioAnimation::LoadAnimation",
                     "Audio Events Size: %lu - Enter",
                     (unsigned long)_animationAudioEvents.size());
    for (size_t idx = 0; idx < _animationAudioEvents.size(); ++idx) {
      PRINT_NAMED_INFO("RobotAudioAnimation::LoadAnimation",
                       "Event Id: %d AudioEvent: %s TimeInMS: %d",
                       _animationAudioEvents[idx].eventId,
                       EnumToString(_animationAudioEvents[idx].audioEvent),
                       _animationAudioEvents[idx].time_ms);
    }
    PRINT_NAMED_INFO("RobotAudioAnimation::LoadAnimation", "Audio Events - Exit");
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::GenerateAudioData(Audio::RobotAudioClient* audioClient)
{
  DEV_ASSERT(audioClient != nullptr, "StreamingAnimation.GenerateAudioData.audioClient.IsNull");
  
  if (_animationAudioEvents.empty()) {
    // No events to buffer
    AddFakeAudio();
    return;
  }
  
  // Get GameObjected and audio buffer from pool
  _audioClient = audioClient;
  const bool success = _audioClient->GetGameObjectAndAudioBufferFromPool(_gameObj, _audioBuffer);
  DEV_ASSERT(success, "StreamingAnimation.GenerateAudioData.GetGameObjectAndAudioBufferFromPool.NoBufferInPool");
  
  _state = BufferState::WaitBuffering;
  
  const uint32_t firstAudioEventOffset = _animationAudioEvents.front().time_ms;
  for (auto& anEvent : _animationAudioEvents) {
    
    AnimationEvent* animationEvent = &anEvent;
    std::weak_ptr<char> isAliveWeakPtr(_isAliveSharedPtr);
    
    Util::Dispatch::Block aBlock = [this, animationEvent, isAliveWeakPtr = std::move(isAliveWeakPtr)]()
    {
      // Check if class has been destroyed
      if (isAliveWeakPtr.expired()) {
        return ;
      }
      
      // Prepare next event
      using namespace AudioEngine;
      using PlayId = Audio::RobotAudioClient::CozmoPlayId;
      const Audio::RobotAudioClient::CozmoEventCallbackFunc callbackFunc = [this, animationEvent, isAliveWeakPtr = std::move(isAliveWeakPtr)]
      (const AudioEngine::AudioCallbackInfo& callbackInfo)
      {
        if (!isAliveWeakPtr.expired()) {
          std::lock_guard<std::mutex> lock(_animationEventLock);
          HandleCozmoEventCallback(animationEvent, callbackInfo);
        }
      };
      
      // Ready to post event update state
      {
        std::lock_guard<std::mutex> lock(_animationEventLock);
        animationEvent->state = AnimationEvent::AnimationEventState::Posted;
        IncrementPostedEventCount();
      }
      
      // Post Event
      const PlayId playId = _audioClient->PostCozmoEvent(animationEvent->audioEvent,
                                                         _gameObj,
                                                         std::move(callbackFunc));
      
      if (playId != AudioEngine::kInvalidAudioPlayingId) {
        // Set event's volume RTPC
        _audioClient->SetCozmoEventParameter(playId,
                                             Audio::GameParameter::ParameterType::Event_Volume,
                                             animationEvent->volume);
      }
      // Processes event NOW, minimize buffering latency
      _audioClient->ProcessEvents();
    };
    
    // Perform Audio Event
    Util::Dispatch::After(_audioClient->GetAudioQueue(),
                          std::chrono::milliseconds(anEvent.time_ms - firstAudioEventOffset),
                          aBlock,
                          "CozmoAudioEvent");
  } // END for (auto& anEvent : _animationAudioEvents)
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::GenerateDeviceAudioEvents(Audio::RobotAudioClient* audioClient)
{
  // FIXME: Do Something!!!
  
  // Create audio silence frames
  AddFakeAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::PushFrameIntoBuffer(const Audio::AudioFrameData* frame)
{
  _audioFrames.push_back(frame);
  ++_audioBufferedFrameCount;
  
  if (_audioBufferedFrameCount == 1) {
    _audioPlayheadIt = _audioFrames.begin();
  }  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StreamingAnimation::AnimationEvent* StreamingAnimation::GetNextEvent() const
{
  if (GetEventIndex() < _animationAudioEvents.size()) {
    return &_animationAudioEvents[GetEventIndex()];
  }
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::HandleCozmoEventCallback(AnimationEvent* animationEvent,
                                                  const AudioEngine::AudioCallbackInfo& callbackInfo)
{
  // Mark audio events completed or error depending on callbacks type
  switch (callbackInfo.callbackType) {
      
    case AudioEngine::AudioCallbackType::Error:
    {
      PRINT_NAMED_WARNING("StreamingAnimation.HandleCozmoEventCallback", "Error: %s",
                          callbackInfo.GetDescription().c_str());
      IncrementCompletedEventCount();
      animationEvent->state = AnimationEvent::AnimationEventState::Error;
      break;
    }
      
    case AudioEngine::AudioCallbackType::Complete:
    {
      IncrementCompletedEventCount();
      animationEvent->state = AnimationEvent::AnimationEventState::Completed;
      break;
    }
      
    default:
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::CopyAudioBuffer()
{
  DEV_ASSERT(_audioClient != nullptr, "StreamingAnimation.Update._audioClient.IsNull");
  DEV_ASSERT(_audioBuffer != nullptr, "StreamingAnimation.Update._audioBuffer.IsNull");
  
  // Add Silence audio frames to while there is no Current Buffer Stream
  AddAudioSilenceFrames();
  
  // Add Audio frames from Buffer Stream
  AddAudioDataFrames();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::AddAudioSilenceFrames()
{
  // No stream
  // Add silence frames
  if (!HasCurrentBufferStream()) {
    
    // Need to know where I'm at in buffer state
    uint32_t frameTime_ms = _audioBufferedFrameCount * IKeyFrame::SAMPLE_LENGTH_MS;
    const AnimationEvent* nextEvent = GetNextEvent();
    Audio::RobotAudioFrameStream* nextStream = _audioBuffer->HasAudioBufferStream() ?
    _audioBuffer->GetFrontAudioBufferStream() : nullptr;
    
    
    // Check Initial case
    if (kInvalidAudioSreamOffsetTime ==_audioStreamOffsetTime_ms
        && nextStream != nullptr) {
      // Calculate the time (relative to the time when Audio Engine creates stream) stream offset
      _audioStreamOffsetTime_ms = nextStream->GetCreatedTime_ms() - nextEvent->time_ms;
    }
    
    // Loop though valid frames add audio silence
    // Note: '<' less then, add 1 extra keyframe of silence to Animation to catch any final keyframes
    while (frameTime_ms < _lastKeyframeTime_ms) {
      
      // First check if we should be pushing audio data, if so break out of while loop
      
      // This implies that we will not buffer any silence until we get the first audio stream. This method expects there
      // there is 1+ audio events.
      if ((kInvalidAudioSreamOffsetTime != _audioStreamOffsetTime_ms) && (nextStream != nullptr)) {
        
        const uint32_t streamRelevantTime_ms = floor(nextStream->GetCreatedTime_ms() - _audioStreamOffsetTime_ms);
        // Note: Intentionally set this only if less then frameTime, will sync with event time
        if (streamRelevantTime_ms < frameTime_ms) {
          // Start playing this stream
          if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
            PRINT_NAMED_INFO( "RobotAudioAnimationOnRobot.UpdateAudioFramesReady",
                             "Set Next Stream | SteamTime - StartTime_ms %d | AnimTime_ms %d",
                             streamRelevantTime_ms, frameTime_ms );
          }
          
          _currentBufferStream = nextStream;
          break;
        }
      }
      
      // Check if next audio event is ready to play
      if (nextEvent != nullptr && nextEvent->time_ms <= frameTime_ms) {
        // Next frame should be stream data
        if (nextStream != nullptr) {
          _currentBufferStream = nextStream;
        }
        else {
          // FIXME: Need to fix how states work
          _state = BufferState::WaitBuffering;
        }
        // Either condition we still need to exit while loop
        break;
      }
      
      // Else
      // If there is no audio for this frame add silence
      PushFrameIntoBuffer(nullptr);
      frameTime_ms += IKeyFrame::SAMPLE_LENGTH_MS;
    } // While
  } // if (!HasCurrentBufferStream())
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::AddAudioDataFrames()
{
  // Need to know where I'm at in buffer state
  uint32_t frameTime_ms = _audioBufferedFrameCount * IKeyFrame::SAMPLE_LENGTH_MS;
  const AnimationEvent* nextEvent = GetNextEvent();
  
  // While there is a current stream move as many frames as are available
  while (frameTime_ms < _lastKeyframeTime_ms && HasCurrentBufferStream()) {
    
    // Check for next frame
    if (!_currentBufferStream->HasAudioFrame()) {
      // No audio frame
      if (_currentBufferStream->IsComplete()) {
        // Stream complete, clear it
        _currentBufferStream = nullptr;
        _audioBuffer->PopAudioBufferStream();
      }
      // Still buffering stream, wait...
      _state = BufferState::WaitBuffering;
    }
    else {
      // Has frames, push them into audio frame list
      PushFrameIntoBuffer(_currentBufferStream->PopRobotAudioFrame());
      frameTime_ms += IKeyFrame::SAMPLE_LENGTH_MS;
      
      //  Check if Audio Event expired
      if (nextEvent != nullptr && nextEvent->time_ms <= frameTime_ms) {
        // I have confirmed there is only 1 event per keyframe
        IncrementEventIndex();
        nextEvent = GetNextEvent();
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::AddFakeAudio()
{
  uint32_t animationTime_ms = 0;
  while (animationTime_ms <= _lastKeyframeTime_ms) {
    
    PushFrameIntoBuffer(nullptr);
    
    animationTime_ms += IKeyFrame::SAMPLE_LENGTH_MS;
  }
  
  // All frames are ready
  _state = BufferState::Completed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StreamingAnimation::IsAnimationDone() const
{
  // Compare completed event count with number of events && there are no more audio streams
  const bool isDone = ((GetCompletedEventCount() >= _animationAudioEvents.size())
                       && !_audioBuffer->HasAudioBufferStream()
                       && !_audioBuffer->IsActive()
                       && ((_audioBufferedFrameCount * IKeyFrame::SAMPLE_LENGTH_MS) > _lastKeyframeTime_ms));
  
  if (DEBUG_ROBOT_ANIMATION_AUDIO) {
    PRINT_NAMED_INFO("RobotAudioAnimation.IsAnimationDone",
                     "eventCount: %zu  eventIdx: %d  completedCount: %d  hasAudioBufferStream: %d | Result %s",
                     _animationAudioEvents.size(),
                     GetEventIndex(),
                     GetCompletedEventCount(),
                     _audioBuffer->HasAudioBufferStream(),
                     isDone ? "T" : "F");
  }
  
  return isDone;
}

} // namespace RobotAnimation
} // namespcae Cozmo
} // namespace Anki
