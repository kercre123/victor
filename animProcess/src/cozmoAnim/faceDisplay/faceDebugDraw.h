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

#include "anki/common/types.h"
#include "anki/common/basestation/math/point.h"
#include "clad/robotInterface/messageEngineToRobot.h"

#include <memory>

namespace Anki {

class ColorRGBA;

namespace Vision {
  class ImageRGB;
}

namespace Cozmo {

namespace RobotInterface {
  struct MicData;
  struct MicDirection;
} 

class FaceDebugDraw {
public:
  FaceDebugDraw();

  enum class DrawState {
    None,

    FAC,
    GeneralInfo,
    SensorInfo1,
    SensorInfo2,
    MicInfo,
    MicDirectionClock,
    CustomText,

    Count
  };

  // Debug drawing is expected from only one thread
  DrawState GetDrawState() const { return _drawState; }
  void ChangeDrawState();

  void SetShouldDrawFAC(bool draw);

  // Begin drawing functionality
  void DrawConfidenceClock(const RobotInterface::MicDirection& micData);
  void DrawStateInfo(const RobotState& state);
  void DrawMicInfo(const RobotInterface::MicData& micData);
  void DrawFAC();
  void SetCustomText(const RobotInterface::DrawTextOnScreen& text) { _customText = text; DrawCustomText(); }
  
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
  
  void DrawCustomText();

  bool _drawFAC = false;

  RobotInterface::DrawTextOnScreen _customText;
};

} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_FaceDisplay_FaceDebugDraw_H_
