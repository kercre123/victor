/** File: connectionFlow.cpp
 *
 * Author: Al Chaussee
 * Created: 02/28/2018
 *
 * Description: Functions for updating what to display on the face
 *              during various parts of the connection flow
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "cozmoAnim/connectionFlow.h"

#include "cozmoAnim/animation/animationStreamer.h"
#include "cozmoAnim/animComms.h"
#include "cozmoAnim/animContext.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "cozmoAnim/robotDataLoader.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/data/dataScope.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/image_impl.h"

#include "clad/robotInterface/messageEngineToRobot.h"

#include "util/console/consoleSystem.h"
#include "util/logging/logging.h"

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

#include "anki/cozmo/shared/factory/emrHelper.h"

#include "osState/osState.h"

namespace Anki {
namespace Cozmo {

namespace {
u32 _pin = 123456;

const std::string kURL = "anki.com/v";
const ColorRGBA   kColor(0.7f, 0.7f, 0.7f, 1.f);
}

// Outputs textSize and scale in order to make text fill
// imageWidth
void MakeTextFillImageWidth(const std::string& text,
                            int font,
                            int thickness,
                            int imageWidth,
                            cv::Size& textSize,
                            float& scale)
{
  int baseline = 0;
  scale = 0.1f;
  cv::Size prevTextSize(0,0);
  for(; scale < 3.f; scale += 0.05f)
  {
    textSize = cv::getTextSize(text, 
                               CV_FONT_NORMAL,
                               scale, 
                               1, 
                               &baseline);

    if(textSize.width > imageWidth)
    {
      scale -= 0.05f;
      textSize = prevTextSize;
      break;
    }
    prevTextSize = textSize;
  }
}

// Draws text on img centered horizontally at verticalPos
// OpenCV does not draw filled text so depending on the scale
// there may be some empty pixels so drawTwiceToMaybeFillGaps
// draws the text a second time at a slight offset to try and
// fill the gaps
void DrawTextCenteredHorizontally(const std::string text,
                                  int font,
                                  float scale,
                                  int thickness,
                                  Vision::ImageRGBA& img,
                                  const ColorRGBA& color,
                                  int verticalPos,
                                  bool drawTwiceToMaybeFillGaps)
{
  int baseline = 0;
  cv::Size textSize = cv::getTextSize(text, 
                                      font,
                                      scale, 
                                      thickness, 
                                      &baseline);

  Point2f p((img.GetNumCols() - textSize.width)/2, 
            verticalPos + textSize.height);

  img.DrawText(p,
               text,
               color,
               scale,
               false,
               thickness);

  if(drawTwiceToMaybeFillGaps)
  {
    p.y() += 1;

    img.DrawText(p,
               text,
               color,
               scale,
               false,
               thickness);
  }
}

// Draws BLE name and url to screen
void DrawStartPairingScreen(AnimationStreamer* animStreamer)
{
  animStreamer->EnableKeepFaceAlive(false, 0);
  animStreamer->Abort();

  auto* img = new Vision::ImageRGBA(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  img->FillWith(Vision::PixelRGBA(0, 0));

  DrawTextCenteredHorizontally(OSState::getInstance()->GetRobotName(), CV_FONT_NORMAL, 0.5f, 1, *img, kColor, 0, false);

  cv::Size textSize;
  float scale = 0;
  MakeTextFillImageWidth(kURL, CV_FONT_NORMAL, 1, img->GetNumCols(), textSize, scale);
  DrawTextCenteredHorizontally(kURL, CV_FONT_NORMAL, scale, 1, *img, kColor, (FACE_DISPLAY_HEIGHT-textSize.height)/2, true);

  auto handle = std::make_shared<Vision::SpriteWrapper>(img);
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetFaceImage(handle, shouldRenderInEyeHue, 0);
}

// Draws BLE name, key icon, and BLE pin to screen
void DrawShowPinScreen(AnimationStreamer* animStreamer, const AnimContext* context)
{
  Vision::ImageRGB key;
  key.Load(context->GetDataLoader()->GetSpritePaths()->GetValue(Vision::SpriteName::PairingIconKey));

  auto* img = new Vision::ImageRGBA(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  img->FillWith(Vision::PixelRGBA(0, 0));

  Point2f p((FACE_DISPLAY_WIDTH - key.GetNumCols())/2,
            (FACE_DISPLAY_HEIGHT - key.GetNumRows())/2);
  img->DrawSubImage(key, p);

  DrawTextCenteredHorizontally(OSState::getInstance()->GetRobotName(), CV_FONT_NORMAL, 0.5f, 1, *img, kColor, 0, false);

  DrawTextCenteredHorizontally(std::to_string(_pin), CV_FONT_NORMAL, 0.7f, 1, *img, kColor, FACE_DISPLAY_HEIGHT-20, false);

  auto handle = std::make_shared<Vision::SpriteWrapper>(img);
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetFaceImage(handle, shouldRenderInEyeHue, 0);
}

// Uses a png sequence animation to draw wifi icon to screen
void DrawWifiScreen(AnimationStreamer* animStreamer)
{
  const bool shouldInterrupt = true;
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_wifi", 0, 0, shouldInterrupt, shouldRenderInEyeHue);
}

// Uses a png sequence animation to draw os updating icon to screen
void DrawUpdatingOSScreen(AnimationStreamer* animStreamer)
{
  const bool shouldInterrupt = true;
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_update", 0, 0, shouldInterrupt, shouldRenderInEyeHue);
}

// Uses a png sequence animation to draw os updating error icon to screen
void DrawUpdatingOSErrorScreen(AnimationStreamer* animStreamer)
{
  const bool shouldInterrupt = true;
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_update_error", 0, 0, shouldInterrupt, shouldRenderInEyeHue);
}

// Uses a png sequence animation to draw os updating error icon to screen
void DrawUpdatingOSErrorScreen(AnimationStreamer* animStreamer)
{
  animStreamer->SetStreamingAnimation("anim_pairing_icon_update_error", 0, 0);
}

// Uses a png sequence animation to draw waiting for app icon to screen
void DrawWaitingForAppScreen(AnimationStreamer* animStreamer)
{
  const bool shouldInterrupt = true;
  const bool shouldRenderInEyeHue = false;
  animStreamer->SetStreamingAnimation("anim_pairing_icon_awaitingapp", 0, 0, shouldInterrupt, shouldRenderInEyeHue);
}

void SetBLEPin(uint32_t pin)
{
  _pin = pin;
}

void InitConnectionFlow(AnimationStreamer* animStreamer)
{
  if(FACTORY_TEST)
  {
    // Don't start connection flow if not packed out
    if(!Factory::GetEMR()->fields.PACKED_OUT_FLAG)
    {
      return;
    }

    DrawStartPairingScreen(animStreamer);
  }
}

void UpdatePairingLight(bool on)
{
  static bool isOn = false;
  if(!isOn && on)
  {
    // Start system pairing light (pulsing orange/green)
    RobotInterface::EngineToRobot m(RobotInterface::SetSystemLight({
          .light = {
            .onColor = 0xFFFF0000,
            .offColor = 0x00000000,
            .onFrames = 16,
            .offFrames = 16,
            .transitionOnFrames = 16,
            .transitionOffFrames = 16,
            .offset = 0
          }}));
    AnimComms::SendPacketToRobot((char*)m.GetBuffer(), m.Size());
    isOn = on; 
  }
  else if(isOn && !on)
  {
    // Turn system pairing light off
    RobotInterface::EngineToRobot m(RobotInterface::SetSystemLight({
          .light = {
            .onColor = 0x00000000,
            .offColor = 0x00000000,
            .onFrames = 1,
            .offFrames = 1,
            .transitionOnFrames = 0,
            .transitionOffFrames = 0,
            .offset = 0
          }}));
    AnimComms::SendPacketToRobot((char*)m.GetBuffer(), m.Size());
    isOn = on;
  }
}
  
bool IsInConnectionFlow()
{
  using namespace SwitchboardInterface;
  return (_curStatus != ConnectionStatus::NONE &&
          _curStatus != ConnectionStatus::START_PAIRING &&
          _curStatus != ConnectionStatus::END_PAIRING);

}

void UpdateConnectionFlow(const SwitchboardInterface::SetConnectionStatus& msg,
                          AnimationStreamer* animStreamer,
                          const AnimContext* context)
{
  using namespace SwitchboardInterface;

  // Update the pairing light
  // Turn it on if we are on the START_PAIRING or SHOW_PIN screen
  // Otherwise turn it off
  UpdatePairingLight((msg.status == ConnectionStatus::START_PAIRING ||
                      msg.status == ConnectionStatus::SHOW_PIN));
  

  _curStatus = msg.status;

  switch(msg.status)
  {
    case ConnectionStatus::NONE:
    {

    }
    break;
    case ConnectionStatus::START_PAIRING:
    {
      FaceInfoScreenManager::getInstance()->EnablePairingScreen(true);
   
      // Throttling square is annoying when trying to inspect the display so disable
      NativeAnkiUtilConsoleSetValueWithString("DisplayThermalThrottling", "false");
      DrawStartPairingScreen(animStreamer);
    }
    break;
    case ConnectionStatus::SHOW_PIN:
    {
      DrawShowPinScreen(animStreamer, context);
    }
    break;
    case ConnectionStatus::SETTING_WIFI:
    {
      DrawWifiScreen(animStreamer);
     }
    break;
    case ConnectionStatus::UPDATING_OS:
    {
      DrawUpdatingOSScreen(animStreamer);
    }
    break;
    case ConnectionStatus::UPDATING_OS_ERROR:
    {
      DrawUpdatingOSErrorScreen(animStreamer);
    }
    break;
    case ConnectionStatus::WAITING_FOR_APP:
    {
      DrawWaitingForAppScreen(animStreamer);
    }
    break;
    case ConnectionStatus::END_PAIRING:
    {
      NativeAnkiUtilConsoleSetValueWithString("DisplayThermalThrottling", "true");
      animStreamer->Abort();

      // Probably will never get here because we will restart
      // while updating os
      if(FACTORY_TEST)
      {
        DrawStartPairingScreen(animStreamer);
      }
      else
      {
        // Reenable keep face alive
        animStreamer->EnableKeepFaceAlive(true, 0);
      }
      FaceInfoScreenManager::getInstance()->EnablePairingScreen(false);
    }
    break;
    case ConnectionStatus::COUNT:
    {

    }
    break;
  }
}

}
}
