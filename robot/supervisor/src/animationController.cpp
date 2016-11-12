// This enables AnkiAsserts(), which are always no-ops by default (see config.h)
#ifndef _DEBUG
#define _DEBUG
#endif

#include "anki/cozmo/robot/hal.h"
#include "animationController.h"
#include "clad/types/animationKeyFrames.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "timeProfiler.h"
#include "anki/cozmo/robot/logging.h"
#ifdef TARGET_ESPRESSIF
#include "anki/cozmo/robot/esp.h"
#include "rtip.h"
#include "face.h"
extern "C" {
  #include "client.h"
  #include "anki/cozmo/robot/drop.h"
}
extern "C" void FacePrintf(const char *format, ...);
#else // Not on Espressif
#define STORE_ATTR
#include <string.h>
#include "headController.h"
#include "liftController.h"
#include "wheelController.h"
#include "steeringController.h"
#include "speedController.h"
static inline u32 system_get_time() { return Anki::Cozmo::HAL::GetTimeStamp() * 1000; }
#endif
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

#define DEBUG_ANIMATION_CONTROLLER 0

namespace Anki {
namespace Cozmo {
namespace AnimationController {

  namespace {

    // Streamed animation will not play until we've got this many _audio_ keyframes
    // buffered.
    static const s32 ANIMATION_PREROLL_LENGTH = 7;

    // Circular byte buffer for keyframe messages
    STORE_ATTR u8 _keyFrameBuffer[KEYFRAME_BUFFER_SIZE];
    s32 _currentBufferPos;
    s32 _lastBufferPos;

    s32 _numAudioFramesBuffered; // NOTE: Also counts EndOfAnimationFrames...
    s32 _numBytesPlayed = 0;
    s32 _numBytesInBuffer = 0;
    s32 _numAudioFramesPlayed = 0;
    
    u32 _lastStateSendTime = 0;

    u8  _currentTag = 0;

    bool _isBufferStarved;
    u16 _haveReceivedTerminationFrame;
    bool _isPlaying;
    bool _bufferFullMessagePrintedThisTick;

    s32 _tracksToPlay;

    int _tracksInUse = 0;
    
    typedef enum {
      DISABLED,
      SUSPENDED,
      ENABLED,
    } ACOpState;

    ACOpState _animCtrlState;

#   if DEBUG_ANIMATION_CONTROLLER
    TimeStamp_t _currentTime_ms;
#   endif

  } // "private" members

  Result Init()
  {
#   if DEBUG_ANIMATION_CONTROLLER
    AnkiDebug( 395, "AnimationController.Init", 3, "Initializing", 0);
#   endif
    
    _animCtrlState = DISABLED;

    return RESULT_OK;
  }
  
  Result EngineInit(const AnimKeyFrame::InitController& msg)
  {
    if (_animCtrlState == DISABLED)
    {
      _tracksToPlay = ALL_TRACKS;
      Clear();
      AnimationController::ClearNumBytesPlayed();
      AnimationController::ClearNumAudioFramesPlayed();
      _animCtrlState = ENABLED;
      return RESULT_OK;
    }
    else
    {
      AnkiWarn( 388, "AnimationController.engine_init.refused", 612, "Not initalizing animation controller because in state %d", 1, _animCtrlState);
      return RESULT_FAIL;
    }
  }

  Result EngineDisconnect()
  {
    if (_animCtrlState == ENABLED)
    {
      _animCtrlState = DISABLED;
    }
    return RESULT_OK;
  }

  static s32 GetNumBytesAvailable()
  {
    if(_lastBufferPos >= _currentBufferPos) {
      return KEYFRAME_BUFFER_SIZE - (_lastBufferPos - _currentBufferPos);
    } else {
      return _currentBufferPos - _lastBufferPos;
    }
  }

  static s32 GetNumBytesInBuffer()
  {
    return _numBytesInBuffer;
  }

  s32 GetTotalNumBytesPlayed() {
    return _numBytesPlayed;
  }

  void ClearNumBytesPlayed() {
    _numBytesPlayed = 0;
  }

  s32 GetTotalNumAudioFramesPlayed() {
    return _numAudioFramesPlayed;
  }

  void ClearNumAudioFramesPlayed() {
    _numAudioFramesPlayed = 0;
  }

  void CountConsumedBytes(const s32 num)
  {
    _numBytesPlayed   += num;
    _numBytesInBuffer -= num;
  }

  static void StopTracks(const u8 whichTracks)
  {
    if(whichTracks)
    {
#ifdef TARGET_ESPRESSIF
      MAKE_RTIP_MSG(msg);
#endif
      
      if(whichTracks & HEAD_TRACK)
      {
#ifdef TARGET_ESPRESSIF
        msg.tag = RobotInterface::EngineToRobot::Tag_moveHead;
        msg.moveHead.speed_rad_per_sec = 0.0f;
        RTIP::SendMessage(msg);
#else
        HeadController::SetAngularVelocity(0);
#endif
      }
      
      if(whichTracks & LIFT_TRACK)
      {
#ifdef TARGET_ESPRESSIF
        msg.tag = RobotInterface::EngineToRobot::Tag_moveLift;
        msg.moveLift.speed_rad_per_sec = 0.0f;
        RTIP::SendMessage(msg);
#else
        LiftController::SetAngularVelocity(0);
#endif
      }
      
      if(whichTracks & BODY_TRACK)
      {
#ifdef TARGET_ESPRESSIF
        msg.tag = RobotInterface::EngineToRobot::Tag_animBodyMotion;
        msg.animBodyMotion.speed = 0;
        msg.animBodyMotion.curvatureRadius_mm = 0;
        RTIP::SendMessage(msg);
#else
        SteeringController::ExecuteDirectDrive(0, 0);
#endif
      }
    }
  }
  
  static inline void StopTracksInUse()
  {
    // In case we are aborting an animation, stop any tracks that were in use
    // (For now, this just means motor-based tracks.) Note that we don't
    // stop tracks we weren't using, in case we were, for example, playing
    // a head animation while driving a path.
    StopTracks(_tracksInUse);
    _tracksInUse = 0;
  }
  
  void SendAnimationEnded()
  {
    if (_currentTag > 0) {
      #ifdef TARGET_ESPRESSIF
        MAKE_RTIP_MSG(msg);

        msg.tag = RobotInterface::EngineToRobot::Tag_setBackpackLayer;
        msg.setBackpackLayer.layer = BPL_USER;
        
        RTIP::SendMessage(msg);
      #else 
        // TODO: BACKPACK CONTROLLER IN SUPERVISOR
      #endif
      
      // Send animation ended to engine
      RobotInterface::AnimationEnded aem;
      aem.tag = _currentTag;
      RobotInterface::SendMessage(aem);
    }
  }

  void Clear()
  {
#   if DEBUG_ANIMATION_CONTROLLER
    AnkiDebug( 377, "AnimationController.Clear", 4, "Clearing", 0);
#   endif
    
    SendAnimationEnded();

    _numBytesPlayed += GetNumBytesInBuffer();
    _numAudioFramesPlayed += _numAudioFramesBuffered;
    _numBytesInBuffer = 0;

    _currentBufferPos = 0;
    _lastBufferPos = 0;
    _currentTag = 0;

    _numAudioFramesBuffered = 0;

    _haveReceivedTerminationFrame = 0;
    _isPlaying = false;
    _isBufferStarved = false;
    _bufferFullMessagePrintedThisTick = false;

    StopTracksInUse();

#   if DEBUG_ANIMATION_CONTROLLER
    _currentTime_ms = 0;
#   endif
  }

  Result SendAnimStateMessage()
  {
    RobotInterface::AnimationState msg;
    msg.timestamp = system_get_time();
    msg.numAnimBytesPlayed = GetTotalNumBytesPlayed();
    msg.numAudioFramesPlayed = GetTotalNumAudioFramesPlayed();
    msg.enabledAnimTracks = GetEnabledTracks();
    msg.tag = GetCurrentTag();
    if (Anki::Cozmo::HAL::RadioSendMessage(msg.GetBuffer(), msg.Size(), RobotInterface::RobotToEngine::Tag_animState))
    {
      return RESULT_OK;
    }
    else
    {
      return RESULT_FAIL;
    }
  }

  static inline RobotInterface::EngineToRobot::Tag PeekBufferTag()
  {
    int nextTagIndex = _currentBufferPos + 2;
    if (nextTagIndex >= KEYFRAME_BUFFER_SIZE) nextTagIndex -= KEYFRAME_BUFFER_SIZE;
    return _keyFrameBuffer[nextTagIndex];
  }

  static u32 GetFromBuffer(u8* data, u32 numBytes)
  {
    AnkiAssert(numBytes < KEYFRAME_BUFFER_SIZE, 5);
    
    if(_currentBufferPos + numBytes < KEYFRAME_BUFFER_SIZE) {
      // There's enough room from current position to end of buffer to just
      // copy directly
      memcpy(data, _keyFrameBuffer + _currentBufferPos, numBytes);
      _currentBufferPos += numBytes;
    } else {
      // Copy the first chunk from whatever remains from current position to end of
      // the buffer
      const s32 firstChunk = KEYFRAME_BUFFER_SIZE - _currentBufferPos;
      memcpy(data, _keyFrameBuffer + _currentBufferPos, firstChunk);

      // Copy the remaining data starting at the beginning of the buffer
      memcpy(data+firstChunk, _keyFrameBuffer, numBytes - firstChunk);
      _currentBufferPos = numBytes-firstChunk;
    }
    return numBytes;
  }

  static s32 GetFromBuffer(RobotInterface::EngineToRobot* msg)
  {
    u16 size;
    GetFromBuffer(reinterpret_cast<u8*>(&size), 2);
    if (GetNumBytesAvailable() < size)
    {
      AnkiError( 136, "AnimationController.BufferCorrupt", 392, "Message size header (%d) greater than available bytes (%d), assuming corrupt and clearing", 2, GetNumBytesAvailable(), size);
      Clear();
      return 0;
    }
    GetFromBuffer(msg->GetBuffer(), size);
    if (msg->Size() != size)
    {
      AnkiError( 136, "AnimationController.BufferCorrupt", 393, "Clad message size (%d) doesn't match stored header (%d), assuming corrupt and clearing", 2, msg->Size(), size);
      Clear();
      return 0;
    }
    // Increment total number of bytes played since startup
    CountConsumedBytes(size);
    return size+2;
  }

  Result BufferKeyFrame(const u8* buffer, const u16 bufferSize)
  {
    const s32 numBytesAvailable = GetNumBytesAvailable();
    const s32 numBytesNeeded = bufferSize + 2;
    
    if (_animCtrlState != ENABLED) 
    {
      AnkiDebug( 378, "AnimationController.buffer_key_frame.not_animCtrlState", 477, "BufferKeyFrame called while disabled", 0);
      return RESULT_FAIL;
    }
    
    if(numBytesAvailable < numBytesNeeded) {
      // Only print the error message if we haven't already done so this tick,
      // to prevent spamming that could clog reliable UDP
      if(!_bufferFullMessagePrintedThisTick) {
        AnkiDebug( 379, "AnimationController.BufferKeyFrame.BufferFull", 6, "BufferKeyFrame.BufferFull %d bytes available, %d needed.", 2,
                  numBytesAvailable, numBytesNeeded);
        _bufferFullMessagePrintedThisTick = true;
      }
      return RESULT_FAIL;
    }
#if DEBUG_ANIMATION_CONTROLLER > 1
    AnkiDebug( 380, "AnimationController.BufferKeyFrame", 7, "BufferKeyFrame, %d -> %d (%d)", 3, numBytesNeeded, _lastBufferPos, numBytesAvailable);
#endif

    AnkiAssert(numBytesNeeded < KEYFRAME_BUFFER_SIZE, 8);

    _keyFrameBuffer[_lastBufferPos++] = bufferSize & 0xff;
    if (_lastBufferPos >= KEYFRAME_BUFFER_SIZE) _lastBufferPos -= KEYFRAME_BUFFER_SIZE;
    _keyFrameBuffer[_lastBufferPos++] = bufferSize >> 8;

    if(_lastBufferPos + bufferSize < KEYFRAME_BUFFER_SIZE) {
      // There's enough room from current end position to end of buffer to just
      // copy directly
      memcpy(_keyFrameBuffer + _lastBufferPos, buffer, bufferSize);
      _lastBufferPos += bufferSize;
    } else {
      // Copy the first chunk into whatever fits from current position to end of
      // the buffer
      const s32 firstChunk = KEYFRAME_BUFFER_SIZE - _lastBufferPos;
      memcpy(_keyFrameBuffer + _lastBufferPos, buffer, firstChunk);

      // Copy the remaining data starting at the beginning of the buffer
      memcpy(_keyFrameBuffer, buffer+firstChunk, bufferSize - firstChunk);
      _lastBufferPos = bufferSize-firstChunk;
     }
    switch(buffer[0]) {
      case RobotInterface::EngineToRobot::Tag_animEndOfAnimation:
        ++_haveReceivedTerminationFrame;
      case RobotInterface::EngineToRobot::Tag_animAudioSample:
      case RobotInterface::EngineToRobot::Tag_animAudioSilence:
        ++_numAudioFramesBuffered;
        break;
        default:
        break;
    }

    AnkiAssert(_lastBufferPos >= 0 && _lastBufferPos < KEYFRAME_BUFFER_SIZE, 9);

    _numBytesInBuffer += bufferSize;

    return RESULT_OK;
  }

  bool IsBufferFull()
  {
    return GetNumBytesAvailable() > 0;
  }

  bool IsPlaying()
  {
    return _isPlaying;
  }

  static inline bool IsReadyToPlay()
  {

    bool ready = false;

    if(_isPlaying) {
      // If we are already in progress playing something, we are "ready to play"
      // until we run out of keyframes in the buffer
      // Note that we need at least two "frames" in the buffer so we can always
      // read from the current one to the next one without reaching end of buffer.
      ready = _numAudioFramesBuffered > 1;

      // Report every time the buffer goes from having a sufficient number of audio frames to not.
      if (!ready) {
        if (!_isBufferStarved) {
          _isBufferStarved = true;
          AnkiWarn( 169, "AnimationController.IsReadyToPlay.BufferStarved", 305, "", 0);
          
          // Failsafe: make sure body stops moving.
          // We don't have to do this with lift/head, because they will stop
          // when the controller reaches its desired value.
          if(_tracksInUse & BODY_TRACK) {
            #ifdef TARGET_ESPRESSIF
            MAKE_RTIP_MSG(msg);
            msg.tag = RobotInterface::EngineToRobot::Tag_animBodyMotion;
            msg.animBodyMotion.speed = 0;
            msg.animBodyMotion.curvatureRadius_mm = 0;
            RTIP::SendMessage(msg);
            #else
            SteeringController::ExecuteDirectDrive(0, 0);
            #endif
            _tracksInUse &= ~BODY_TRACK;
          }
          
        }
      } else {
        _isBufferStarved = false;
      }

    } else {
      // Otherwise, wait until we get enough frames to start
      ready = (_numAudioFramesBuffered >= ANIMATION_PREROLL_LENGTH || _haveReceivedTerminationFrame > 0);
      if(ready) {
        _isPlaying = true;
        _isBufferStarved = false;

#       if DEBUG_ANIMATION_CONTROLLER
        _currentTime_ms = 0;
#       endif
      }
    }

    //AnkiAssert(_currentFrame <= _lastFrame, 11);

    return ready;
  } // IsReadyToPlay()

  inline bool AdvanceAudio(RobotInterface::EngineToRobot& msg)
  {
    if (_animCtrlState != ENABLED) return false;
    else if(IsReadyToPlay()) {
      if (HAL::AudioReady())
      {
        START_TIME_PROFILE(Anim, AUDIOPLAY);

        // Next thing in the buffer should be audio or silence:
        RobotInterface::EngineToRobot::Tag msgID = PeekBufferTag();

        // If the next message is not audio, the animation queue has completely desynchronized
        // We almost certainly are looking at the middle of a message (and not start), so we can't continue
        // If it's not an animationController bug, it's most likely a mismatch between basestation and Espressif CLAD builds
        if (msgID != RobotInterface::EngineToRobot::Tag_animAudioSilence &&
              msgID != RobotInterface::EngineToRobot::Tag_animAudioSample)
        {
          // Shut down, and report back what happened
          AnkiWarn( 404, "AnimationController.advance_audio.currupt_buffer", 620, "Unexpected msgID=%d", 1, msgID);
          SendAnimationEnded();
        }

        switch(msgID)
        {
          case RobotInterface::EngineToRobot::Tag_animAudioSilence:
          {
            GetFromBuffer(&msg);
            HAL::AudioPlaySilence();
            return true; // Advance animation
          }
          case RobotInterface::EngineToRobot::Tag_animAudioSample:
          {
            if(_tracksToPlay & AUDIO_TRACK) {
              GetFromBuffer(&msg);
              HAL::AudioPlayFrame(&msg.animAudioSample);
              return true; // Advance animation
            } else {
              GetFromBuffer(&msg);
              HAL::AudioPlaySilence();
              return true; // Advance animation
            }
          }
          default:
          {
            AnkiWarn( 177, "AnimationController.ExpectedAudio", 455, "Expecting either audio sample or silence next in animation buffer. (Got 0x%02x instead)", 1, msgID);
            Clear();
            return false;
          }
        }
      }
      else
      {
        return false; // Do not advance animation
      }
    }
    else
    {
      return false; // Do not advance animation
    }
  }

  Result Update()
  {
    RobotInterface::EngineToRobot msg; // Allocate on stack here to share with AdvanceAudio function
    if (AdvanceAudio(msg))
    {
      RobotInterface::EngineToRobot::Tag msgID;
#       if DEBUG_ANIMATION_CONTROLLER
      _currentTime_ms += 33;
#       endif

      MARK_NEXT_TIME_PROFILE(Anim, WHILE);

      // Keep reading until we hit another audio type
      bool nextAudioFrameFound = false;
      bool terminatorFound = false;
      while(!nextAudioFrameFound && !terminatorFound)
      {
        if(_currentBufferPos == _lastBufferPos) {
          // We should not be here if there isn't at least another audio sample,
          // silence, or end-of-animation keyframe in the buffer to find.
          // (Note that IsReadyToPlay() checks for there being at least _two_
          //  keyframes in the buffer, where a "keyframe" is considered an
          //  audio sample (or silence) or an end-of-animation indicator.)
          AnkiWarn( 178, "AnimationController.BufferUnexpectedlyEmpty", 451, "Ran out of animation buffer after getting audio/silence.", 0);
          return RESULT_FAIL;
        }

        msgID = PeekBufferTag();

        switch(msgID)
        {
          case RobotInterface::EngineToRobot::Tag_animAudioSample:
          {
            _tracksInUse |= AUDIO_TRACK;
            // Fall through to below...
          }
          case RobotInterface::EngineToRobot::Tag_animAudioSilence:
          {
            nextAudioFrameFound = true;
            break;
          }
          case RobotInterface::EngineToRobot::Tag_animStartOfAnimation:
          {
            GetFromBuffer(&msg);
            _currentTag = msg.animStartOfAnimation.tag;

            RobotInterface::AnimationStarted msg;
            msg.tag = _currentTag;
            RobotInterface::SendMessage(msg);

#           if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 168, "AnimationController.StartOfAnimation", 454, "StartOfAnimation w/ tag=%d", 1,  _currentTag);
#           endif
            break;
          }
          case RobotInterface::EngineToRobot::Tag_animEndOfAnimation:
          {
#           if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 166, "AnimationController.EndOfAnimation", 17, "[t=%dms(%d)] tag %d hit EndOfAnimation", 3,
                    _currentTime_ms, system_get_time(), _currentTag);
#           endif
            GetFromBuffer(&msg);
            terminatorFound = true;

            // Failsafe: make sure body stops moving when we hit end of animation
            // We don't have to do this with lift/head, because they will stop
            // when the controller reaches its desired value.
            if(_tracksInUse & BODY_TRACK) {
              #ifdef TARGET_ESPRESSIF
                // Reusing existing msg struct here to save stack
                msg.tag = RobotInterface::EngineToRobot::Tag_animBodyMotion;
                msg.animBodyMotion.speed = 0;
                msg.animBodyMotion.curvatureRadius_mm = 0;
                RTIP::SendMessage(msg);
              #else
                SteeringController::ExecuteDirectDrive(0, 0);
              #endif
            }

            SendAnimationEnded();
            
            _tracksInUse = 0;
            break;
          }

          case RobotInterface::EngineToRobot::Tag_animHeadAngle:
          {
            GetFromBuffer(&msg);
            if(_tracksToPlay & HEAD_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 381, "AnimationController.RequestHeadAngle", 18, "[t=%dms(%d)] requesting head angle of %ddeg over %.2fsec", 4,
                    _currentTime_ms, system_get_time(),
                    msg.animHeadAngle.angle_deg, static_cast<f32>(msg.animHeadAngle.time_ms)*.001f);
#               endif
              #ifdef TARGET_ESPRESSIF
                RTIP::SendMessage(msg);
              #else
                HeadController::SetDesiredAngleByDuration(DEG_TO_RAD_F32(static_cast<f32>(msg.animHeadAngle.angle_deg)), 0.1f, 0.1f,
                                                static_cast<f32>(msg.animHeadAngle.time_ms)*.001f);
              #endif
              _tracksInUse |= HEAD_TRACK;
            }
            break;
          }

          case RobotInterface::EngineToRobot::Tag_animLiftHeight:
          {
            GetFromBuffer(&msg);
            if(_tracksToPlay & LIFT_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 382, "AnimationController.RequestLiftHeight", 19, "[t=%dms(%d)] requesting lift height of %dmm over %.2fsec", 4,
                    _currentTime_ms, system_get_time(),
                    msg.animLiftHeight.height_mm, static_cast<f32>(msg.animLiftHeight.time_ms)*.001f);
#               endif

              #ifdef TARGET_ESPRESSIF
                RTIP::SendMessage(msg);
              #else
                LiftController::SetDesiredHeightByDuration(static_cast<f32>(msg.animLiftHeight.height_mm), 0.1f, 0.1f,
                                                           static_cast<f32>(msg.animLiftHeight.time_ms)*.001f);
              #endif
              _tracksInUse |= LIFT_TRACK;
            }
            break;
          }

          case RobotInterface::EngineToRobot::Tag_animBackpackLights:
          {
            GetFromBuffer(&msg);

            if(_tracksToPlay & BACKPACK_LIGHTS_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 383, "AnimationController.SetBackpackLEDs", 20, "[t=%dms(%d)] setting backpack LEDs.", 2,
                    _currentTime_ms, system_get_time());
#               endif

              #ifdef TARGET_ESPRESSIF
                {
                  MAKE_RTIP_MSG(output);

                  // Update spine lights
                  output.tag = RobotInterface::EngineToRobot::Tag_setBackpackLightsMiddle;
                  output.setBackpackLightsMiddle.layer = BPL_ANIMATION;

                  output.setBackpackLightsMiddle.lights[0].onColor = 
                  output.setBackpackLightsMiddle.lights[0].offColor = msg.animBackpackLights.colors[1];
                  output.setBackpackLightsMiddle.lights[1].onColor = 
                  output.setBackpackLightsMiddle.lights[1].offColor = msg.animBackpackLights.colors[2];
                  output.setBackpackLightsMiddle.lights[2].onColor = 
                  output.setBackpackLightsMiddle.lights[2].offColor = msg.animBackpackLights.colors[3];

                  RTIP::SendMessage(output);

                  // Update turn signals
                  output.tag = RobotInterface::EngineToRobot::Tag_setBackpackLightsTurnSignals;
                  output.setBackpackLightsTurnSignals.layer = BPL_ANIMATION;

                  output.setBackpackLightsTurnSignals.lights[0].onColor = 
                  output.setBackpackLightsTurnSignals.lights[0].offColor = msg.animBackpackLights.colors[0];
                  output.setBackpackLightsTurnSignals.lights[1].onColor = 
                  output.setBackpackLightsTurnSignals.lights[1].offColor = msg.animBackpackLights.colors[4];

                  RTIP::SendMessage(output);

                  // Set current layer
                  output.tag = RobotInterface::EngineToRobot::Tag_setBackpackLayer;
                  output.setBackpackLayer.layer = BPL_ANIMATION;
        
                  RTIP::SendMessage(output);
                }
              #else
                // TODO: Backpack animation controller in simulator
              #endif
              _tracksInUse |= BACKPACK_LIGHTS_TRACK;
            }
            break;
          }

          case RobotInterface::EngineToRobot::Tag_animFaceImage:
          {
            GetFromBuffer(&msg);

            if(_tracksToPlay & FACE_IMAGE_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 384, "AnimationController.SetFaceFrame", 21, "[t=%dms(%d)] setting face frame.", 2,
                    _currentTime_ms, system_get_time());
#               endif

              HAL::FaceAnimate(msg.animFaceImage.image, msg.animFaceImage.image_length);

              _tracksInUse |= FACE_IMAGE_TRACK;
            }
            break;
          }

          case RobotInterface::EngineToRobot::Tag_animEvent:
          {
            GetFromBuffer(&msg);

            if(_tracksToPlay & EVENT_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 385, "AnimationController.event", 456, "[t=%dms(%d)] event %d.", 3,
                    _currentTime_ms, system_get_time(), msg.tag);
#               endif

              #ifdef TARGET_ESPRESSIF
              msg.tag = RobotInterface::EngineToRobot::Tag_animEventToRTIP;
              msg.animEventToRTIP.event_id = msg.animEvent.event_id;
              msg.animEventToRTIP.tag = _currentTag;
              RTIP::SendMessage(msg);
              #else
              RobotInterface::AnimationEvent emsg;
              emsg.timestamp = HAL::GetTimeStamp();
              emsg.event_id = msg.animEvent.event_id;
              emsg.tag = _currentTag;
              RobotInterface::SendMessage(emsg);
              #endif
              
              _tracksInUse |= EVENT_TRACK;
            }
            break;
          }

          case RobotInterface::EngineToRobot::Tag_animBodyMotion:
          {
            GetFromBuffer(&msg);

            if(_tracksToPlay & BODY_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 386, "AnimationController.SetBodyMotion", 24, "[t=%dms(%d)] setting body motion to radius=%d, speed=%d", 4,
                    _currentTime_ms, system_get_time(), msg.animBodyMotion.curvatureRadius_mm,
                    msg.animBodyMotion.speed);
#               endif

              _tracksInUse |= BODY_TRACK;

              #ifdef TARGET_ESPRESSIF
              RTIP::SendMessage(msg);
              #else
              SteeringController::ExecuteDriveCurvature(msg.animBodyMotion.speed,
                                                        msg.animBodyMotion.curvatureRadius_mm);
              #endif
            }
            break;
          }

          default:
          {
            AnkiWarn( 167, "AnimationController.UnexpectedTag", 478, "Unexpected message type %d in animation buffer!", 1, msgID);
            return RESULT_FAIL;
          }

        } // switch
      } // while(!nextAudioFrameFound && !terminatorFound)

      ++_numAudioFramesPlayed;
      --_numAudioFramesBuffered;

      if(terminatorFound) {
        _isPlaying = false;
        AnkiAssert(_haveReceivedTerminationFrame > 0, 281);
        _haveReceivedTerminationFrame--;
        --_numAudioFramesBuffered;
        ++_numAudioFramesPlayed; // end of anim considered "audio" for counting
#         if DEBUG_ANIMATION_CONTROLLER
        AnkiDebug( 387, "AnimationController.termination", 26, "Reached animation %d termination frame (%d frames still buffered, curPos/lastPos = %d/%d).", 4,
              _currentTag, _numAudioFramesBuffered, _currentBufferPos, _lastBufferPos);
#         endif
        _currentTag = 0;
      }

      // Print time profile stats
      END_TIME_PROFILE(Anim);
      PERIODIC_PRINT_AND_RESET_TIME_PROFILE(Anim, 120);


    } // if AdvanceAudio

    if (_animCtrlState == ENABLED)
    {
      const u32 now = system_get_time();
      if ((now - _lastStateSendTime) > ANIM_STATE_INTERVAL_us)
      {
        if (AnimationController::SendAnimStateMessage() == RESULT_OK)
        {
          _lastStateSendTime = now;
        }
      }
    }

    return RESULT_OK;
  } // Update()

  void EnableTracks(u8 whichTracks)
  {
    _tracksToPlay |= whichTracks;
  }

  void DisableTracks(u8 whichTracks)
  {
    // Don't play disabled tracks
    _tracksToPlay &= ~whichTracks;
    
    // Disabled tracks are no longer "in use"
    _tracksInUse &= ~whichTracks;
    
    // Actually stop motors for head/lift/body if they are being disabled (i.e. are now "locked")
    StopTracks(whichTracks);
  }
  
  u8 GetEnabledTracks()
  {
    return _tracksToPlay;
  }

  u8 GetCurrentTag()
  {
    return _currentTag;
  }

  s32 SuspendAndGetBuffer(u8** buffer)
  {
    _animCtrlState = SUSPENDED;
    Clear();
    *buffer = _keyFrameBuffer;
    return KEYFRAME_BUFFER_SIZE;
  }
  
  void ResumeAndRestoreBuffer()
  {
    _animCtrlState = DISABLED;
  }

} // namespace AnimationController
} // namespace Cozmo
} // namespace Anki
