/**
 * File: eyeController.h
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

#include "anki/cozmo/shared/cozmoTypes.h"

namespace Anki {
namespace Cozmo {
namespace EyeController {
  
  Result Init();
  
  Result Update();
  
  // Change color without affecting mode (e.g. continue blinking)
  // Use LED_CURRENT_COLOR to keep the color as is.
  void SetEyeColor(u32 color);
  void SetEyeColor(u32 leftColor, u32 rightColor);
  
  // Set the eyes to a specific shape (this disables any current eye animation)
  // Use EYE_CURRENT_SHAPE to keep the shape in whatever state it already is.
  void SetEyeShape(EyeShape shape);
  void SetEyeShape(EyeShape leftShape, EyeShape rightShape);

  void StartBlinking(u16 onPeriod_ms, u16 offPeriod_ms);
  void StartBlinking(u16 leftOnPeriod_ms,  u16 leftOffPeriod_ms,
                     u16 rightOnPeriod_ms, u16 rightOffPeriod_ms);

  void StartFlashing(EyeShape shape, u16 onPeriod_ms, u16 offPeriod_ms);
  void StartFlashing(EyeShape leftShape, u16 leftOnPeriod_ms, u16 leftOffPeriod_ms,
                     EyeShape rightShape, u16 rightOnPeriod_ms, u16 rightOffPeriod_ms);
  
  void StartSpinning(u16 period_ms, bool leftClockWise, bool rightClockWise);
  
  // Stop blinking, cycling, etc, the eyes and leave the eyes in whatever
  // state (shape/color) they were in
  void StopAnimating();
  
} // namespace EyeController
} // namespace Cozmo
} // namespace Anki
