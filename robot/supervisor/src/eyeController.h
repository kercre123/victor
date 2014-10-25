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
  void SetEyeColor(LEDColor color);
  void SetEyeColor(LEDColor leftColor, LEDColor rightColor);
  
  // Set the eyes to a specific shape (this disables any current eye animation)
  void SetEyeShape(EyeShape shape);
  void SetEyeShape(EyeShape leftShape, EyeShape rightShape);

  void StartBlinking(u16 onPeriod_ms, u16 offPeriod_ms);
  void StartBlinking(u16 leftOnPeriod_ms,  u16 leftOffPeriod_ms,
                     u16 rightOnPeriod_ms, u16 rightOffPeriod_ms);

  // Stop blinking, cycling, etc, the eyes and leave the eyes in whatever
  // state (shape/color) they were in
  void StopAnimating();
  
} // namespace EyeController
} // namespace Cozmo
} // namespace Anki