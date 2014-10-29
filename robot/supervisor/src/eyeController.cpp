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
    
    // Spin in canonical clockwise order:
    const EyeSegments SpinAnimation[NUM_EYE_SEGMENTS] = {EYE_SEGMENT_TOP, EYE_SEGMENT_LEFT, EYE_SEGMENT_BOTTOM, EYE_SEGMENT_RIGHT};
    
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
        
        struct {
          bool   clockwise;
          u16    period_ms;
          u8     fadeIndex1;
          u8     fadeIndex2;
          u8     offIndex;
        } spinSettings;
      };
    };
    
    Eye _leftEye, _rightEye;
    
  }; // "private" variables

  
  // Forward declaration for helper functions
  static void SetEyeShapeHelper(Eye& eye, EyeShape shape);
  static void BlinkHelper(Eye& eye, TimeStamp_t currentTime);
  static void StartBlinkingHelper(Eye& eye, TimeStamp_t currentTime,
                                  u16 onPeriod_ms, u16 offPeriod_ms);
  static void StartFlashingHelper(Eye& eye, TimeStamp_t currentTime,
                                  EyeShape shape, u16 onPeriod_ms, u16 offPeriod_ms);
  static void FlashHelper(Eye& eye, TimeStamp_t currentTime);
  static void SpinHelper(Eye& eye, TimeStamp_t currentTime);
  static void StartSpinningHelper(Eye& eye, TimeStamp_t currentTime,
                                  u16 period_ms, bool clockwise);
  
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
    const TimeStamp_t currentTime = HAL::GetTimeStamp();
    
    StartFlashingHelper(_leftEye, currentTime, leftShape, leftOnPeriod_ms, leftOffPeriod_ms);
    StartFlashingHelper(_rightEye, currentTime, rightShape, rightOnPeriod_ms, rightOffPeriod_ms);
    
    _eyeAnimMode = FLASH;
  }

  void StartSpinning(u16 period_ms, bool leftClockWise, bool rightClockWise)
  {
    const TimeStamp_t currentTime = HAL::GetTimeStamp();
    
    StartSpinningHelper(_leftEye, currentTime, period_ms, leftClockWise);
    StartSpinningHelper(_rightEye, currentTime, period_ms, rightClockWise);
    
    _eyeAnimMode = SPIN;
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
      case SPIN:
      {
        SpinHelper(_leftEye, currentTime);
        SpinHelper(_rightEye, currentTime);
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
   
    if(onPeriod_ms != eye.blinkTimings[0] ||
       offPeriod_ms/3 != eye.blinkTimings[3])
    {
      eye.nextSwitchTime = currentTime;
      eye.blinkTimings[0] = onPeriod_ms;
      eye.blinkTimings[1] = offPeriod_ms/3;
      eye.blinkTimings[2] = eye.blinkTimings[1];
      eye.blinkTimings[3] = eye.blinkTimings[1];
      eye.animIndex = 2;
    }
    
  } // StartBlinkingHelper()
  
  
  void StartFlashingHelper(Eye& eye, TimeStamp_t currentTime,
                           EyeShape shape, u16 onPeriod_ms, u16 offPeriod_ms)
  {
    
    if(shape != eye.flashSettings.shape ||
       onPeriod_ms != eye.flashSettings.timings[1] ||
       offPeriod_ms != eye.flashSettings.timings[0])
    {
      eye.flashSettings.shape = shape;
      eye.flashSettings.timings[0] = offPeriod_ms;
      eye.flashSettings.timings[1] = onPeriod_ms;
      eye.nextSwitchTime = currentTime;
    }
  }
  
  void StartSpinningHelper(Eye& eye, TimeStamp_t currentTime,
                           u16 period_ms, bool clockwise)
  {
    if(period_ms != eye.spinSettings.period_ms ||
       clockwise != eye.spinSettings.clockwise)
    {
      eye.spinSettings.clockwise = clockwise;
      eye.spinSettings.period_ms = period_ms;
      eye.animIndex = 0;
      eye.spinSettings.fadeIndex1 = (clockwise ? NUM_EYE_SEGMENTS-1 : 1);
      eye.spinSettings.fadeIndex2 = (clockwise ? NUM_EYE_SEGMENTS-2 : 2);
      eye.spinSettings.offIndex   = (clockwise ? NUM_EYE_SEGMENTS-3 : 3);
      eye.nextSwitchTime = currentTime;
    }
  }
  
  
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
  
  void SpinHelper(Eye& eye, TimeStamp_t currentTime)
  {
    if(currentTime > eye.nextSwitchTime) {
      if(eye.spinSettings.clockwise) {
        if(++eye.animIndex == NUM_EYE_SEGMENTS) {
          eye.animIndex = 0;
        }
        if(++eye.spinSettings.fadeIndex1 == NUM_EYE_SEGMENTS) {
          eye.spinSettings.fadeIndex1 = 0;
        }
        if(++eye.spinSettings.fadeIndex2 == NUM_EYE_SEGMENTS) {
          eye.spinSettings.fadeIndex2 = 0;
        }
        if(++eye.spinSettings.offIndex == NUM_EYE_SEGMENTS) {
          eye.spinSettings.offIndex = 0;
        }
      } else {
        if(eye.animIndex-- == 0) {
          eye.animIndex = NUM_EYE_SEGMENTS-1;
        }
        if(eye.spinSettings.fadeIndex1-- == 0) {
          eye.spinSettings.fadeIndex1 = NUM_EYE_SEGMENTS-1;
        }
        if(eye.spinSettings.fadeIndex2-- == 0) {
          eye.spinSettings.fadeIndex2 = NUM_EYE_SEGMENTS-1;
        }
        if(eye.spinSettings.offIndex-- == 0) {
          eye.spinSettings.offIndex = NUM_EYE_SEGMENTS-1;
        }
      }
      
      u8 rgb[3] = {
        static_cast<u8>((eye.color & 0xff0000) >> 16),
        static_cast<u8>((eye.color & 0x00ff00) >> 8),
        static_cast<u8>(eye.color & 0x0000ff)
      };
      
      HAL::SetLED(eye.segments[SpinAnimation[eye.animIndex]], eye.color);
      
      HAL::SetLED(eye.segments[SpinAnimation[eye.spinSettings.fadeIndex1]],
                  static_cast<u32>((rgb[0]/2) << 16) +
                  static_cast<u32>((rgb[1]/2) << 8 ) +
                  static_cast<u32>((rgb[2]/2)));
      
      HAL::SetLED(eye.segments[SpinAnimation[eye.spinSettings.fadeIndex2]],
                  static_cast<u32>((rgb[0]/4) << 16) +
                  static_cast<u32>((rgb[1]/4) << 8 ) +
                  static_cast<u32>((rgb[2]/4)));
      
      HAL::SetLED(eye.segments[SpinAnimation[eye.spinSettings.offIndex]], LED_OFF);
     
      eye.nextSwitchTime = currentTime + eye.spinSettings.period_ms/NUM_EYE_SEGMENTS;
    }
  }
  
} // namespace EyeController
} // namespace Cozmo
} // namespace Anki
