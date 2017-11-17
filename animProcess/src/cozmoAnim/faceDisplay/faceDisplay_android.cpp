/**
 * File: faceDisplay_android.cpp
 *
 * Author: Kevin Yoon
 * Created: 07/20/2017
 *
 * Description:
 *               Defines interface to simulated face display
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cozmoAnim/faceDisplay/faceDisplay.h"

#include "core/lcd.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
  // Definition of static field
  FaceDisplay* FaceDisplay::_instance = 0;
  
  /**
   * Returns the single instance of the object.
   */
  FaceDisplay* FaceDisplay::getInstance() {
    // check if the instance has been created yet
    if(0 == _instance) {
      // if not, then create it
      _instance = new FaceDisplay;
    }
    // return the single instance
    return _instance;
  }
  
  /**
   * Removes instance
   */
  void FaceDisplay::removeInstance() {
    // check if the instance has been created yet
    if(0 != _instance) {
      delete _instance;
      _instance = 0;
    }
  };
  
  
  FaceDisplay::FaceDisplay()
  {
    lcd_init();
  }

  FaceDisplay::~FaceDisplay()
  {
    FaceClear();
    lcd_shutdown();
  }
  
  void FaceDisplay::FaceClear()
  {
    u16 frame[FACE_DISPLAY_WIDTH*FACE_DISPLAY_HEIGHT*sizeof(u16)];
    FaceDraw(frame);
  }
  
  void FaceDisplay::FaceDraw(const u16* frame)
  {
    lcd_draw_frame2(frame, FACE_DISPLAY_WIDTH*FACE_DISPLAY_HEIGHT*sizeof(u16));
  }
  
  void FaceDisplay::FacePrintf(const char* format, ...)
  {
    // Stub
  }

} // namespace Cozmo
} // namespace Anki

extern "C" void on_exit(void)
{
  lcd_shutdown();
}
