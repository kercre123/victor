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
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/vision/engine/image.h"

#include "opencv2/highgui.hpp"

#include <inttypes.h>
#include <unordered_map>

namespace Anki {
namespace Vector {
   
namespace {
  static const std::string kFaultURL = "support.anki.com";

  // Map of fault codes that map to images that should be drawn instead of the number
  std::unordered_map<uint16_t, std::string> kFaultImageMap = {
    {FaultCode::SHUTDOWN_BATTERY_CRITICAL_TEMP, "/anki/data/assets/cozmo_resources/config/devOnlySprites/independentSprites/battery_overheated.png"},
  };
}

void DrawFaultCode(uint16_t fault)
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

  // Draw fault URL centered horizontally and slightly above
  // the bottom of the screen
  size = Vision::Image::GetTextSize(kFaultURL, 0.5, 1);
  img.DrawTextCenteredHorizontally(kFaultURL,
				   CV_FONT_NORMAL,
				   0.5,
				   1,
				   NamedColors::WHITE,
				   FACE_DISPLAY_HEIGHT - size.y(),
				   false);

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

}
}

extern "C" void core_common_on_exit(void)
{
  // Don't shutdown the lcd in order to keep the fault code displayed
  //lcd_shutdown();
}
  
int main(int argc, char * argv[])
{
  using namespace Anki::Vector;
  
  lcd_init();

  char* end;
  // Convert first argument from a string to a uint16_t
  uint16_t code = (uint16_t)strtoimax(argv[1], &end, 10);

  // See if an image or number should be drawn
  bool imageDrawn = false;
  auto faultImageIt = kFaultImageMap.find(code);
  if (faultImageIt != kFaultImageMap.end()) {
    imageDrawn = DrawImage(faultImageIt->second);
  } 
  
  // Draw fault code if no image to draw or DrawImage() failed
  if (!imageDrawn) {
    DrawFaultCode(code);
  }

  // Don't shutdown the lcd in order to keep the fault code displayed
  //lcd_shutdown();

  return 0;
}
