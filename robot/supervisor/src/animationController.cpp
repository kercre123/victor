#include "animationController.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"
#include "anki/common/shared/velocityProfileGenerator.h"

#include "eyeController.h"
#include "headController.h"
#include "liftController.h"
#include "localization.h"
#include "wheelController.h"
#include "steeringController.h"
#include "speedController.h"


#define DEBUG_ANIMATION_CONTROLLER 0

#define USE_HARDCODED_ANIMATIONS 0

namespace Anki {
namespace Cozmo {
namespace AnimationController {
  
  namespace {
    
    // Streaming KeyFrame buffer size, in bytes
    static const s32 KEYFRAME_BUFFER_SIZE = 16384;
    
    // Amount of "pre-roll" that needs to be buffered before a streamed animation
    // will start playing, in number of keyframes
    static const s32 PREROLL_BUFFER_LENGTH = 5;
    
    // If buffer gets within this number of bytes of the buffer length,
    // then it is considered "full" for the purposes of IsBufferFull() below.
    static const s32 KEYFRAME_BUFFER_PADDING = KEYFRAME_BUFFER_SIZE / 4;
    
    // Circular byte buffer for keyframe messages
    ONCHIP u8 _keyFrameBuffer[KEYFRAME_BUFFER_SIZE];
    s32 _currentBufferPos;
    s32 _lastBufferPos;
    
    s32  _numFramesBuffered;
    
    bool _haveReceivedTerminationFrame;
    bool _isPlaying;
    bool _bufferFullMessagePrintedThisTick;
    
    AnimTrackFlag _tracksToPlay;
    
#   if DEBUG_ANIMATION_CONTROLLER
    TimeStamp_t _currentTime_ms;
#   endif

  } // "private" members
  
  static void DefineHardCodedAnimations()
  {
#if USE_HARDCODED_ANIMATIONS
    //
    // FAST HEAD NOD - 3 fast nods
    //
    {
      ClearCannedAnimation(ANIM_HEAD_NOD);
      KeyFrame kf;
      kf.transitionIn  = KF_TRANSITION_LINEAR;
      kf.transitionOut = KF_TRANSITION_LINEAR;
      
      
      // Start the nod
      kf.type = KeyFrame::START_HEAD_NOD;
      kf.relTime_ms = 0;
      kf.StartHeadNod.lowAngle  = DEG_TO_RAD(-10);
      kf.StartHeadNod.highAngle = DEG_TO_RAD( 10);
      kf.StartHeadNod.period_ms = 600;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD);
      
      // Stop the nod
      kf.type = KeyFrame::STOP_HEAD_NOD;
      kf.relTime_ms = 1500;
      kf.StopHeadNod.finalAngle = 0.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD);
      
    } // FAST HEAD NOD
    
    //
    // SLOW HEAD NOD - 2 slow nods
    //
    {
      ClearCannedAnimation(ANIM_HEAD_NOD_SLOW);
      KeyFrame kf;
      kf.transitionIn  = KF_TRANSITION_LINEAR;
      kf.transitionOut = KF_TRANSITION_LINEAR;
      
      // Start the nod
      kf.type = KeyFrame::START_HEAD_NOD;
      kf.relTime_ms = 0;
      kf.StartHeadNod.lowAngle  = DEG_TO_RAD(-25);
      kf.StartHeadNod.highAngle = DEG_TO_RAD( 25);
      kf.StartHeadNod.period_ms = 1200;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD_SLOW);
      
      // Stop the nod
      kf.type = KeyFrame::STOP_HEAD_NOD;
      kf.relTime_ms = 2400;
      kf.StopHeadNod.finalAngle = 0.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_HEAD_NOD_SLOW);
      
    } // SLOW HEAD NOD
    
    
    //
    // BLINK
    //
    {
      ClearCannedAnimation(ANIM_BLINK);
      KeyFrame kf;
      kf.type = KeyFrame::SET_LED_COLORS;
      kf.transitionIn  = KF_TRANSITION_INSTANT;
      kf.transitionOut = KF_TRANSITION_INSTANT;
      
      // Start with all eye segments on:
      kf.relTime_ms = 0;
      kf.SetLEDcolors.led[LED_LEFT_EYE_BOTTOM]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_LEFT]    = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_TOP]     = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_BOTTOM] = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_TOP]    = LED_BLUE;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK);
      
      // Turn off top/bottom segments first
      kf.relTime_ms = 1700;
      kf.SetLEDcolors.led[LED_LEFT_EYE_BOTTOM]  = LED_OFF;
      kf.SetLEDcolors.led[LED_LEFT_EYE_LEFT]    = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_TOP]     = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_BOTTOM] = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_TOP]    = LED_OFF;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK);
      
      // Turn off all segments shortly thereafter
      kf.relTime_ms = 1750;
      kf.SetLEDcolors.led[LED_LEFT_EYE_BOTTOM]  = LED_OFF;
      kf.SetLEDcolors.led[LED_LEFT_EYE_LEFT]    = LED_OFF;
      kf.SetLEDcolors.led[LED_LEFT_EYE_RIGHT]   = LED_OFF;
      kf.SetLEDcolors.led[LED_LEFT_EYE_TOP]     = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_BOTTOM] = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_LEFT]   = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_RIGHT]  = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_TOP]    = LED_OFF;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK);
      
      // Turn on left/right segments first
      kf.relTime_ms = 1850;
      kf.SetLEDcolors.led[LED_LEFT_EYE_BOTTOM]  = LED_OFF;
      kf.SetLEDcolors.led[LED_LEFT_EYE_LEFT]    = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_TOP]     = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_BOTTOM] = LED_OFF;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_TOP]    = LED_OFF;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK);
      
      // Turn on all segments shortly thereafter
      kf.relTime_ms = 1900;
      kf.SetLEDcolors.led[LED_LEFT_EYE_BOTTOM]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_LEFT]    = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_RIGHT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_LEFT_EYE_TOP]     = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_BOTTOM] = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_LEFT]   = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_RIGHT]  = LED_BLUE;
      kf.SetLEDcolors.led[LED_RIGHT_EYE_TOP]    = LED_BLUE;
      AddKeyFrameToCannedAnimation(kf, ANIM_BLINK);
    } // BLINK
    
    //
    // Up/Down/Left/Right
    //  Move lift and head up and down (in opposite directions)
    //  Then turn to the left and the to the right
    {
      ClearCannedAnimation(ANIM_UPDOWNLEFTRIGHT);
      KeyFrame kf;
      
      // Move head up
      kf.type = KeyFrame::HEAD_ANGLE;
      kf.relTime_ms = 0;
      kf.SetHeadAngle.targetAngle = DEG_TO_RAD(25.f);
      kf.SetHeadAngle.targetSpeed = 5.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
      // Move lift down
      kf.type = KeyFrame::LIFT_HEIGHT;
      kf.relTime_ms = 0;
      kf.SetLiftHeight.targetHeight = 0.f;
      kf.SetLiftHeight.targetSpeed  = 50.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
      // Move head down
      kf.type = KeyFrame::HEAD_ANGLE;
      kf.relTime_ms = 750;
      kf.SetHeadAngle.targetAngle = DEG_TO_RAD(-25.f);
      kf.SetHeadAngle.targetSpeed = 5.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
      // Move lift up
      kf.type = KeyFrame::LIFT_HEIGHT;
      kf.relTime_ms = 750;
      kf.SetLiftHeight.targetHeight = 75.f;
      kf.SetLiftHeight.targetSpeed  = 50.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
      // Turn left
      kf.type = KeyFrame::POINT_TURN;
      kf.relTime_ms = 1250;
      kf.TurnInPlace.relativeAngle = DEG_TO_RAD(-45);
      kf.TurnInPlace.targetSpeed = 100.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);

      // Turn right
      kf.type = KeyFrame::POINT_TURN;
      kf.relTime_ms = 2250;
      kf.TurnInPlace.relativeAngle = DEG_TO_RAD(90);
      kf.TurnInPlace.targetSpeed = 100.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
      // Turn back to center
      kf.type = KeyFrame::POINT_TURN;
      kf.relTime_ms = 2750;
      kf.TurnInPlace.relativeAngle = DEG_TO_RAD(-45);
      kf.TurnInPlace.targetSpeed = 100.f;
      AddKeyFrameToCannedAnimation(kf, ANIM_UPDOWNLEFTRIGHT);
      
    } // Up/Down/Left/Right
    
    //
    // BACK_AND_FORTH_EXCITED
    //
    {
      ClearCannedAnimation(ANIM_BACK_AND_FORTH_EXCITED);
      KeyFrame kf;
      
      kf.type = KeyFrame::DRIVE_LINE_SEGMENT;
      kf.relTime_ms = 300;
      kf.DriveLineSegment.relativeDistance = -9;
      kf.DriveLineSegment.targetSpeed = 30;
      AddKeyFrameToCannedAnimation(kf, ANIM_BACK_AND_FORTH_EXCITED);
      
      kf.type = KeyFrame::DRIVE_LINE_SEGMENT;
      kf.relTime_ms = 600;
      kf.DriveLineSegment.relativeDistance = 9;
      kf.DriveLineSegment.targetSpeed = 30;
      AddKeyFrameToCannedAnimation(kf, ANIM_BACK_AND_FORTH_EXCITED);
      
    } // BACK_AND_FORTH_EXCITED
    
#endif // USE_HARDCODED_ANIMATIONS
    
  } // DefineHardCodedAnimations()
  
  
  Result Init()
  {
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Initializing AnimationController\n");
#   endif
    
    _tracksToPlay = ENABLE_ALL_TRACKS;
    
    Clear();
    
    DefineHardCodedAnimations();
    
    return RESULT_OK;
  }
  
  
  void Clear()
  {
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Clearing AnimationController\n");
#   endif
    
    _currentBufferPos = 0;
    _lastBufferPos = 0;
    
    _numFramesBuffered = 0;

    _haveReceivedTerminationFrame = false;
    _isPlaying = false;
    _bufferFullMessagePrintedThisTick = false;
    
#   if DEBUG_ANIMATION_CONTROLLER
    _currentTime_ms = 0;
#   endif
    
    // Necessary?
    //memset(_keyFrameBuffer, KEYFRAME_BUFFER_SIZE, sizeof(u8));
  }
 
  static s32 GetNumBytesAvailable()
  {
    if(_lastBufferPos >= _currentBufferPos) {
      return sizeof(_keyFrameBuffer) - (_lastBufferPos - _currentBufferPos);
    } else {
      return _currentBufferPos - _lastBufferPos;
    }
  }
  
  s32 GetApproximateNumBytesFree()
  {
    return MAX(0, GetNumBytesAvailable() - KEYFRAME_BUFFER_PADDING);
  }
  
  static void CopyIntoBuffer(const u8* data, s32 numBytes)
  {
    assert(numBytes < sizeof(_keyFrameBuffer));
    
    if(_lastBufferPos + numBytes < sizeof(_keyFrameBuffer)) {
      // There's enough room from current end position to end of buffer to just
      // copy directly
      memcpy(_keyFrameBuffer + _lastBufferPos, data, numBytes);
      _lastBufferPos += numBytes;
    } else {
      // Copy the first chunk into whatever fits from current position to end of
      // the buffer
      const s32 firstChunk = sizeof(_keyFrameBuffer) - _lastBufferPos;
      memcpy(_keyFrameBuffer + _lastBufferPos, data, firstChunk);
      
      // Copy the remaining data starting at the beginning of the buffer
      memcpy(_keyFrameBuffer, data+firstChunk, numBytes - firstChunk);
      _lastBufferPos = numBytes-firstChunk;
     }
    
    assert(_lastBufferPos >= 0 && _lastBufferPos < sizeof(_keyFrameBuffer));
  }
  
  static void RewindBufferOneType()
  {
    _currentBufferPos -= sizeof(Messages::ID);
    if(_currentBufferPos < 0) {
      _currentBufferPos += sizeof(_keyFrameBuffer);
    }
  }
  
  static inline void SetTypeIndicator(Messages::ID msgType)
  {
    CopyIntoBuffer((u8*)&msgType, sizeof(msgType));
  }
  
  static void GetFromBuffer(u8* data, s32 numBytes)
  {
    assert(numBytes < sizeof(_keyFrameBuffer));
    
    if(_currentBufferPos + numBytes < sizeof(_keyFrameBuffer)) {
      // There's enough room from current position to end of buffer to just
      // copy directly
      memcpy(data, _keyFrameBuffer + _currentBufferPos, numBytes);
      _currentBufferPos += numBytes;
    } else {
      // Copy the first chunk from whatever remains from current position to end of
      // the buffer
      const s32 firstChunk = sizeof(_keyFrameBuffer) - _currentBufferPos;
      memcpy(data, _keyFrameBuffer + _currentBufferPos, firstChunk);
      
      // Copy the remaining data starting at the beginning of the buffer
      memcpy(data+firstChunk, _keyFrameBuffer, numBytes - firstChunk);
      _currentBufferPos = numBytes-firstChunk;
    }
    
    assert(_currentBufferPos >= 0 && _currentBufferPos < sizeof(_keyFrameBuffer));
  }
  
  static inline Messages::ID GetTypeIndicator()
  {
    Messages::ID msgID;
    GetFromBuffer((u8*)&msgID, sizeof(Messages::ID));
    return msgID;
  }
  
  template<typename MSG_TYPE>
  static inline Result BufferKeyFrameHelper(const MSG_TYPE& msg, Messages::ID msgID)
  {
    const s32 numBytesAvailable = GetNumBytesAvailable();
    const s32 numBytesNeeded = sizeof(msg) + sizeof(Messages::ID);
    if(numBytesAvailable < numBytesNeeded) {
      // Only print the error message if we haven't already done so this tick,
      // to prevent spamming that could clog reliable UDP
      if(!_bufferFullMessagePrintedThisTick) {
        AnkiError("AnimationController.BufferKeyFrame.BufferFull",
                  "%d bytes available, %d needed.\n",
                  numBytesAvailable, numBytesNeeded);
        _bufferFullMessagePrintedThisTick = true;
      }
      return RESULT_FAIL;
    }
    SetTypeIndicator(msgID);
    CopyIntoBuffer((u8*)&msg, sizeof(msg));
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_EndOfAnimation& msg)
  {
    Result lastResult = BufferKeyFrameHelper(msg, GET_MESSAGE_ID(Messages::AnimKeyFrame_EndOfAnimation));
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
    
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Buffering EndOfAnimation KeyFrame\n");
#   endif
    _haveReceivedTerminationFrame = true;
    ++_numFramesBuffered;
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_AudioSample& msg)
  {
    Result lastResult = BufferKeyFrameHelper(msg, GET_MESSAGE_ID(Messages::AnimKeyFrame_AudioSample));
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
    
    //PRINT("Buffering AudioSample KeyFrame\n");
    ++_numFramesBuffered;
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_AudioSilence& msg)
  {
    Result lastResult = BufferKeyFrameHelper(msg, GET_MESSAGE_ID(Messages::AnimKeyFrame_AudioSilence));
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
    
    //PRINT("Buffering AudioSilence KeyFrame\n");
    ++_numFramesBuffered;
    return RESULT_OK;
  }

  Result BufferKeyFrame(const Messages::AnimKeyFrame_HeadAngle& msg) {
    Result lastResult = BufferKeyFrameHelper(msg, GET_MESSAGE_ID(Messages::AnimKeyFrame_HeadAngle));
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Buffering HeadAngle KeyFrame\n");
#   endif
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_LiftHeight& msg) {
    Result lastResult = BufferKeyFrameHelper(msg, GET_MESSAGE_ID(Messages::AnimKeyFrame_LiftHeight));
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Buffering LiftHeight KeyFrame\n");
#   endif
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_FaceImage& msg) {
    Result lastResult = BufferKeyFrameHelper(msg, GET_MESSAGE_ID(Messages::AnimKeyFrame_FaceImage));
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Buffering FaceImage KeyFrame\n");
#   endif
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_FacePosition& msg) {
    Result lastResult = BufferKeyFrameHelper(msg, GET_MESSAGE_ID(Messages::AnimKeyFrame_FacePosition));
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Buffering FacePosition KeyFrame\n");
#   endif
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_BackpackLights& msg) {
    Result lastResult = BufferKeyFrameHelper(msg, GET_MESSAGE_ID(Messages::AnimKeyFrame_BackpackLights));
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Buffering BackpackLights KeyFrame\n");
#   endif
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_BodyMotion& msg) {
    Result lastResult = BufferKeyFrameHelper(msg, GET_MESSAGE_ID(Messages::AnimKeyFrame_BodyMotion));
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Buffering BodyMotion KeyFrame\n");
#   endif
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_Blink& msg)
  {
    Result lastResult = BufferKeyFrameHelper(msg, GET_MESSAGE_ID(Messages::AnimKeyFrame_Blink));
    if(RESULT_OK != lastResult) {
      return lastResult;
    }
#   if DEBUG_ANIMATION_CONTROLLER
    PRINT("Buffering Blink KeyFrame\n");
#   endif
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
      ready = _numFramesBuffered > 1;
    } else {
      // Otherwise, wait until we get enough frames to start
      ready = (_numFramesBuffered > PREROLL_BUFFER_LENGTH || _haveReceivedTerminationFrame);
      if(ready) {
        _isPlaying = true;
        
#       if DEBUG_ANIMATION_CONTROLLER
        _currentTime_ms = 0;
#       endif
      }
    }
    
    //assert(_currentFrame <= _lastFrame);
    
    return ready;
  } // IsReadyToPlay()
  
  Result Update()
  {
    if(IsReadyToPlay()) {
      
      // If AudioReady() returns true, we are ready to move to the next keyframe
      if(HAL::AudioReady())
      {
        
        // Next thing in the buffer should be audio or silence:
        switch(GetTypeIndicator())
        {
          case Messages::AnimKeyFrame_AudioSilence_ID:
          {
            Messages::AnimKeyFrame_AudioSilence msg;
            GetFromBuffer((u8*)&msg, sizeof(msg));
            HAL::AudioPlayFrame(NULL);
            break;
          }
          case Messages::AnimKeyFrame_AudioSample_ID:
          {
            Messages::AnimKeyFrame_AudioSample msg;
            GetFromBuffer((u8*)&msg, sizeof(msg));
            if(_tracksToPlay & AUDIO_TRACK) {
              HAL::AudioPlayFrame(msg.sample);
            } else {
              HAL::AudioPlayFrame(NULL);
            }
            break;
          }
          default:
            PRINT("Expecting either audio sample or silence next in animation buffer.\n");
            return RESULT_FAIL;
        }
      
#       if DEBUG_ANIMATION_CONTROLLER
        _currentTime_ms += 33;
#       endif
        
        // Keep reading until we hit another audio type, then rewind one
        // (The rewind is a little icky, but I'm leaving it for now)
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
            PRINT("Ran out of animation buffer after getting audio/silence.\n");
            return RESULT_FAIL;
          }
          
          const Messages::ID msgID = GetTypeIndicator();
          switch(msgID)
          {
            case Messages::AnimKeyFrame_AudioSample_ID:
            case Messages::AnimKeyFrame_AudioSilence_ID:
              
              // Rewind so we re-see this type again on next update (a little icky, yes...)
              RewindBufferOneType();
              
              nextAudioFrameFound = true;
              break;
              
            case Messages::AnimKeyFrame_EndOfAnimation_ID:
            {
#             if DEBUG_ANIMATION_CONTROLLER
              PRINT("AnimationController[t=%dms(%d)] hit EndOfAnimation\n",
                    _currentTime_ms, HAL::GetTimeStamp());
#             endif
              Messages::AnimKeyFrame_EndOfAnimation msg;
              GetFromBuffer((u8*)&msg, sizeof(msg)); // just pull it out of the buffer
              terminatorFound = true;
              break;
            }
              
            case Messages::AnimKeyFrame_HeadAngle_ID:
            {
              Messages::AnimKeyFrame_HeadAngle msg;
              GetFromBuffer((u8*)&msg, sizeof(msg));
              
              if(_tracksToPlay & HEAD_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
                PRINT("AnimationController[t=%dms(%d)] requesting head angle of %ddeg over %.2fsec\n",
                      _currentTime_ms, HAL::GetTimeStamp(),
                      msg.angle_deg, static_cast<f32>(msg.time_ms)*.001f);
#               endif
                
                HeadController::SetDesiredAngle(DEG_TO_RAD(static_cast<f32>(msg.angle_deg)), 0.1f, 0.1f,
                                                static_cast<f32>(msg.time_ms)*.001f);
              }
              break;
            }
              
            case Messages::AnimKeyFrame_LiftHeight_ID:
            {
              Messages::AnimKeyFrame_LiftHeight msg;
              GetFromBuffer((u8*)&msg, sizeof(msg));
              
              if(_tracksToPlay & LIFT_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
                PRINT("AnimationController[t=%dms(%d)] requesting lift height of %dmm over %.2fsec\n",
                      _currentTime_ms, HAL::GetTimeStamp(),
                      msg.height_mm, static_cast<f32>(msg.time_ms)*.001f);
#               endif
                
                LiftController::SetDesiredHeight(static_cast<f32>(msg.height_mm), 0.1f, 0.1f,
                                                 static_cast<f32>(msg.time_ms)*.001f);
              }
              break;
            }
              
            case Messages::AnimKeyFrame_BackpackLights_ID:
            {
              Messages::AnimKeyFrame_BackpackLights msg;
              GetFromBuffer((u8*)&msg, sizeof(msg));
              
              if(_tracksToPlay & BACKPACK_LIGHTS_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
                PRINT("AnimationController[t=%dms(%d)] setting backpack LEDs.\n",
                      _currentTime_ms, HAL::GetTimeStamp());
#               endif
                
                for(s32 iLED=0; iLED<NUM_BACKPACK_LEDS; ++iLED) {
                  HAL::SetLED(static_cast<LEDId>(iLED), msg.colors[iLED]);
                }
              }
              break;
            }
              
            case Messages::AnimKeyFrame_FaceImage_ID:
            {
              Messages::AnimKeyFrame_FaceImage msg;
              GetFromBuffer((u8*)&msg, sizeof(msg));
              
              if(_tracksToPlay & FACE_IMAGE_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
                PRINT("AnimationController[t=%dms(%d)] setting face frame.\n",
                      _currentTime_ms, HAL::GetTimeStamp());
#               endif
                
                HAL::FaceAnimate(msg.image);
              }
              break;
            }
              
            case Messages::AnimKeyFrame_FacePosition_ID:
            {
              Messages::AnimKeyFrame_FacePosition msg;
              GetFromBuffer((u8*)&msg, sizeof(msg));
              
              if(_tracksToPlay & FACE_POS_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
                PRINT("AnimationController[t=%dms(%d)] setting face position to (%d,%d).\n",
                      _currentTime_ms, HAL::GetTimeStamp(), msg.xCen, msg.yCen);
#               endif
                
                HAL::FaceMove(msg.xCen, msg.yCen);
              }
              break;
            }
              
            case Messages::AnimKeyFrame_Blink_ID:
            {
              Messages::AnimKeyFrame_Blink msg;
              GetFromBuffer((u8*)&msg, sizeof(msg));
              
              if(_tracksToPlay & BLINK_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
                PRINT("AnimationController[t=%dms(%d)] Blinking.\n",
                      _currentTime_ms, HAL::GetTimeStamp());
#               endif
                
                if(msg.blinkNow) {
                  HAL::FaceBlink();
                } else {
                  if(msg.enable) {
                    EyeController::Enable();
                  } else {
                    EyeController::Disable();
                  }
                }
              }
              break;
            }
              
            case Messages::AnimKeyFrame_BodyMotion_ID:
            {
              Messages::AnimKeyFrame_BodyMotion msg;
              GetFromBuffer((u8*)&msg, sizeof(msg));
              
              if(_tracksToPlay & BODY_TRACK) {
#               if DEBUG_ANIMATION_CONTROLLER
                PRINT("AnimationController[t=%dms(%d)] setting body motion to radius=%d, speed=%d\n",
                      _currentTime_ms, HAL::GetTimeStamp(), msg.curvatureRadius_mm, msg.speed_mmPerSec);
#               endif
                
                f32 leftSpeed, rightSpeed;
                if(msg.speed == 0) {
                  // Stop
                  leftSpeed = 0.f;
                  rightSpeed = 0.f;
                } else if(msg.curvatureRadius_mm == s16_MAX || msg.curvatureRadius_mm == s16_MIN) {
                  // Drive straight
                  leftSpeed  = static_cast<f32>(msg.speed);
                  rightSpeed = static_cast<f32>(msg.speed);
                } else if(msg.curvatureRadius_mm == 0) {
                  // Turn in place: positive speed means turn left
                  // Interpret speed as degrees/sec
                  leftSpeed  = static_cast<f32>(-msg.speed);
                  rightSpeed = static_cast<f32>( msg.speed);
                  
                  const f32 speed_mmps = WHEEL_DIST_HALF_MM*tan(DEG_TO_RAD(static_cast<f32>(msg.speed)));
                  leftSpeed  = -speed_mmps;
                  rightSpeed =  speed_mmps;
                  
                } else {
                  // Drive an arc
                  
                  //if speed is positive, the left wheel should turn slower, so
                  // it becomes the INNER wheel
                  leftSpeed = static_cast<f32>(msg.speed) * (1.0f - WHEEL_DIST_HALF_MM / static_cast<f32>(msg.curvatureRadius_mm));
                  
                  //if speed is positive, the right wheel should turn faster, so
                  // it becomes the OUTER wheel
                  rightSpeed = static_cast<f32>(msg.speed) * (1.0f + WHEEL_DIST_HALF_MM / static_cast<f32>(msg.curvatureRadius_mm));
                }
                
                SteeringController::ExecuteDirectDrive(leftSpeed, rightSpeed);
              }
              break;
            }
              
            default:
            {
              PRINT("Unexpected message type %d in animation buffer!\n");
              return RESULT_FAIL;
            }
              
          } // switch(GetTypeIndicator())
        } // while(!nextAudioFrameFound && !terminatorFound)

        --_numFramesBuffered;
        
        if(terminatorFound) {
          _isPlaying = false;
          _haveReceivedTerminationFrame = false;
          --_numFramesBuffered;
#         if DEBUG_ANIMATION_CONTROLLER
          PRINT("Reached animation termination frame (%d frames still buffered, curPos/lastPos = %d/%d).\n",
                _numFramesBuffered, _currentBufferPos, _lastBufferPos);
#         endif
        }
        
      } // if(AudioReady())
    } // if(IsReadyToPlay())
    
    return RESULT_OK;
  } // Update()

  void SetTracksToPlay(AnimTrackFlag tracksToPlay)
  {
    _tracksToPlay = tracksToPlay;
  }
  
} // namespace AnimationController
} // namespace Cozmo
} // namespace Anki
