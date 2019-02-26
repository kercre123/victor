/**
* File: faultCodeDisplay.cpp
*
* Author: Al Chaussee
* Date:   7/26/2018
*
* Description: Displays the first argument to the screen
*
* Copyright: Anki, Inc. 2018
**/

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/shared/factory/faultCodes.h"
#include "core/lcd.h"
#include "coretech/common/shared/array2d_impl.h"
#include "coretech/vision/engine/image.h"
#include "rescue/pairing_icon_key.h"
#include "opencv2/highgui.hpp"

#include <getopt.h>
#include <inttypes.h>
#include <unordered_map>

namespace Anki {
namespace Vector {

namespace {
  constexpr const char * kSupportURL = "support.anki.com";
  constexpr const char * kVectorWillRestart = "Vector will restart";

  // BRC-TODO: Embed this image directly into binary
  constexpr const char * kPairingIconImagePath = 
    "/anki/data/assets/cozmo_resources/config/devOnlySprites/independentSprites/pairing_icon_key.png";

  const f32 kRobotNameScale = 0.6f;
  const std::string kURL = "anki.com/v";
  // const ColorRGBA   kColor(0.9f, 0.9f, 0.9f, 1.f);
  const ColorRGBA     kColor(0.9f, 0.f, 0.f, 1.f);
}

extern "C" void core_common_on_exit(void)
{
  lcd_shutdown();
}

void DrawFaultCode(uint16_t fault, bool willRestart)
{
  // Image in which the fault code is drawn
  static Vision::ImageRGB img(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);

  img.FillWith(0);

  // Draw the fault code centered horizontally
  const std::string faultString = std::to_string(fault);
  Vec2f size = Vision::Image::GetTextSize(faultString, 1.5,  1);
  img.DrawTextCenteredHorizontally(faultString,
				   CV_FONT_NORMAL,
				   1.5,
				   2,
				   NamedColors::WHITE,
				   (FACE_DISPLAY_HEIGHT/2 + size.y()/4),
				   false);

  // Draw text centered horizontally and slightly above
  // the bottom of the screen
  const std::string & text = (willRestart ? kVectorWillRestart : kSupportURL);
  const int font = CV_FONT_NORMAL;
  const f32 scale = 0.5f;
  const int thickness = 1;
  const auto color = NamedColors::WHITE;
  const bool drawTwice = false;

  size = Vision::Image::GetTextSize(text, scale, thickness);
  img.DrawTextCenteredHorizontally(text, font, scale, thickness, color, FACE_DISPLAY_HEIGHT - size.y(), drawTwice);

  Vision::ImageRGB565 img565(img);
  lcd_draw_frame2(reinterpret_cast<u16*>(img565.GetDataPointer()), img565.GetNumRows() * img565.GetNumCols() * sizeof(u16));
}

bool DrawImage(std::string& image_path)
{
  Vision::ImageRGB565 img565;
  if (img565.Load(image_path) != RESULT_OK) {
    return false;
  }

  // Fail if the image isn't the right size
  if (img565.GetNumCols() != FACE_DISPLAY_WIDTH || img565.GetNumRows() != FACE_DISPLAY_HEIGHT) {
    return false;
  }

  lcd_draw_frame2(reinterpret_cast<u16*>(img565.GetDataPointer()), img565.GetNumRows() * img565.GetNumCols() * sizeof(u16));
  return true;
}

// Draws BLE name and url to screen
bool DrawStartPairingScreen(const std::string& robotName)
{
  if(robotName == "")
  {
    return false;
  }
  
  auto img = Vision::ImageRGBA(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  img.FillWith(Vision::PixelRGBA(0, 0));

  img.DrawTextCenteredHorizontally(robotName, CV_FONT_NORMAL, kRobotNameScale, 1, kColor, 15, false);

  cv::Size textSize;
  float scale = 0;
  Vision::Image::MakeTextFillImageWidth(kURL, CV_FONT_NORMAL, 1, img.GetNumCols(), textSize, scale);
  img.DrawTextCenteredHorizontally(kURL, CV_FONT_NORMAL, scale, 1, kColor, (FACE_DISPLAY_HEIGHT + textSize.height)/2, true);

  Vision::ImageRGB565 img565(img);
  lcd_draw_frame2(reinterpret_cast<u16*>(img565.GetDataPointer()),
                  img565.GetNumRows() * img565.GetNumCols() * sizeof(u16));

  return true;
}

// Draws BLE name, key icon, and BLE pin to screen
void DrawShowPinScreen(const std::string& robotName, const std::string& pin)
{
  // Somehow read the pairing icon
  Vision::ImageRGB key;
  key.Load(kPairingIconImagePath);

  auto img = Vision::ImageRGBA(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  img.FillWith(Vision::PixelRGBA(0, 0));

  Point2f p((FACE_DISPLAY_WIDTH - key.GetNumCols())/2,
            (FACE_DISPLAY_HEIGHT - key.GetNumRows())/2);
  img.DrawSubImage(key, p);

  img.DrawTextCenteredHorizontally(robotName, CV_FONT_NORMAL, kRobotNameScale, 1, kColor, 15, false);

  img.DrawTextCenteredHorizontally(pin, CV_FONT_NORMAL, 0.8f, 1, kColor, FACE_DISPLAY_HEIGHT-5, false);

  Vision::ImageRGB565 img565(img);
  lcd_draw_frame2(reinterpret_cast<u16*>(img565.GetDataPointer()),
                  img565.GetNumRows() * img565.GetNumCols() * sizeof(u16));
}

}
}
