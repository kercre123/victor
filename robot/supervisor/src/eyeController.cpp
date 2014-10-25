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
      LEDColor      color;
      u8            animIndex;
      TimeStamp_t   nextSwitchTime;
      LEDId         segments[NUM_EYE_SEGMENTS];
      
      TimeStamp_t   blinkTimings[BLINK_ANIM_LENGTH]; //   = {    1650,      100,        100,      100};
    };
    
    Eye _leftEye, _rightEye;
    
  }; // "private" variables

  
  // Forward declaration for helper functions
  static void SetEyeShapeHelper(Eye& eye, EyeShape shape);
  static void BlinkHelper(Eye& eye, TimeStamp_t currentTime);

  
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
    
    return RESULT_OK;
  } // Init()
  
  
  void SetEyeColor(LEDColor color) {
    SetEyeColor(color, color);
  }
  
  void SetEyeColor(LEDColor leftColor, LEDColor rightColor)
  {
    
    _leftEye.color  = leftColor;
    _rightEye.color = rightColor;
    
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
    
    _leftEye.nextSwitchTime = currentTime;
    _leftEye.blinkTimings[0] = leftOnPeriod_ms;
    _leftEye.blinkTimings[1] = leftOffPeriod_ms/3;
    _leftEye.blinkTimings[2] = _leftEye.blinkTimings[1];
    _leftEye.blinkTimings[3] = _leftEye.blinkTimings[1];
    _leftEye.animIndex = 0;
    
    _rightEye.nextSwitchTime = currentTime;
    _rightEye.blinkTimings[0] = rightOnPeriod_ms;
    _rightEye.blinkTimings[1] = rightOffPeriod_ms/3;
    _rightEye.blinkTimings[2] = _rightEye.blinkTimings[1];
    _rightEye.blinkTimings[3] = _rightEye.blinkTimings[1];
    _rightEye.animIndex = 0;
    
    _eyeAnimMode = BLINK;
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
    } // switch(_eyeAnimMode)

    return RESULT_OK;
    
  } // Update()
  
  
  
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
  
  
} // namespace EyeController
} // namespace Cozmo
} // namespace Anki