#include "animationController.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities_c.h"
#include "anki/common/shared/radians.h"
#include "anki/common/shared/velocityProfileGenerator.h"

#include "localization.h"
#include "headController.h"
#include "liftController.h"
#include "wheelController.h"
#include "steeringController.h"
#include "streamedKeyFrame.h"
#include "speedController.h"


#define DEBUG_ANIMATION_CONTROLLER 0

#define USE_HARDCODED_ANIMATIONS 0

namespace Anki {
namespace Cozmo {
namespace AnimationController {
  
  namespace {
    
    // Circular buffer of keyframes
    StreamedKeyFrame _keyFrameBuffer[KEYFRAME_BUFFER_LENGTH];
    s32  _currentFrame;
    s32  _lastFrame;
    s32  _numFramesBuffered;
    bool _haveReceivedTerminationFrame;
    bool _isPlaying;
    
  } // "private" members

  
  // Forward decl.
//  void ReallyStop();
//  void StopCurrent();
  
  
  // ========== End of Animation Start/Update/Stop functions ===========
  
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
    Clear();
    
    DefineHardCodedAnimations();
    
    return RESULT_OK;
  }
  
  
  void Clear()
  {
    _currentFrame = 0;
    _lastFrame    = 0;
    _numFramesBuffered = 0;
    _haveReceivedTerminationFrame = false;
    _isPlaying = false;
    
    // Necessary?
    //memset(_keyFrameBuffer, KEYFRAME_BUFFER_LENGTH, sizeof(StreamedKeyFrame));
  }
 
  
  static Result AdvanceLastFrame()
  {
    // Move to next frame in the buffer, and deal with the fact that this is a
    // circular buffer
    ++_lastFrame;
    if(_lastFrame == KEYFRAME_BUFFER_LENGTH) {
      _lastFrame = 0;
    }
    
    if(_lastFrame == _currentFrame) {
      // Ran out of buffer!
      AnkiError("AnimationController.BufferKeyFrame.BufferFull",
                "KeyFrame buffer full, can't add given KeyFrame!\n");
      
      // Put lastFrame back to what it was
      if(_lastFrame == 0) {
        _lastFrame = KEYFRAME_BUFFER_LENGTH-1;
      } else {
        --_lastFrame;
      }
      
      return RESULT_FAIL_OUT_OF_MEMORY;
    }

    // "Reset" the frame:
    _keyFrameBuffer[_lastFrame].setsWhichTracks = 0;
    
    ++_numFramesBuffered;
    
    return RESULT_OK;
    
  } // AdvanceLastFrame()
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_AudioSample& msg)
  {
    // New audio frame: advance to the next frame in the buffer and then
    // populate it
    Result lastResult = AdvanceLastFrame();
    AnkiConditionalErrorAndReturnValue(lastResult== RESULT_OK, lastResult,
                                       "BufferKeyFrame", "Failed to advance last frame counter.\n");
    
    StreamedKeyFrame& lastKeyFrame = _keyFrameBuffer[_lastFrame];
    
    lastKeyFrame.setsWhichTracks |= StreamedKeyFrame::KF_SETS_AUDIO;
    
    assert(sizeof(msg.sample)==sizeof(StreamedKeyFrame::audioSample));
    
    memcpy(lastKeyFrame.audioSample, msg.sample, sizeof(StreamedKeyFrame::audioSample));

    return lastResult;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_AudioSilence& msg)
  {
    // Silent audio frame just advances us to the next frame in the buffer
    return AdvanceLastFrame();
  }
  
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_HeadAngle& msg)
  {
    // Just set the last frame's head properties:
    StreamedKeyFrame& lastKeyFrame = _keyFrameBuffer[_lastFrame];
    lastKeyFrame.setsWhichTracks |= StreamedKeyFrame::KF_SETS_HEAD;
    lastKeyFrame.headAngle_deg = msg.angle_deg;
    lastKeyFrame.headTime_ms   = msg.time_ms;
    
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_LiftHeight&   msg)
  {
    StreamedKeyFrame& lastKeyFrame = _keyFrameBuffer[_lastFrame];
    lastKeyFrame.setsWhichTracks |= StreamedKeyFrame::KF_SETS_LIFT;
    lastKeyFrame.liftHeight_mm = msg.height_mm;
    lastKeyFrame.liftTime_ms   = msg.time_ms;
    
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_FaceImage& msg)
  {
    StreamedKeyFrame& lastKeyFrame = _keyFrameBuffer[_lastFrame];
    lastKeyFrame.setsWhichTracks |= StreamedKeyFrame::KF_SETS_FACE_FRAME;
    
    // TODO: Update this to copy out a variable-length amount of data
    memcpy(lastKeyFrame.faceFrame, msg.image, sizeof(msg.image));
    
    return RESULT_OK;
  }
  
  Result BufferKeyFrame(const Messages::AnimKeyFrame_FacePosition& msg)
  {
    StreamedKeyFrame& lastKeyFrame = _keyFrameBuffer[_lastFrame];
    lastKeyFrame.setsWhichTracks |= StreamedKeyFrame::KF_SETS_FACE_POSITION;
    lastKeyFrame.faceCenX = msg.xCen;
    lastKeyFrame.faceCenY = msg.yCen;
    
    return RESULT_OK;
  }
  
  bool IsBufferFull()
  {
    return _numFramesBuffered > (KEYFRAME_BUFFER_LENGTH - KEYFRAME_BUFFER_PADDING);
  }
  
  s32 GetNumFramesFree()
  {
    const s32 framesFree = MAX(0, KEYFRAME_BUFFER_LENGTH-KEYFRAME_BUFFER_PADDING-_numFramesBuffered);

    return framesFree;
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
      ready = _numFramesBuffered > 0;
    } else {
      // Otherwise, wait until we get enough frames to start
      ready = (_numFramesBuffered > PREROLL_BUFFER_LENGTH ||
               (_numFramesBuffered > 0 && _haveReceivedTerminationFrame));
    }
    
    assert(_currentFrame <= _lastFrame);
    
    return ready;
  } // IsReadyToPlay()
  
  void Update()
  {
    if(IsReadyToPlay()) {
      
      // If AudioReady() returns true, we are ready to move to the next keyframe
      if(HAL::AudioReady()) {
        
        StreamedKeyFrame& keyFrame = _keyFrameBuffer[_currentFrame];
        
        // Go through each track and see if this keyframe specifies anything
        // for it. If so, take appropriate action.
        if( !(keyFrame.setsWhichTracks & StreamedKeyFrame::KF_IS_TERMINATION) ) {
          _isPlaying = true;
          
          if(keyFrame.setsWhichTracks & StreamedKeyFrame::KF_SETS_AUDIO) {
            HAL::AudioPlayFrame(keyFrame.audioSample);
          } else {
            
            // Play "silence"
            HAL::AudioPlayFrame(NULL);
            
          } // if(setsAudio)
          
          if(keyFrame.setsWhichTracks & StreamedKeyFrame::KF_SETS_BACKPACK_LEDS) {
            
          } // if(setsBackPackLEDs)
          
          if(keyFrame.setsWhichTracks & StreamedKeyFrame::KF_SETS_HEAD) {
            
            HeadController::SetDesiredAngle(DEG_TO_RAD(static_cast<f32>(keyFrame.headAngle_deg)), 0.5f, 0.5f,
                                            static_cast<f32>(keyFrame.headTime_ms)*.001f);
            
          } // if(setsHead)
          
          if(keyFrame.setsWhichTracks & StreamedKeyFrame::KF_SETS_LIFT) {
            
            LiftController::SetDesiredHeight(static_cast<f32>(keyFrame.liftHeight_mm), 0.5f, 0.5f,
                                             static_cast<f32>(keyFrame.liftTime_ms)*.001f);
          } // if(setsLift)
          
          
          if(keyFrame.setsWhichTracks & StreamedKeyFrame::KF_SETS_WHEELS) {
            
            WheelController::SetDesiredWheelSpeeds(keyFrame.wheelSpeedL, keyFrame.wheelSpeedR);
          }
          
        } else {
          // Termination frame reached!
          _isPlaying = false;
          _haveReceivedTerminationFrame = false;
          
#         if DEBUG_ANIMATION_CONTROLLER
          PRINT("Reached animation termination frame.\n");
#         endif
        } // if(sets *any* tracks)
        
        // Move to next keyframe in the (circular) buffer
        ++_currentFrame;
        if(_currentFrame == KEYFRAME_BUFFER_LENGTH) {
          _currentFrame = 0;
        }
        
        --_numFramesBuffered;
        
      } // if(AudioReady())
    } // if(IsReadyToPlay())
    
  } // Update()

  
} // namespace AnimationController
} // namespace Cozmo
} // namespace Anki
