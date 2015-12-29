#include "animationController.h"
#include "clad/types/animationKeyFrames.h"
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
#define ONCHIP

#else // Not on Espressif
#include <assert.h>
#include "anki/cozmo/robot/hal.h"
#include "headController.h"
#include "liftController.h"
#include "localization.h"
#include "wheelController.h"
#include "steeringController.h"
#include "speedController.h"
static inline u32 system_get_time() { return Anki::Cozmo::HAL::GetTimeStamp(); }
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
    ONCHIP u8 _keyFrameBuffer[KEYFRAME_BUFFER_SIZE];
    s32 _currentBufferPos;
    s32 _lastBufferPos;

    s32 _numAudioFramesBuffered; // NOTE: Also counts EndOfAnimationFrames...
    s32 _numBytesPlayed = 0;
    s32 _numAudioFramesPlayed = 0;

    u8  _currentTag = 0;

    bool _isBufferStarved;
    u16 _haveReceivedTerminationFrame;
    bool _isPlaying;
    bool _bufferFullMessagePrintedThisTick;

    s32 _tracksToPlay;

    int _tracksInUse = 0;

    s16 _audioReadInd;
    bool _playSilence;


#   if DEBUG_ANIMATION_CONTROLLER
    TimeStamp_t _currentTime_ms;
#   endif

  } // "private" members

  Result Init()
  {
#   if DEBUG_ANIMATION_CONTROLLER
    AnkiDebug( 15, "AnimationController", 84, "Initializing", 0);
#   endif

    _tracksToPlay = ENABLE_ALL_TRACKS;

    Clear();

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
    if(_lastBufferPos >= _currentBufferPos) {
      return (_lastBufferPos - _currentBufferPos);
    } else {
      return KEYFRAME_BUFFER_SIZE - (_currentBufferPos - _lastBufferPos);
    }
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

  void StopTracksInUse()
  {
    if(_tracksInUse) {
      // In case we are aborting an animation, stop any tracks that were in use
      // (For now, this just means motor-based tracks.) Note that we don't
      // stop tracks we weren't using, in case we were, for example, playing
      // a head animation while driving a path.
      if(_tracksInUse & HEAD_TRACK) {
#ifdef TARGET_ESPRESSIF
        // Don't have an API for this on Espressif right now, PUNT
#else
        HeadController::SetAngularVelocity(0);
#endif
      }
      if(_tracksInUse & LIFT_TRACK) {
#ifdef TARGET_ESPRESSIF
        // Don't have an API for this on Espressif right now, PUNT
#else
        LiftController::SetAngularVelocity(0);
#endif
      }
      if(_tracksInUse & BODY_TRACK) {
#ifdef TARGET_ESPRESSIF
        RobotInterface::EngineToRobot msg;
        msg.tag = RobotInterface::EngineToRobot::Tag_animBodyMotion;
        msg.animBodyMotion.speed = 0;
        msg.animBodyMotion.curvatureRadius_mm = 0;
        RTIP::SendMessage(msg);
#else
        SteeringController::ExecuteDirectDrive(0, 0);
#endif
      }
    }
    _tracksInUse = 0;
  }

  void Clear()
  {
#   if DEBUG_ANIMATION_CONTROLLER
    AnkiDebug( 15, "AnimationController", 85, "Clearing", 0);
#   endif

    _numBytesPlayed += GetNumBytesInBuffer();
    _numAudioFramesPlayed += _numAudioFramesBuffered;

    _currentBufferPos = 0;
    _lastBufferPos = 0;
    _currentTag = 0;

    _numAudioFramesBuffered = 0;

    _haveReceivedTerminationFrame = 0;
    _isPlaying = false;
    _isBufferStarved = false;
    _bufferFullMessagePrintedThisTick = false;
    _audioReadInd = 0;
    _playSilence = false;

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
    msg.status = 0;
    if (IsBufferFull()) msg.status |= IS_ANIM_BUFFER_FULL;
    if (_isPlaying) msg.status     |= IS_ANIMATING;
    msg.tag = GetCurrentTag();
    if (Anki::Cozmo::HAL::RadioSendMessage(msg.GetBuffer(), msg.Size(), RobotInterface::RobotToEngine::Tag_animState, false, false))
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
    return _keyFrameBuffer[_currentBufferPos];
  }

  static u32 GetFromBuffer(u8* data, u32 numBytes)
  {
    AnkiAssert(numBytes < KEYFRAME_BUFFER_SIZE, 86);
    
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

    // Increment total number of bytes played since startup
    _numBytesPlayed += numBytes;

    return numBytes;
  }

  static s32 GetFromBuffer(RobotInterface::EngineToRobot* msg)
  {
    s32 readSoFar;
    memset(msg, 0, sizeof(RobotInterface::EngineToRobot)); // Memset 0, presumably sets all size fields to 0
    readSoFar  = GetFromBuffer(msg->GetBuffer(), RobotInterface::EngineToRobot::MIN_SIZE); // Read in enough to know what it is
    readSoFar += GetFromBuffer(msg->GetBuffer() + readSoFar, msg->Size() - readSoFar); // Read in the minimum size for the type to get length fields
    readSoFar += GetFromBuffer(msg->GetBuffer() + readSoFar, msg->Size() - readSoFar); // Read in anything left now that we know how big minimum fields are
    return readSoFar;
  }

  Result BufferKeyFrame(const RobotInterface::EngineToRobot& msg)
  {
    const s32 numBytesAvailable = GetNumBytesAvailable();
    const s32 numBytesNeeded = msg.Size();
    if(numBytesAvailable < numBytesNeeded) {
      // Only print the error message if we haven't already done so this tick,
      // to prevent spamming that could clog reliable UDP
      if(!_bufferFullMessagePrintedThisTick) {
        AnkiDebug( 15, "AnimationController", 87, "BufferKeyFrame.BufferFull %d bytes available, %d needed.", 2,
                  numBytesAvailable, numBytesNeeded);
        _bufferFullMessagePrintedThisTick = true;
      }
      return RESULT_FAIL;
    }
#if DEBUG_ANIMATION_CONTROLLER > 1
    AnkiDebug( 15, "AnimationController", 88, "BufferKeyFrame, %d -> %d (%d)", 3, numBytesNeeded, _lastBufferPos, numBytesAvailable);
#endif

    AnkiAssert(numBytesNeeded < KEYFRAME_BUFFER_SIZE, 89);

    if(_lastBufferPos + numBytesNeeded < KEYFRAME_BUFFER_SIZE) {
      // There's enough room from current end position to end of buffer to just
      // copy directly
      memcpy(_keyFrameBuffer + _lastBufferPos, msg.GetBuffer(), numBytesNeeded);
      _lastBufferPos += numBytesNeeded;
    } else {
      // Copy the first chunk into whatever fits from current position to end of
      // the buffer
      const s32 firstChunk = KEYFRAME_BUFFER_SIZE - _lastBufferPos;
      memcpy(_keyFrameBuffer + _lastBufferPos, msg.GetBuffer(), firstChunk);

      // Copy the remaining data starting at the beginning of the buffer
      memcpy(_keyFrameBuffer, msg.GetBuffer()+firstChunk, numBytesNeeded - firstChunk);
      _lastBufferPos = numBytesNeeded-firstChunk;
     }
    switch(msg.tag) {
      case RobotInterface::EngineToRobot::Tag_animEndOfAnimation:
        ++_haveReceivedTerminationFrame;
      case RobotInterface::EngineToRobot::Tag_animAudioSample:
      case RobotInterface::EngineToRobot::Tag_animAudioSilence:
        ++_numAudioFramesBuffered;
        break;
        default:
        break;
    }

    AnkiAssert(_lastBufferPos >= 0 && _lastBufferPos < KEYFRAME_BUFFER_SIZE, 90);

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
          AnkiWarn( 15, "AnimationController", 91, "IsReadyToPlay.BufferStarved", 0);
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

    //AnkiAssert(_currentFrame <= _lastFrame, 92);

    return ready;
  } // IsReadyToPlay()

  extern "C" bool PumpAudioData(uint8_t* dest)
  {
    if(IsReadyToPlay()) {
      #ifdef TARGET_ESPRESSIF
      if (_audioReadInd == 0)
      #else
      if (HAL::AudioReady())
      #endif
      {
        START_TIME_PROFILE(Anim, AUDIOPLAY);

        // Next thing in the buffer should be audio or silence:
        RobotInterface::EngineToRobot::Tag msgID = PeekBufferTag();
        RobotInterface::EngineToRobot msg;

        // If the next message is not audio, then delete it until it is.
        while(msgID != RobotInterface::EngineToRobot::Tag_animAudioSilence &&
              msgID != RobotInterface::EngineToRobot::Tag_animAudioSample) {
          AnkiWarn( 15, "AnimationController", 93, "Expecting either audio sample or silence next in animation buffer. (Got 0x%02x instead). Dumping message. (FYI AudioSample_ID = 0x%02x)", 2, msgID, RobotInterface::EngineToRobot::Tag_animAudioSample);
          GetFromBuffer(&msg);
          msgID = PeekBufferTag();
        }

        switch(msgID)
        {
          case RobotInterface::EngineToRobot::Tag_animAudioSilence:
          {
            GetFromBuffer(&msg);
            _playSilence = true;
            #ifdef TARGET_ESPRESSIF
              _audioReadInd = MAX_AUDIO_BYTES_PER_DROP;
              return false; // Do not play audio
            #else
              HAL::AudioPlaySilence();
              return true; // Advance animation
            #endif
          }
          case RobotInterface::EngineToRobot::Tag_animAudioSample:
          {
            if(_tracksToPlay & AUDIO_TRACK) {
              _playSilence = false;
              #ifdef TARGET_ESPRESSIF
                u8 tag;
                GetFromBuffer(&tag, 1); // Get the audio sample header out
                GetFromBuffer(dest, MAX_AUDIO_BYTES_PER_DROP); // Get the first MAX_AUDIO_BYTES_PER_DROP from the buffer
                _audioReadInd = MAX_AUDIO_BYTES_PER_DROP;
                return true; // Play audio
              #else
                GetFromBuffer(&msg);
                HAL::AudioPlayFrame(&msg.animAudioSample);
                return true; // Advance animation
              #endif
            } else {
              GetFromBuffer(&msg);
              _playSilence = true;
              #ifdef TARGET_ESPRESSIF
                _audioReadInd = MAX_AUDIO_BYTES_PER_DROP;
                return false; // Do not play audio
              #else
                HAL::AudioPlaySilence();
                return true; // Advance animation
              #endif
            }
          }
          default:
          {
            AnkiWarn( 15, "AnimationController", 94, "Expecting either audio sample or silence next in animation buffer. (Got 0x%02x instead)", 1, msgID);
            Clear();
            return false;
          }
        }
      }
      else
      {
#ifdef TARGET_ESPRESSIF
        if (!_playSilence)
        {
          if (_audioReadInd == AUDIO_SAMPLE_SIZE-2) // XXX Temporary hack for EP1 compatibility until we can adjust the audio chunk size
          {
            GetFromBuffer(dest, MAX_AUDIO_BYTES_PER_DROP-1);
            dest[MAX_AUDIO_BYTES_PER_DROP-1] = dest[MAX_AUDIO_BYTES_PER_DROP-2];
          }
          else
          {
            GetFromBuffer(dest, MAX_AUDIO_BYTES_PER_DROP); // Get the next MAX_AUDIO_BYTES_PER_DROP from the buffer
          }
        }
        _audioReadInd += MAX_AUDIO_BYTES_PER_DROP;
        if (_audioReadInd >= AUDIO_SAMPLE_SIZE)
        {
          --_numAudioFramesBuffered;
          ++_numAudioFramesPlayed;
#         if DEBUG_ANIMATION_CONTROLLER
          AnkiDebug( 15, "AnimationController", 95, "Update()\tari = %d\tnafb = %d", 2, _audioReadInd, _numAudioFramesBuffered);
#         endif
          _audioReadInd = 0;
          Update(); // Done with audio message, grab next thing from buffer
#         if DEBUG_ANIMATION_CONTROLLER
          _currentTime_ms += 33;
#         endif
        }
        return !_playSilence; // Return whether we are playing audio or not
#else
        return false; // Do not advance animation
#endif
      }
    }
    else
    {
      return false; // Do not advance animation
    }
  }

  Result Update()
  {
#ifndef TARGET_ESPRESSIF
    if (PumpAudioData(NULL))
#endif
    {
      RobotInterface::EngineToRobot msg;
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
          AnkiWarn( 15, "AnimationController", 96, "Ran out of animation buffer after getting audio/silence.", 0);
          return RESULT_FAIL;
        }

        msgID = PeekBufferTag();

        switch(msgID)
        {
          case RobotInterface::EngineToRobot::Tag_animAudioSample:
          {
            _tracksInUse |= BACKPACK_LIGHTS_TRACK;
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

#             if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 15, "AnimationController", 97, "StartOfAnimation w/ tag=%d", 1, _currentTag);
#           endif
            break;
          }
          case RobotInterface::EngineToRobot::Tag_animEndOfAnimation:
          {
#           if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 15, "AnimationController", 98, "[t=%dms(%d)] hit EndOfAnimation", 2,
                    _currentTime_ms, system_get_time());
#           endif
            GetFromBuffer(&msg);
            terminatorFound = true;

            // Failsafe: make sure body stops moving when we hit end of animation
            // We don't have to do this with lift/head, because they will stop
            // when the controller reaches its desired value.
            if(_tracksInUse & BODY_TRACK) {
              #ifdef TARGET_ESPRESSIF
                RobotInterface::EngineToRobot msg;
                msg.tag = RobotInterface::EngineToRobot::Tag_animBodyMotion;
                msg.animBodyMotion.speed = 0;
                msg.animBodyMotion.curvatureRadius_mm = 0;
                RTIP::SendMessage(msg);
              #else
                SteeringController::ExecuteDirectDrive(0, 0);
              #endif
            }

            RobotInterface::AnimationEnded aem;
            aem.tag = _currentTag;
            RobotInterface::SendMessage(aem);

            _tracksInUse = 0;
            break;
          }

          case RobotInterface::EngineToRobot::Tag_animHeadAngle:
          {
            GetFromBuffer(&msg);
            if(_tracksToPlay & HEAD_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 15, "AnimationController", 99, "[t=%dms(%d)] requesting head angle of %ddeg over %.2fsec", 4,
                    _currentTime_ms, system_get_time(),
                    msg.animHeadAngle.angle_deg, static_cast<f32>(msg.animHeadAngle.time_ms)*.001f);
#               endif
              #ifdef TARGET_ESPRESSIF
                RTIP::SendMessage(msg);
              #else
                HeadController::SetDesiredAngle(DEG_TO_RAD(static_cast<f32>(msg.animHeadAngle.angle_deg)), 0.1f, 0.1f,
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
              AnkiDebug( 15, "AnimationController", 100, "[t=%dms(%d)] requesting lift height of %dmm over %.2fsec", 4,
                    _currentTime_ms, system_get_time(),
                    msg.animLiftHeight.height_mm, static_cast<f32>(msg.animLiftHeight.time_ms)*.001f);
#               endif

              #ifdef TARGET_ESPRESSIF
                RTIP::SendMessage(msg);
              #else
                LiftController::SetDesiredHeight(static_cast<f32>(msg.animLiftHeight.height_mm), 0.1f, 0.1f,
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
              AnkiDebug( 15, "AnimationController", 101, "[t=%dms(%d)] setting backpack LEDs.", 2,
                    _currentTime_ms, system_get_time());
#               endif

              #ifdef TARGET_ESPRESSIF
                RTIP::SendMessage(msg);
              #else
                for(s32 iLED=0; iLED<NUM_BACKPACK_LEDS; ++iLED) {
                  HAL::SetLED(static_cast<LEDId>(iLED), msg.animBackpackLights.colors[iLED]);
                }
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
              AnkiDebug( 15, "AnimationController", 102, "[t=%dms(%d)] setting face frame.", 2,
                    _currentTime_ms, system_get_time());
#               endif

              #ifdef TARGET_ESPRESSIF
                Face::Update(msg.animFaceImage);
              #else
                HAL::FaceAnimate(msg.animFaceImage.image);
              #endif

              _tracksInUse |= FACE_IMAGE_TRACK;
            }
            break;
          }

          case RobotInterface::EngineToRobot::Tag_animFacePosition:
          {
            GetFromBuffer(&msg);

            if(_tracksToPlay & FACE_POS_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 15, "AnimationController", 103, "[t=%dms(%d)] setting face position to (%d,%d).", 4,
                    _currentTime_ms, system_get_time(), msg.animFacePosition.xCen, msg.animFacePosition.yCen);
#               endif

              #ifdef TARGET_ESPRESSIF
                Face::Move(msg.animFacePosition.xCen, msg.animFacePosition.yCen);
              #else
                HAL::FaceMove(msg.animFacePosition.xCen, msg.animFacePosition.yCen);
              #endif

              _tracksInUse |= FACE_POS_TRACK;
            }
            break;
          }

          case RobotInterface::EngineToRobot::Tag_animBlink:
          {
            GetFromBuffer(&msg);

            if(_tracksToPlay & BLINK_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 15, "AnimationController", 104, "[t=%dms(%d)] Blinking.", 2,
                    _currentTime_ms, system_get_time());
#               endif

              if(msg.animBlink.blinkNow) {
              #ifdef TARGET_ESPRESSIF
                Face::Blink();
              #else
                HAL::FaceBlink();
              } else {
                if(msg.animBlink.enable) {
                  //EyeController::Enable();
                } else {
                  //EyeController::Disable();
                }
              #endif
              }
              _tracksInUse |= BLINK_TRACK;
            }
            break;
          }

          case RobotInterface::EngineToRobot::Tag_animBodyMotion:
          {
            GetFromBuffer(&msg);

            if(_tracksToPlay & BODY_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
              AnkiDebug( 15, "AnimationController", 105, "[t=%dms(%d)] setting body motion to radius=%d, speed=%d", 4,
                    _currentTime_ms, system_get_time(), msg.animBodyMotion.curvatureRadius_mm,
                    msg.animBodyMotion.speed);
#               endif

              _tracksInUse |= BODY_TRACK;

              #ifdef TARGET_ESPRESSIF
              RTIP::SendMessage(msg);
              #else
              f32 leftSpeed=0, rightSpeed=0;
              if(msg.animBodyMotion.speed == 0) {
                // Stop
                leftSpeed = 0.f;
                rightSpeed = 0.f;
              } else if(msg.animBodyMotion.curvatureRadius_mm == s16_MAX ||
                        msg.animBodyMotion.curvatureRadius_mm == s16_MIN) {
                // Drive straight
                leftSpeed  = static_cast<f32>(msg.animBodyMotion.speed);
                rightSpeed = static_cast<f32>(msg.animBodyMotion.speed);
              } else if(msg.animBodyMotion.curvatureRadius_mm == 0) {
                SteeringController::ExecutePointTurn(DEG_TO_RAD_F32(msg.animBodyMotion.speed), 50);
                break;

              } else {
                // Drive an arc

                //if speed is positive, the left wheel should turn slower, so
                // it becomes the INNER wheel
                leftSpeed = static_cast<f32>(msg.animBodyMotion.speed) * (1.0f - WHEEL_DIST_HALF_MM / static_cast<f32>(msg.animBodyMotion.curvatureRadius_mm));

                //if speed is positive, the right wheel should turn faster, so
                // it becomes the OUTER wheel
                rightSpeed = static_cast<f32>(msg.animBodyMotion.speed) * (1.0f + WHEEL_DIST_HALF_MM / static_cast<f32>(msg.animBodyMotion.curvatureRadius_mm));
              }

              SteeringController::ExecuteDirectDrive(leftSpeed, rightSpeed);
              #endif
            }
            break;
          }

          default:
          {
            AnkiWarn( 15, "AnimationController", 106, "Unexpected message type %d in animation buffer!", 1, msgID);
            return RESULT_FAIL;
          }

        } // switch
      } // while(!nextAudioFrameFound && !terminatorFound)

      #ifndef TARGET_ESPRESSIF
        ++_numAudioFramesPlayed;
        --_numAudioFramesBuffered;
      #endif

      if(terminatorFound) {
        _isPlaying = false;
        assert(_haveReceivedTerminationFrame > 0);
        _haveReceivedTerminationFrame--;
        --_numAudioFramesBuffered;
        ++_numAudioFramesPlayed; // end of anim considered "audio" for counting
#         if DEBUG_ANIMATION_CONTROLLER
        AnkiDebug( 15, "AnimationController", 107, "Reached animation %d termination frame (%d frames still buffered, curPos/lastPos = %d/%d).", 4,
              _currentTag, _numAudioFramesBuffered, _currentBufferPos, _lastBufferPos);
#         endif
        _currentTag = 0;
      }

      // Print time profile stats
      END_TIME_PROFILE(Anim);
      PERIODIC_PRINT_AND_RESET_TIME_PROFILE(Anim, 120);


    } // if PumpAudio

    return RESULT_OK;
  } // Update()

  void EnableTracks(u8 whichTracks)
  {
    _tracksToPlay |= whichTracks;
  }

  void DisableTracks(u8 whichTracks)
  {
    _tracksToPlay &= ~whichTracks;
  }

  u8 GetCurrentTag()
  {
    return _currentTag;
  }

} // namespace AnimationController
} // namespace Cozmo
} // namespace Anki
