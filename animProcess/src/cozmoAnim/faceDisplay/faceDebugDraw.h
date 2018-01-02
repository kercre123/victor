/**
* File: faceDebugDraw.h
*
* Author: Lee Crippen
* Created: 12/19/2017
*
* Description: Handles drawing debug data to the robot's face.
* Usage: Add drawing functionality as needed from various components. Add a corresponding DrawState.
*        In the new drawing functionality, return early if the DrawState does not match appropriately.
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __AnimProcess_CozmoAnim_FaceDisplay_FaceDebugDraw_H_
#define __AnimProcess_CozmoAnim_FaceDisplay_FaceDebugDraw_H_

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/point.h"
#include <memory>

namespace Anki {

class ColorRGBA;

namespace Vision {
  class ImageRGB;
}

namespace Cozmo {

namespace RobotInterface {
  struct MicDirection;
}

class FaceDebugDraw {
public:
  FaceDebugDraw();

  enum class DrawState {
    None,
    MicDirectionClock,

    Count
  };

  // Debug drawing is expected from only one thread
  DrawState GetDrawState() const { return _drawState; }
  void ChangeDrawState();

  // Begin drawing functionality
  void DrawConfidenceClock(const RobotInterface::MicDirection& micData);
  
private:
  DrawState                         _drawState = DrawState::None;
  std::unique_ptr<Vision::ImageRGB> _scratchDrawingImg;

  // Helper methods for drawing debug data to face
  void ClearFace();
  void DrawTextOnScreen(const std::vector<std::string>& textVec, 
                        const ColorRGBA& textColor,
                        const ColorRGBA& bgColor,
                        const Point2f& loc = {0, 0},
                        u32 textSpacing_pix = 10,
                        f32 textScale = 3.f);
};

} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_FaceDisplay_FaceDebugDraw_H_
