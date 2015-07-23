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
#include "anki/common/robot/utilities.h"


namespace Anki {
namespace Cozmo {
namespace EyeController {
  namespace {
    
    const s32 NUM_BLINKS = 15;
    const s32 _blinkSpacings_ms[NUM_BLINKS] = {
      5000, 6000, 2000, 5000, 4000, 3000, 4000, 4000, 8000, 7000, 3000, 7000,
      5000, 4000, 5000
    };
    
    s32 _blinkIndex;
    TimeStamp_t _blinkStart_ms;
    
    enum EyeAnimMode {
      NONE,
      BLINK_TO_MIDDLE,
      BLINK_TO_BOTTOM,
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

    const s32 BLINK_TO_MIDDLE_ANIM_LENGTH = 4;
    const EyeShape BlinkAnimation[BLINK_TO_MIDDLE_ANIM_LENGTH] = {
      EYE_OPEN, EYE_HALF, EYE_CLOSED, EYE_HALF};
    
    const s32 BLINK_TO_BOTTOM_ANIM_LENGTH = 6;
    const EyeShape BlinkAnimation_ToBtm[BLINK_TO_BOTTOM_ANIM_LENGTH] = {
      EYE_OPEN, EYE_OFF_PUPIL_UP, EYE_ON_PUPIL_DOWN, EYE_CLOSED, EYE_ON_PUPIL_DOWN, EYE_OFF_PUPIL_UP
    };
    
    // Spin in canonical clockwise order:
    const EyeSegments SpinAnimation[NUM_EYE_SEGMENTS] = {EYE_SEGMENT_TOP, EYE_SEGMENT_LEFT, EYE_SEGMENT_BOTTOM, EYE_SEGMENT_RIGHT};
    
    struct Eye {
      u32           color;
      u8            animIndex;
      TimeStamp_t   nextSwitchTime;
      LEDId         segments[NUM_EYE_SEGMENTS];
      
      union {
        
        // set by on/off periods in StartBlinking
        TimeStamp_t   blinkToMiddleTimings[BLINK_TO_MIDDLE_ANIM_LENGTH];
        TimeStamp_t   blinkToBottomTimings[BLINK_TO_BOTTOM_ANIM_LENGTH];
        
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
    
    s32 _blinkVariation_ms;
    
    bool _enable = false;
    
  }; // "private" variables

  
  // Forward declaration for helper functions
  static void SetEyeShapeHelper(Eye& eye, EyeShape shape);
  static void BlinkHelper(Eye& eye, TimeStamp_t currentTime, bool toMiddle);
  static void StartBlinkingHelper(Eye& eye, TimeStamp_t currentTime,
                                  u16 onPeriod_ms, u16 offPeriod_ms,
                                  bool toMiddle);
  static void StartFlashingHelper(Eye& eye, TimeStamp_t currentTime,
                                  EyeShape shape, u16 onPeriod_ms, u16 offPeriod_ms);
  static void FlashHelper(Eye& eye, TimeStamp_t currentTime);
  static void SpinHelper(Eye& eye, TimeStamp_t currentTime);
  static void StartSpinningHelper(Eye& eye, TimeStamp_t currentTime,
                                  u16 period_ms, bool clockwise);
  
  Result Init()
  {
    /*
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
    
    _blinkVariation_ms = 200;
    */
    
    _blinkIndex = 0;
    _blinkStart_ms = HAL::GetTimeStamp();
    
    _enable = true;
    
    return RESULT_OK;
    
  } // Init()

  
  void Enable()
  {
    _enable = true;
  }
  
  void Disable()
  {
    _enable = false;
  }
  
  void SetEyeColor(u32 color) {
    
    SetEyeColor(color, color);
  }
  
  void SetEyeColor(u32 leftColor, u32 rightColor)
  {
    AnkiConditionalWarnAndReturn(_enable, "EyeController.SetEyeColor.Disabled",
                                 "SetEyeColor() command ignored: EyeController is disabled.\n");
    
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
    AnkiConditionalWarnAndReturn(_enable, "EyeController.SetEyeShape.Disabled",
                                 "SetEyeShape() command ignored: EyeController is disabled.\n");
    
    SetEyeShapeHelper(_leftEye, leftShape);
    SetEyeShapeHelper(_rightEye, rightShape);
    
    StopAnimating();
    
  } // SetEyeShape()

  
  void StartBlinking(u16 onPeriod_ms, u16 offPeriod_ms, bool toMiddle)
  {
    StartBlinking(onPeriod_ms, offPeriod_ms,
                  onPeriod_ms, offPeriod_ms,
                  toMiddle);
  }
  
  void StartBlinking(u16 leftOnPeriod_ms,  u16 leftOffPeriod_ms,
                     u16 rightOnPeriod_ms, u16 rightOffPeriod_ms,
                     bool toMiddle)
  {
    AnkiConditionalWarnAndReturn(_enable, "EyeController.StartBlinking.Disabled",
                                 "StartBlinking() command ignored: EyeController is disabled.\n");
    
    const TimeStamp_t currentTime = HAL::GetTimeStamp();
    
    StartBlinkingHelper(_leftEye, currentTime, leftOnPeriod_ms, leftOffPeriod_ms, toMiddle);
    
    StartBlinkingHelper(_rightEye, currentTime, rightOnPeriod_ms, rightOffPeriod_ms, toMiddle);
    
    _eyeAnimMode = (toMiddle ? BLINK_TO_MIDDLE : BLINK_TO_BOTTOM);
  }
  
  void StartFlashing(EyeShape shape, u16 onPeriod_ms, u16 offPeriod_ms)
  {
    StartFlashing(shape, onPeriod_ms, offPeriod_ms,
                  shape, onPeriod_ms, offPeriod_ms);
  }
  
  
  void StartFlashing(EyeShape leftShape, u16 leftOnPeriod_ms, u16 leftOffPeriod_ms,
                     EyeShape rightShape, u16 rightOnPeriod_ms, u16 rightOffPeriod_ms)
  {
    AnkiConditionalWarnAndReturn(_enable, "EyeController.StartFlashing.Disabled",
                                 "StartFlashing() command ignored: EyeController is disabled.\n");
    
    const TimeStamp_t currentTime = HAL::GetTimeStamp();
    
    StartFlashingHelper(_leftEye, currentTime, leftShape, leftOnPeriod_ms, leftOffPeriod_ms);
    StartFlashingHelper(_rightEye, currentTime, rightShape, rightOnPeriod_ms, rightOffPeriod_ms);
    
    _eyeAnimMode = FLASH;
  }

  void StartSpinning(u16 period_ms, bool leftClockWise, bool rightClockWise)
  {
    AnkiConditionalWarnAndReturn(_enable, "EyeController.StartSpinning.Disabled",
                                 "StartSpinning() command ignored: EyeController is disabled.\n");
    
    const TimeStamp_t currentTime = HAL::GetTimeStamp();
    
    StartSpinningHelper(_leftEye, currentTime, period_ms, leftClockWise);
    StartSpinningHelper(_rightEye, currentTime, period_ms, rightClockWise);
    
    _eyeAnimMode = SPIN;
  }
  
  void StopAnimating()
  {
    AnkiConditionalWarnAndReturn(_enable, "EyeController.StopAnimating.Disabled",
                                 "StopAnimating() command ignored: EyeController is disabled.\n");
    _eyeAnimMode = NONE;
  }
  
  void SetBlinkVariability(s32 variation_ms)
  {
    AnkiConditionalWarnAndReturn(_enable, "EyeController.SetBlinkVariability.Disabled",
                                 "SetBlinkVariability() command ignored: EyeController is disabled.\n");
    _blinkVariation_ms = variation_ms;
  }
  
  Result Update()
  {
    if(!_enable) {
      return RESULT_OK;
    }

    TimeStamp_t currentTime = HAL::GetTimeStamp();
    
    if(currentTime - _blinkStart_ms >= _blinkSpacings_ms[_blinkIndex]) {
      
      HAL::FaceBlink();
      
      _blinkStart_ms = currentTime;
      
      ++_blinkIndex;
      if(_blinkIndex == NUM_BLINKS) {
        _blinkIndex = 0;
      }
    }
    
    return RESULT_OK;
    
  } // Update()
  
  
#if 0
#pragma mark ---- Static Helpers ----
#endif
  
  void StartBlinkingHelper(Eye& eye, TimeStamp_t currentTime,
                           u16 onPeriod_ms, u16 offPeriod_ms,
                           bool toMiddle)
  {
    if(eye.color == LED_OFF) {
      AnkiWarn("EyeController.StartBlinkingHelper.NoEyeColor",
               "Eye is currently off: blinking will not have any effect.\n");
    }


    const s32 length = (toMiddle ? BLINK_TO_MIDDLE_ANIM_LENGTH : BLINK_TO_BOTTOM_ANIM_LENGTH);
    TimeStamp_t* timings = (toMiddle ? eye.blinkToMiddleTimings : eye.blinkToBottomTimings);
    
    const TimeStamp_t offTime = offPeriod_ms/(length-1);
    if(onPeriod_ms != timings[0] || offTime != timings[1])
    {
      eye.nextSwitchTime = currentTime;
      timings[0] = onPeriod_ms;
      for(s32 i=1; i<length; ++i) {
        timings[i] = offTime;
      }
      eye.animIndex = 0;
    }
    
  } // StartBlinkingHelper()
  
  
  void StartFlashingHelper(Eye& eye, TimeStamp_t currentTime,
                           EyeShape shape, u16 onPeriod_ms, u16 offPeriod_ms)
  {
    
    if(shape != eye.flashSettings.shape ||
       onPeriod_ms != eye.flashSettings.timings[1] ||
       offPeriod_ms != eye.flashSettings.timings[0])
    {
      eye.animIndex = 0;
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
  
  
  void BlinkHelper(Eye& eye, TimeStamp_t currentTime, bool toMiddle)
  {
    if(currentTime > eye.nextSwitchTime) {
      ++eye.animIndex;
      if(eye.animIndex == (toMiddle ? BLINK_TO_MIDDLE_ANIM_LENGTH : BLINK_TO_BOTTOM_ANIM_LENGTH)) {
        eye.animIndex = 0;
      }
      
      SetEyeShapeHelper(eye, (toMiddle ?
                              BlinkAnimation[eye.animIndex] :
                              BlinkAnimation_ToBtm[eye.animIndex]));
      
      eye.nextSwitchTime = currentTime + (toMiddle ?
                                          eye.blinkToMiddleTimings[eye.animIndex] :
                                          eye.blinkToBottomTimings[eye.animIndex]);

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
