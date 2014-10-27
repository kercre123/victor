/**
 * File: eyeController.cpp
 *
 * Author: Andrew Stein
 * Created: 10/16/2014
 *
 * Description:
 *
 *   Controller for the color, shape, and low-level animation of the eyes.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "eyeController.h"

#include "anki/cozmo/robot/hal.h"

#include "anki/common/robot/errorHandling.h"


namespace Anki {
namespace Cozmo {
namespace EyeController {
  namespace {
    
    enum EyeAnimMode {
      NONE,
      BLINK,
      FLASH,
      SPIN
    };
    
    EyeAnimMode _eyeAnimMode;
    
    enum EyeSegments {
      EYE_SEGMENT_TOP,
      EYE_SEGMENT_LEFT,
      EYE_SEGMENT_RIGHT,
      EYE_SEGMENT_BOTTOM,
      NUM_EYE_SEGMENTS
    };

    const s32 BLINK_ANIM_LENGTH = 4;
    const EyeShape BlinkAnimation[BLINK_ANIM_LENGTH] = {EYE_OPEN, EYE_HALF, EYE_CLOSED, EYE_HALF};
    
    struct Eye {
      u32           color;
      u8            animIndex;
      TimeStamp_t   nextSwitchTime;
      LEDId         segments[NUM_EYE_SEGMENTS];
      
      union {
        TimeStamp_t   blinkTimings[BLINK_ANIM_LENGTH];  // set by on/off periods in StartBlinking
        
        struct {
          TimeStamp_t   timings[2]; // off and on
          EyeShape      shape;
        } flashSettings;
      };
    };
    
    Eye _leftEye, _rightEye;
    
  }; // "private" variables

  
  // Forward declaration for helper functions
  static void SetEyeShapeHelper(Eye& eye, EyeShape shape);
  static void BlinkHelper(Eye& eye, TimeStamp_t currentTime);
  static void StartBlinkingHelper(Eye& eye, TimeStamp_t currentTime,
                                  u16 onPeriod_ms, u16 offPeriod_ms);
  static void FlashHelper(Eye& eye, TimeStamp_t currentTime);
  
  
  Result Init()
  {
    _eyeAnimMode = NONE;
    
    _leftEye.segments[EYE_SEGMENT_TOP]    = LED_LEFT_EYE_TOP;
    _leftEye.segments[EYE_SEGMENT_LEFT]   = LED_LEFT_EYE_LEFT;
    _leftEye.segments[EYE_SEGMENT_RIGHT]  = LED_LEFT_EYE_RIGHT;
    _leftEye.segments[EYE_SEGMENT_BOTTOM] = LED_LEFT_EYE_BOTTOM;
        
    _rightEye.segments[EYE_SEGMENT_TOP]    = LED_RIGHT_EYE_TOP;
    _rightEye.segments[EYE_SEGMENT_LEFT]   = LED_RIGHT_EYE_LEFT;
    _rightEye.segments[EYE_SEGMENT_RIGHT]  = LED_RIGHT_EYE_RIGHT;
    _rightEye.segments[EYE_SEGMENT_BOTTOM] = LED_RIGHT_EYE_BOTTOM;
    
    SetEyeColor(LED_OFF);
    
    return RESULT_OK;
  } // Init()
  
  
  void SetEyeColor(u32 color) {
    SetEyeColor(color, color);
  }
  
  void SetEyeColor(u32 leftColor, u32 rightColor)
  {
    
    if(leftColor != LED_CURRENT_COLOR) {
      _leftEye.color  = leftColor;
    }
    
    if(rightColor != LED_CURRENT_COLOR) {
      _rightEye.color = rightColor;
    }
    
  } // SetEyeColor()
  
  void SetEyeShape(EyeShape shape) {
    SetEyeShape(shape, shape);
  }
  
  void SetEyeShape(EyeShape leftShape, EyeShape rightShape)
  {
    SetEyeShapeHelper(_leftEye, leftShape);
    SetEyeShapeHelper(_rightEye, rightShape);
    
    StopAnimating();
    
  } // SetEyeShape()

  
  void StartBlinking(u16 onPeriod_ms, u16 offPeriod_ms)
  {
    StartBlinking(onPeriod_ms, offPeriod_ms,
                  onPeriod_ms, offPeriod_ms);
  }
  
  void StartBlinking(u16 leftOnPeriod_ms,  u16 leftOffPeriod_ms,
                     u16 rightOnPeriod_ms, u16 rightOffPeriod_ms)
  {
    const TimeStamp_t currentTime = HAL::GetTimeStamp();
    
    StartBlinkingHelper(_leftEye, currentTime, leftOnPeriod_ms, leftOffPeriod_ms);
    
    StartBlinkingHelper(_rightEye, currentTime, rightOnPeriod_ms, rightOffPeriod_ms);
    
    _eyeAnimMode = BLINK;
  }
  
  void StartFlashing(EyeShape shape, u16 onPeriod_ms, u16 offPeriod_ms)
  {
    StartFlashing(shape, onPeriod_ms, offPeriod_ms,
                  shape, onPeriod_ms, offPeriod_ms);
  }
  
  
  void StartFlashing(EyeShape leftShape, u16 leftOnPeriod_ms, u16 leftOffPeriod_ms,
                     EyeShape rightShape, u16 rightOnPeriod_ms, u16 rightOffPeriod_ms)
  {
    
    _leftEye.flashSettings.shape = leftShape;
    _leftEye.flashSettings.timings[0] = leftOffPeriod_ms;
    _leftEye.flashSettings.timings[1] = leftOnPeriod_ms;
    
    _rightEye.flashSettings.shape = rightShape;
    _rightEye.flashSettings.timings[0] = rightOffPeriod_ms;
    _rightEye.flashSettings.timings[1] = rightOnPeriod_ms;
    
    _eyeAnimMode = FLASH;
  }

  
  void StopAnimating()
  {
    _eyeAnimMode = NONE;
  }
  
  
  Result Update()
  {
    TimeStamp_t currentTime = HAL::GetTimeStamp();
    
    switch(_eyeAnimMode)
    {
      case NONE:
      {
        // Just leave eyes as they are
        break;
      }
      case BLINK:
      {
        BlinkHelper(_leftEye,  currentTime);
        BlinkHelper(_rightEye, currentTime);
        break;
      }
      case FLASH:
      {
        FlashHelper(_leftEye, currentTime);
        FlashHelper(_rightEye, currentTime);
        break;
      }
        
      // TODO: Implement other modes
        
    } // switch(_eyeAnimMode)

    return RESULT_OK;
    
  } // Update()
  
  
#if 0
#pragma mark ---- Static Helpers ----
#endif
  
  void StartBlinkingHelper(Eye& eye, TimeStamp_t currentTime,
                                  u16 onPeriod_ms, u16 offPeriod_ms)
  {
    if(eye.color == LED_OFF) {
      AnkiWarn("EyeController.StartBlinkingHelper.NoEyeColor",
               "Eye is currently off: blinking will not have any effect.\n");
    }
    
    eye.nextSwitchTime = currentTime;
    eye.blinkTimings[0] = onPeriod_ms;
    eye.blinkTimings[1] = offPeriod_ms/3;
    eye.blinkTimings[2] = eye.blinkTimings[1];
    eye.blinkTimings[3] = eye.blinkTimings[1];
    eye.animIndex = 2;
    
  } // StartBlinkingHelper()
  
  
  void SetEyeShapeHelper(Eye& eye, EyeShape shape)
  {
    switch(shape)
    {
      case EYE_OPEN:
      {
        // Turn all eye segments on
        for(s32 iSegment=0; iSegment < NUM_EYE_SEGMENTS; ++iSegment) {
          HAL::SetLED(eye.segments[iSegment], eye.color);
        }
        break;
      }
        
      case EYE_CLOSED:
      {
        // Turn all eye segments off
        for(s32 iSegment=0; iSegment < NUM_EYE_SEGMENTS; ++iSegment) {
          HAL::SetLED(eye.segments[iSegment], LED_OFF);
        }
        break;
      }
        
      case EYE_HALF:
      {
        // Top/bottom off, left/right on
        HAL::SetLED(eye.segments[EYE_SEGMENT_TOP],    LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_BOTTOM], LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_LEFT],   eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_RIGHT],  eye.color);
        break;
      }
        
      case EYE_SLIT:
      {
        // Left/right off, top/bottom on
        HAL::SetLED(eye.segments[EYE_SEGMENT_TOP],    LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_BOTTOM], LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_LEFT],   eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_RIGHT],  eye.color);
        break;
      }
        
      case EYE_OFF_PUPIL_UP:
      {
        HAL::SetLED(eye.segments[EYE_SEGMENT_TOP],    LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_BOTTOM], eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_LEFT],   eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_RIGHT],  eye.color);
        break;
      }
        
      case EYE_ON_PUPIL_UP:
      {
        HAL::SetLED(eye.segments[EYE_SEGMENT_TOP],    eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_BOTTOM], LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_LEFT],   LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_RIGHT],  LED_OFF);
        break;
      }
        
      case EYE_OFF_PUPIL_DOWN:
      {
        HAL::SetLED(eye.segments[EYE_SEGMENT_TOP],    eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_BOTTOM], LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_LEFT],   eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_RIGHT],  eye.color);
        break;
      }
        
      case EYE_ON_PUPIL_DOWN:
      {
        HAL::SetLED(eye.segments[EYE_SEGMENT_TOP],    LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_BOTTOM], eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_LEFT],   LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_RIGHT],  LED_OFF);
        break;
      }
        
      case EYE_OFF_PUPIL_LEFT:
      {
        HAL::SetLED(eye.segments[EYE_SEGMENT_TOP],    eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_BOTTOM], eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_LEFT],   LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_RIGHT],  eye.color);
        break;
      }
        
      case EYE_ON_PUPIL_LEFT:
      {
        HAL::SetLED(eye.segments[EYE_SEGMENT_TOP],    LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_BOTTOM], LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_LEFT],   eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_RIGHT],  LED_OFF);
        break;
      }
        
      case EYE_OFF_PUPIL_RIGHT:
      {
        HAL::SetLED(eye.segments[EYE_SEGMENT_TOP],    eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_BOTTOM], eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_LEFT],   eye.color);
        HAL::SetLED(eye.segments[EYE_SEGMENT_RIGHT],  LED_OFF);
        break;
      }
        
      case EYE_ON_PUPIL_RIGHT:
      {
        HAL::SetLED(eye.segments[EYE_SEGMENT_TOP],    LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_BOTTOM], LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_LEFT],   LED_OFF);
        HAL::SetLED(eye.segments[EYE_SEGMENT_RIGHT],  eye.color);
        break;
      }
        
      default:
        AnkiError("LightController.SetEyeShape.InvalidShape", "Unknown shape provided to SetEyeShape.\n");
    }
    
  } // SetEyeShape()
  
  
  void BlinkHelper(Eye& eye, TimeStamp_t currentTime)
  {
    if(currentTime > eye.nextSwitchTime) {
      ++eye.animIndex;
      if(eye.animIndex == BLINK_ANIM_LENGTH) {
        eye.animIndex = 0;
      }
      
      SetEyeShapeHelper(eye, BlinkAnimation[eye.animIndex]);
      eye.nextSwitchTime = currentTime + eye.blinkTimings[eye.animIndex];
    }
  } // BlinkHelper()
  
  
  void FlashHelper(Eye& eye, TimeStamp_t currentTime)
  {
    if(currentTime > eye.nextSwitchTime) {
      ++eye.animIndex;
      if(eye.animIndex == 2) {
        eye.animIndex = 0;
      }
      
      if(eye.animIndex == 0) {
        // Off state
        SetEyeShapeHelper(eye, EYE_CLOSED);
      } else if(eye.animIndex == 1) {
        // On state
        SetEyeShapeHelper(eye, eye.flashSettings.shape);
      }
      
      eye.nextSwitchTime = currentTime + eye.flashSettings.timings[eye.animIndex];
    }
  }
  
} // namespace EyeController
} // namespace Cozmo
} // namespace Anki