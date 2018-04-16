/**
* File: faceInfoScreenManager.h
*
* Author: Lee Crippen
* Created: 12/19/2017
*
* Description: Handles navigation and drawing of the Customer Support Menu / Debug info screens.
*
* Usage: Add drawing functionality as needed from various components. 
*        Add a corresponding ScreenName in faceInfoScreenTypes.h.
*        In the new drawing functionality, return early if the ScreenName does not match appropriately.
*
* Copyright: Anki, Inc. 2017
*
*/

#ifndef __AnimProcess_CozmoAnim_FaceDisplay_FaceInfoScreenManager_H_
#define __AnimProcess_CozmoAnim_FaceDisplay_FaceInfoScreenManager_H_

#include "anki/cozmo/shared/cozmoConfig.h"
#include "coretech/common/shared/types.h"
#include "coretech/common/engine/colorRGBA.h"
#include "coretech/common/engine/math/point.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenTypes.h"
#include "clad/robotInterface/messageEngineToRobot.h"

#include "util/singleton/dynamicSingleton.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Anki {

namespace Vision {
  class ImageRGB565;
}

namespace Cozmo {

class AnimContext;
class AnimationStreamer;
class FaceInfoScreen;
  
namespace RobotInterface {
  struct MicData;
  struct MicDirection;
}

namespace WebService {
  class WebService;
}
  
  
class FaceInfoScreenManager : public Util::DynamicSingleton<FaceInfoScreenManager> {
  
  ANKIUTIL_FRIEND_SINGLETON(FaceInfoScreenManager); // Allows base class singleton access
  
public:
  FaceInfoScreenManager();

  void Init(AnimContext* context, AnimationStreamer* animStreamer);
  void Update(const RobotState& state);
  
  // Debug drawing is expected from only one thread
  ScreenName GetCurrScreenName() const;

  // Returns true if the current screen is one that
  // FaceInfoScreenManager is responsible for drawing to.
  // Otherwise, it's assumed that someone else is doing it.
  bool IsActivelyDrawingToScreen() const;

  void SetShouldDrawFAC(bool draw);
  void SetCustomText(const RobotInterface::DrawTextOnScreen& text);  

  // When BLE pairing mode is enabled/disabled, this screen should
  // be called so that the physical inputs (head, lift, button) wheels
  // are handled appropriately. 
  // The FaceInfoScreenManager otherwise does nothing since drawing on 
  // screen is handled by ConnectionFlow when in pairing mode.
  void EnablePairingScreen(bool enable);

  // Begin drawing functionality
  // These functions update the screen only if they are relevant to the current screen
  void DrawConfidenceClock(const RobotInterface::MicDirection& micData,
                           float bufferFullPercent,
                           uint32_t secondsRemaining,
                           bool triggerRecognized);
  void DrawMicInfo(const RobotInterface::MicData& micData);
  void DrawCameraImage(const Vision::ImageRGB565& img);
  
private:
  std::unique_ptr<Vision::ImageRGB565> _scratchDrawingImg;

  // Sets the current screen
  void SetScreen(ScreenName screen);
  
  // Gets the current screen
  FaceInfoScreen* GetScreen(ScreenName name);
  
  // Process wheel, head, lift, button motion for menu navigation
  void ProcessMenuNavigation(const RobotState& state);
  u32 _wheelMovingForwardsCount;
  u32 _wheelMovingBackwardsCount;
  bool _liftTriggerReady;
  bool _headTriggerReady;
  
  // Flag indicating when debug screens have been unlocked
  bool _debugInfoScreensUnlocked;
  
  // Map of all screen names to their associated screen objects
  std::unordered_map<ScreenName, FaceInfoScreen> _screenMap;
  FaceInfoScreen* _currScreen;

  // Internal draw functions that
  void DrawFAC();
  void DrawMain();
  void DrawNetwork();
  void DrawSensorInfo(const RobotState& state);
  void DrawIMUInfo(const RobotState& state);
  void DrawMotorInfo(const RobotState& state);
  void DrawCustomText();

  // Draw the _scratchDrawingImg to the face
  void DrawScratch();

  // Updates the FAC screen if needed
  void UpdateFAC();
  
  static const Point2f kDefaultTextStartingLoc_pix;
  static const u32 kDefaultTextSpacing_pix;
  static const f32 kDefaultTextScale;

  // Helper methods for drawing debug data to face
  void DrawTextOnScreen(const std::vector<std::string>& textVec, 
                        const ColorRGBA& textColor = NamedColors::WHITE,
                        const ColorRGBA& bgColor = NamedColors::BLACK,
                        const Point2f& loc = kDefaultTextStartingLoc_pix,
                        u32 textSpacing_pix = kDefaultTextSpacing_pix,
                        f32 textScale = kDefaultTextScale);

  struct ColoredText {
    ColoredText(const std::string& text, const ColorRGBA& color = NamedColors::WHITE)
    : text(text)
    , color(color)
    {}

    const std::string text;
    const ColorRGBA color;
  };

  using ColoredTextLines = std::vector<std::vector<ColoredText> >;
  void DrawTextOnScreen(const ColoredTextLines& lines, 
                        const ColorRGBA& bgColor = NamedColors::BLACK,
                        const Point2f& loc = kDefaultTextStartingLoc_pix,
                        u32 textSpacing_pix = kDefaultTextSpacing_pix,
                        f32 textScale = kDefaultTextScale);

  RobotInterface::DrawTextOnScreen _customText;
  WebService::WebService* _webService;
  
  bool _drawFAC = false;
  
  // Reboot Linux
  void Reboot();

};

} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_FaceDisplay_FaceInfoScreenManager_H_
