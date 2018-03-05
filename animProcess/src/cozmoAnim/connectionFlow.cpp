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
#include "cozmoAnim/faceDisplay/faceDisplay.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/image_impl.h"

#include "clad/robotInterface/messageEngineToRobot.h"

#include "util/console/consoleSystem.h"
#include "util/logging/logging.h"

#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"

namespace Anki {
namespace Cozmo {

namespace {
std::string _name = "Vector 0000";
u32 _pin = 123456;

const std::string kURL = "vector.anki.com";
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
                                  Vision::ImageRGB565& img,
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

  Vision::ImageRGB565 img(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  img.FillWith(Vision::PixelRGB565(0, 0, 0));

  DrawTextCenteredHorizontally(_name, CV_FONT_NORMAL, 0.5f, 1, img, kColor, 0, false);

  cv::Size textSize;
  float scale = 0;
  MakeTextFillImageWidth(kURL, CV_FONT_NORMAL, 1, img.GetNumCols(), textSize, scale);
  DrawTextCenteredHorizontally(kURL, CV_FONT_NORMAL, scale, 1, img, kColor, (FACE_DISPLAY_HEIGHT-textSize.height)/2, true);

  animStreamer->SetFaceImage(img, 0);
}

// Draws BLE name, key icon, and BLE pin to screen
void DrawShowPinScreen(AnimationStreamer* animStreamer)
{
  Vision::ImageRGB key;
  key.Load("/anki/data/assets/cozmo_resources/config/facePNGs/key.png");

  Vision::ImageRGB img(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  img.FillWith(0);

  Point2f p((FACE_DISPLAY_WIDTH - key.GetNumCols())/2,
            (FACE_DISPLAY_HEIGHT - key.GetNumRows())/2);
  img.DrawSubImage(key, p);

  Vision::ImageRGB565 i;
  i.SetFromImageRGB(img);

  DrawTextCenteredHorizontally(_name, CV_FONT_NORMAL, 0.5f, 1, i, kColor, 0, false);

  DrawTextCenteredHorizontally(std::to_string(_pin), CV_FONT_NORMAL, 0.6f, 1, i, kColor, FACE_DISPLAY_HEIGHT-15, false);

  animStreamer->SetFaceImage(i, 0);
}

// Uses a png sequence animation to draw wifi icon to screen
void DrawWifiScreen(AnimationStreamer* animStreamer)
{
  // Taking advantage of a "feature" of the neon hsv2rgb565 converter
  // where a hue of 255 outputs white since we don't have control over
  // face saturation and value in order to achieve white nor do we currently
  // support non-grayscale png sequences
  ProceduralFace::SetHue(255);
  animStreamer->Abort();
  // TODO(Al): Shouldn't need to lock tracks since actual animations won't move them
  animStreamer->SetLockedTracks((u8)AnimTrackFlag::ALL_TRACKS);
  animStreamer->UnlockTrack(AnimTrackFlag::FACE_IMAGE_TRACK);
  animStreamer->SetStreamingAnimation("anim_timerset_getin_01", 0, 0);
}

// Uses a png sequence animation to draw os updating icon to screen
void DrawUpdatingOSScreen(AnimationStreamer* animStreamer)
{
  animStreamer->Abort();
  animStreamer->SetLockedTracks((u8)AnimTrackFlag::ALL_TRACKS);
  animStreamer->UnlockTrack(AnimTrackFlag::FACE_IMAGE_TRACK);
  animStreamer->SetStreamingAnimation("anim_timerset_getin_02", 0, 0);
}

// Uses a png sequence animation to draw waiting for app icon to screen
void DrawWaitingForAppScreen(AnimationStreamer* animStreamer)
{
  animStreamer->Abort();
  animStreamer->SetLockedTracks((u8)AnimTrackFlag::ALL_TRACKS);
  animStreamer->UnlockTrack(AnimTrackFlag::FACE_IMAGE_TRACK);
  animStreamer->SetStreamingAnimation("anim_bored_event_04", 0, 0);
}

void SetBLEName(const std::string& name)
{
  _name = name;
}

void SetBLEPin(uint32_t pin)
{
  _pin = pin;
}

void UpdateConnectionFlow(const SwitchboardInterface::SetConnectionStatus& msg,
                          AnimationStreamer* animStreamer)
{
  using namespace SwitchboardInterface;

  switch(msg.status)
  {
    case ConnectionStatus::NONE:
    {

    }
    break;
    case ConnectionStatus::START_PAIRING:
    {
      // Throttling square is annoying when trying to inspect the display so disable
      NativeAnkiUtilConsoleSetValueWithString("DisplayThermalThrottling", "false");
      DrawStartPairingScreen(animStreamer);
    }
    break;
    case ConnectionStatus::SHOW_PIN:
    {
      DrawShowPinScreen(animStreamer);
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
    case ConnectionStatus::WAITING_FOR_APP:
    {
      DrawWaitingForAppScreen(animStreamer);
    }
    break;
    case ConnectionStatus::END_PAIRING:
    {
      NativeAnkiUtilConsoleSetValueWithString("DisplayThermalThrottling", "true");
      // Reset hue back to default because it gets changed by Draw(Wifi/OS/App)Screen
      ProceduralFace::ResetHueToDefault();
      animStreamer->Abort();
      // Reenable keep face alive
      animStreamer->EnableKeepFaceAlive(true, 0);
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