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

#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include "anki/cozmo/basestation/audio/robotAudioBuffer.h"

#include "audioEngine/audioCallback.h"
#include "audioEngine/audioTools/audioDataTypes.h"


#include "util/dispatchQueue/dispatchQueue.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/random/randomGenerator.h"


namespace Anki {
namespace Cozmo {
namespace RobotAnimation {
  
// TEMP Solution to converting audio frame into robot audio message
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
constexpr float INT16_MAX_FLT = (float)std::numeric_limits<int16_t>::max();

uint8_t encodeMuLaw(float in_val)
{
  static const uint8_t MuLawCompressTable[] =
  {
    0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
    5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
  };
  
  // Convert float (-1.0, 1.0) to int16
  
  int16_t sample = Util::numeric_cast<int16_t>( Util::Clamp(in_val, -1.f, 1.f) * INT16_MAX_FLT );
  
  const bool sign = sample < 0;
  
  if (sign)	{
    sample = ~sample;
  }
  
  const uint8_t exponent = MuLawCompressTable[sample >> 8];
  uint8_t mantissa;
  
  if (exponent) {
    mantissa = (sample >> (exponent + 3)) & 0xF;
  } else {
    mantissa = sample >> 4;
  }
  
  return (sign ? 0x80 : 0) | (exponent << 4) | mantissa;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StreamingAnimation::StreamingAnimation(const std::string& name,
                                       AudioMetaData::GameObjectType gameObject,
                                       Audio::RobotAudioBuffer* audioBuffer,
                                       Audio::RobotAudioClient& audioClient)
: Anki::Cozmo::Animation(name)
, _gameObj(gameObject)
, _audioBuffer(audioBuffer)
, _audioClient(audioClient)
{
  // Get Animation Info
  // FIXME: This doesn't work we don't know how long the audio is
  _lastKeyframeTime_ms = GetLastKeyFrameTime_ms();
  
  // Reset all tracks
  Init();
  
  GenerateAudioEventList();
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StreamingAnimation::StreamingAnimation(const Animation& originalAnimation,
                                       AudioMetaData::GameObjectType gameObject,
                                       Audio::RobotAudioBuffer* audioBuffer,
                                       Audio::RobotAudioClient& audioClient)
: Anki::Cozmo::Animation(originalAnimation)
, _gameObj(gameObject)
, _audioBuffer(audioBuffer)
, _audioClient(audioClient)
{
  // Get Animation Info
  // FIXME: This doesn't work we don't know how long the audio is
  _lastKeyframeTime_ms = GetLastKeyFrameTime_ms();
  
  // Reset all tracks
  Init();
  
  GenerateAudioEventList();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StreamingAnimation::~StreamingAnimation()
{
  AbortAnimation();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result StreamingAnimation::Init()
{
  _playheadFrame = 0;
  _playheadTime_ms = 0;
  _audioPlayheadIt = _audioFrames.begin();
  _nextAnimationAudioEventIt = _animationAudioEvents.begin();
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
  // Check if there is more buffer to consume
  CopyAudioBuffer();
  
  switch (_state) {
    case BufferState::None:
      // Do Nothing
      break;
      
    case BufferState::WaitBuffering:
      // Check if there is more buffer to consume
      
// FIXME: Don't like this if statement might be a better way to test this, why are we being set to this state?
      
//      if ((_audioBuffer->HasAudioBufferStream() && _audioBuffer->GetFrontAudioBufferStream()->HasAudioFrame())
//          || (GetCompletedEventCount() >= _animationAudioEvents.size())) {
//        _state = BufferState::ReadyBuffering;
//      }
      
      if ((_audioFrames.size() - _playheadFrame) > 0) {
        _state = BufferState::ReadyBuffering;
      }
      
      break;
      
    case BufferState::ReadyBuffering:
      // Check if animation is completely buffered?
      if (IsAnimationDone()) {
        _state = BufferState::Completed;
      }
      break;
      
    case BufferState::Completed:
      // Done!!! Do something?
      break;
  }
  
  PRINT_CH_INFO("Animations", "StreamingAnimation.Update",
                "%s : isLive: %c PlayHeadFrame: %d  State: %d - audioFrames: %lu",
                GetName().c_str(), (IsLive() ? 'Y' : 'N'), _playheadFrame, _state,
                (unsigned long)(_audioFrames.size() - _playheadFrame));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::TickPlayhead(StreamingAnimationFrame& out_frame)
{
  // Don't advance past next frame
  DEV_ASSERT(CanPlayNextFrame(), "StreamingAnimation.TickPlayhead.NextFrameIsNotReady");
  
  PRINT_CH_INFO("Animations", "StreamingAnimation.TickPlayhead",
                "%s : isLive: %c PlayHeadFrame: %d Remaining Audio Frames: %lu",
                GetName().c_str(), (IsLive() ? 'Y' : 'N'), _playheadFrame,
                (unsigned long)(_audioFrames.size() - _playheadFrame));
  
  out_frame.ClearFrame();
  
  //////////////////////////////
  // Audio
  // Get Audio Frame animation
  out_frame.audioFrameData = *_audioPlayheadIt;
  
  
  //////////////////////////////
  // Audio Events
  // Is it time to play device audio? (using actual basestation time)
  // FIXME: Do this!

  auto& deviceAudioTrack = GetTrack<DeviceAudioKeyFrame>();
  if(deviceAudioTrack.HasFramesLeft() &&
     deviceAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_playheadTime_ms)) {
    //      deviceAudioTrack.GetCurrentKeyFrame().PlayOnDevice();
    deviceAudioTrack.MoveToNextKeyFrame();
  }
  
  // Don't send audio event track
  auto& robotAudioTrack  = GetTrack<RobotAudioKeyFrame>();
  if (robotAudioTrack.HasFramesLeft() && robotAudioTrack.GetCurrentKeyFrame().IsTimeToPlay(_playheadTime_ms)) {
    // Audio Events are already accounted for, just move to the next keyframe
    robotAudioTrack.MoveToNextKeyFrame();
  }
  
  // If output setting is PlayOnDevice post audio events to Audio Engine
  if (_audioClient.GetOutputSource() == Audio::RobotAudioClient::RobotAudioOutputSource::PlayOnDevice) {
    // Play all audio events in ths frame
    while (_nextAnimationAudioEventIt != _animationAudioEvents.end()) {
      AnimationEvent& event = *_nextAnimationAudioEventIt;
      // Check if the next audio event time is ready to play
      if (event.time_ms > _playheadTime_ms) {
        break;
      }
      // Play animation audio event
      // TODO: May need to add delay to compensate for robot buffer time
      std::weak_ptr<char> isAliveWeakPtr(_isAliveSharedPtr);
      PlayAnimationAudioEvent(&event, isAliveWeakPtr);
      ++_nextAnimationAudioEventIt;
    }
  }
  
  // Procedural Face
  auto& procFaceTrack = GetTrack<ProceduralFaceKeyFrame>();
  out_frame.procFaceFrame = procFaceTrack.GetCurrentKeyFrame(_playheadTime_ms);
  
  // Animated Face
  auto& animFaceTrack = GetTrack<FaceAnimationKeyFrame>();
  out_frame.animFaceFrame = animFaceTrack.GetCurrentKeyFrame(_playheadTime_ms);
  
  // Motors
  auto& headTrack = GetTrack<HeadAngleKeyFrame>();
  out_frame.headFrame = headTrack.GetCurrentKeyFrame(_playheadTime_ms);
  
  auto& liftTrack = GetTrack<LiftHeightKeyFrame>();
  out_frame.liftFrame = liftTrack.GetCurrentKeyFrame(_playheadTime_ms);
  
  auto& bodyTrack = GetTrack<BodyMotionKeyFrame>();
  out_frame.bodyFrame = bodyTrack.GetCurrentKeyFrame(_playheadTime_ms);
  
  auto& turnToRecordedHeadingTrack = GetTrack<TurnToRecordedHeadingKeyFrame>();
  out_frame.turnToRecordedHeadingFrame = turnToRecordedHeadingTrack.GetCurrentKeyFrame(_playheadTime_ms);
  
  // Record heading
  auto& recordHeadingTrack = GetTrack<RecordHeadingKeyFrame>();
  out_frame.recordHeadingFrame = recordHeadingTrack.GetCurrentKeyFrame(_playheadTime_ms);
  
  // Animation Events
  auto& eventTrack = GetTrack<EventKeyFrame>();
  out_frame.eventFrame = eventTrack.GetCurrentKeyFrame(_playheadTime_ms);
  
  // Backpack Lights
  auto& backpackTrack = GetTrack<BackpackLightsKeyFrame>();
  out_frame.backpackFrame = backpackTrack.GetCurrentKeyFrame(_playheadFrame);
  
  // Advance playheads
  ++_playheadFrame;
  _playheadTime_ms += IKeyFrame::SAMPLE_LENGTH_MS;
  ++_audioPlayheadIt;
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::GenerateAudioEventList()
{
  // Loop through tracks
  // Prep animation audio events
  Animations::Track<RobotAudioKeyFrame>& audioTrack = GetTrack<RobotAudioKeyFrame>();
  
  AnimationEvent::AnimationEventId eventId = AnimationEvent::kInvalidAnimationEventId;
  while (audioTrack.HasFramesLeft()) {
    const RobotAudioKeyFrame& aFrame = audioTrack.GetCurrentKeyFrame();
    const RobotAudioKeyFrame::AudioRef& audioRef = aFrame.GetAudioRef();
    const AudioMetaData::GameEvent::GenericEvent event = audioRef.audioEvent;
    if (AudioMetaData::GameEvent::GenericEvent::Invalid != event) {
      
      // Apply random weight
      bool playEvent = Util::IsFltNear(audioRef.probability, 1.0f);
      if (!playEvent) {
        playEvent = audioRef.probability >= _audioClient.GetRandomGenerator().RandDbl(1.0);
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
  
  // Reset nextEventIt to front
  _nextAnimationAudioEventIt = _animationAudioEvents.begin();
  
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

void StreamingAnimation::PlayAnimationAudioEvent(AnimationEvent* animationEvent, std::weak_ptr<char> isAliveWeakPtr)
{
  DEV_ASSERT(animationEvent != nullptr, "StreamingAnimation.PlayAnimationAudioEvent.animationEvent.IsNull");
  
  // Check if class has been destroyed
  if (isAliveWeakPtr.expired()) {
    return;
  }
  
  // Prepare next event
  using namespace AudioEngine;
  using PlayId = Audio::RobotAudioClient::CozmoPlayId;
  const Audio::RobotAudioClient::CozmoEventCallbackFunc callbackFunc = [this, animationEvent, isAliveWeakPtr]
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
  const PlayId playId = _audioClient.PostCozmoEvent(animationEvent->audioEvent,
                                                    _gameObj,
                                                    callbackFunc);
  
  if (playId != AudioEngine::kInvalidAudioPlayingId) {
    // Set event's volume RTPC
    _audioClient.SetCozmoEventParameter(playId,
                                        AudioMetaData::GameParameter::ParameterType::Event_Volume,
                                        animationEvent->volume);
  }
  // Processes event NOW, minimize buffering latency
  _audioClient.ProcessEvents();
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::GenerateAudioData()
{
  if (_animationAudioEvents.empty()) {
    // No events to buffer
    AddFakeAudio();
    return;
  }
  
  // Get GameObject and audio buffer from pool
  
//  _audioClient = audioClient;
//  const bool success = _audioClient.GetGameObjectAndAudioBufferFromPool(_gameObj, _audioBuffer);
//  ASSERT_NAMED(success, "StreamingAnimation.GenerateAudioData.GetGameObjectAndAudioBufferFromPool.NoBufferInPool");
  
  _state = BufferState::WaitBuffering;
  
  const uint32_t firstAudioEventOffset = _animationAudioEvents.front().time_ms;
  for (auto& anEvent : _animationAudioEvents) {
    
    AnimationEvent* animationEvent = &anEvent;
    std::weak_ptr<char> isAliveWeakPtr(_isAliveSharedPtr);
    
    Util::Dispatch::Block aBlock = [this, animationEvent, isAliveWeakPtr]()
    {
      PlayAnimationAudioEvent(animationEvent, isAliveWeakPtr);
    };
    
    // Perform Audio Event
    Util::Dispatch::After(_audioClient.GetAudioQueue(),
                          std::chrono::milliseconds(anEvent.time_ms - firstAudioEventOffset),
                          aBlock,
                          "CozmoAudioEvent");
  } // END for (auto& anEvent : _animationAudioEvents)
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::GenerateDeviceAudioEvents()
{
  // FIXME: Do Something!!!
  
  // Create audio silence frames
  AddFakeAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::AbortAnimation()
{
  switch (_state) {
      
    case BufferState::None:
    case BufferState::Completed:
      // Do nothing, audio engine is not running
      break;
      
    case BufferState::WaitBuffering:
    case BufferState::ReadyBuffering:
    {
      // Stop buffering audio
      // TODO: Cancel dispatch after events
      
      if (nullptr != _audioBuffer) {
        // Check if all posted events have been completed
        const bool isComplete = ( GetPostedEventCount() <= GetCompletedEventCount() );
        _audioBuffer->ResetAudioBufferAnimationCompleted(isComplete);
      }
      _audioClient.StopCozmoEvent(_gameObj);
      
      // FIXME: I think this is ok to be marked as completed rather then create "Abort" state
      _state = BufferState::Completed;
      
      break;
    }
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TEMP Method - Help us bridge to old AnimationStreamer
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RobotInterface::EngineToRobot* StreamingAnimation::CreateRobotMessage(const AudioEngine::AudioFrameData* audioFrame) {
  
  RobotInterface::EngineToRobot* audioMsg = nullptr;
  if (nullptr != audioFrame) {
    // Create Audio Frame
    AnimKeyFrame::AudioSample keyFrame;
    DEV_ASSERT(static_cast<int32_t>( AnimConstants::AUDIO_SAMPLE_SIZE ) <= keyFrame.Size(),
                 "Block size must be less or equal to audioSameple size");
    // Convert audio format to robot format
    for ( size_t idx = 0; idx < audioFrame->sampleCount; ++idx ) {
      keyFrame.sample[idx] = encodeMuLaw( audioFrame->samples[idx] );
    }
    
    // Pad the back of the buffer with 0s
    // This should only apply to the last frame
    if (audioFrame->sampleCount < static_cast<int32_t>(AnimConstants::AUDIO_SAMPLE_SIZE)) {
      std::fill(keyFrame.sample.begin() + audioFrame->sampleCount, keyFrame.sample.end(), 0);
    }
    audioMsg = new RobotInterface::EngineToRobot( std::move(keyFrame) );
  }
  else {
    // Create Silence message
    audioMsg = new RobotInterface::EngineToRobot(AnimKeyFrame::AudioSilence());
  }
  
  return audioMsg;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::PushFrameIntoBuffer(const AudioEngine::AudioFrameData* frame)
{
  const bool resetAudioIt = (_audioPlayheadIt == _audioFrames.end());
  if (resetAudioIt) {
    // Go to previous node before pushing back more frames
    // This occurs when all of the available audio frames have been consumed
    --_audioPlayheadIt;
  }
  
  // Add new frame to buffer.  Note that nullptr values may be added to indicate silence.
  _audioFrames.push_back(frame);
  ++_audioBufferedFrameCount;
  
  if (_audioBufferedFrameCount == 1) {
    _audioPlayheadIt = _audioFrames.begin();
  } else if (resetAudioIt) {
    // Go forward to set iterator to the frame that was just added
    ++_audioPlayheadIt;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StreamingAnimation::AnimationEvent* StreamingAnimation::GetNextAudioEvent() const
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
    {
      break;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::CopyAudioBuffer()
{
  if (_audioBuffer != nullptr) {
    // Add audio silence frames if there is no Current Buffer Stream
    AddAudioSilenceFrames();
    
    // Add Audio frames from Buffer Stream
    AddAudioDataFrames();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::AddAudioSilenceFrames()
{
  // No stream
  // Add silence frames
  if (!HasCurrentBufferStream()) {
    
    // Need to know where I'm at in buffer state
    uint32_t frameStartTime_ms = _audioBufferedFrameCount * IKeyFrame::SAMPLE_LENGTH_MS;
    uint32_t frameEndTime_ms = frameStartTime_ms + IKeyFrame::SAMPLE_LENGTH_MS - 1;
    
    const AnimationEvent* nextEvent = GetNextAudioEvent();
    Audio::RobotAudioFrameStream* nextStream = _audioBuffer->HasAudioBufferStream() ?
    _audioBuffer->GetFrontAudioBufferStream() : nullptr;
    
    
    // Check Initial case
    if (kInvalidAudioStreamOffsetTime ==_audioStreamOffsetTime_ms
        && nextStream != nullptr) {
      // Calculate the time (relative to the time when Audio Engine creates stream) stream offset
      _audioStreamOffsetTime_ms = nextStream->GetCreatedTime_ms() - nextEvent->time_ms;
    }
    
    // Loop though valid frames add audio silence
    // ---- UPDATE COMMENT --- Note: '<' less then, add 1 extra keyframe of silence to Animation to catch any final keyframes
    while (frameStartTime_ms <= _lastKeyframeTime_ms) {
      
      // First check if we should be pushing audio data, if so break out of while loop
      
      // This implies that we will not buffer any silence until we get the first audio stream. This method expects there
      // there is 1+ audio events.
      if ((kInvalidAudioStreamOffsetTime != _audioStreamOffsetTime_ms) && (nextStream != nullptr)) {
        
        const uint32_t streamRelevantTime_ms = floor(nextStream->GetCreatedTime_ms() - _audioStreamOffsetTime_ms);
        // Note: Intentionally set this only if less then frameTime, will sync with event time
        if (streamRelevantTime_ms < frameEndTime_ms) {
          // Start playing this stream
          if ( DEBUG_ROBOT_ANIMATION_AUDIO ) {
            PRINT_NAMED_INFO( "StreamingAnimation.AddAudioSilenceFrames",
                             "Set Next Stream | SteamTime - StartTime_ms %d | frameStartTime_ms %d | frameEndTime_ms %d",
                             streamRelevantTime_ms, frameStartTime_ms, frameEndTime_ms);
          }
          
          _currentBufferStream = nextStream;
          break;
        }
      }
      
      // Check if next audio event is ready to play
      if (nextEvent != nullptr && nextEvent->time_ms <= frameStartTime_ms) {
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
      frameStartTime_ms += IKeyFrame::SAMPLE_LENGTH_MS;
    } // While
  } // if (!HasCurrentBufferStream())
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingAnimation::AddAudioDataFrames()
{
  // Need to know where I'm at in buffer state
  uint32_t frameEndTime_ms = (_audioBufferedFrameCount * IKeyFrame::SAMPLE_LENGTH_MS) + IKeyFrame::SAMPLE_LENGTH_MS - 1;
  
  const AnimationEvent* nextEvent = GetNextAudioEvent();
  
  // While there is a current stream, move as many frames as are available
  while (HasCurrentBufferStream()) {
    
    // Check for next frame
    if (_currentBufferStream->HasAudioFrame()) {
      // Has frames, push them into audio frame list
      PushFrameIntoBuffer(_currentBufferStream->PopRobotAudioFrame());
      frameEndTime_ms += IKeyFrame::SAMPLE_LENGTH_MS;
      
      //  Check if Audio Event expired
      if (nextEvent != nullptr && nextEvent->time_ms < frameEndTime_ms) {
        // I have confirmed there is only 1 event per keyframe
        IncrementEventIndex();
        nextEvent = GetNextAudioEvent();
      }
    }
    else {
      // No audio frame
      if (_currentBufferStream->IsComplete()) {
        // Stream complete, clear it
        _currentBufferStream = nullptr;
        _audioBuffer->PopAudioBufferStream();
      }
      else if (GetCompletedEventCount() < _animationAudioEvents.size()) {
        // Still buffering stream, wait...
        _state = BufferState::WaitBuffering;
      }
      break;
    }
  } // While END
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
} // namespace Cozmo
} // namespace Anki
