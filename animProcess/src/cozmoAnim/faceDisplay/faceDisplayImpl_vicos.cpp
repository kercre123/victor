/**
 * File: faceDisplayImpl_vicos.cpp
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

#include "cozmoAnim/faceDisplay/faceDisplayImpl.h"

#include "core/lcd.h"

#include "util/logging/logging.h"

#define WORKING_LCD 1

namespace Anki {
namespace Cozmo {

  FaceDisplayImpl::FaceDisplayImpl()
  {
#if WORKING_LCD
    lcd_init();
#endif
  }

  FaceDisplayImpl::~FaceDisplayImpl()
  {
#if WORKING_LCD
    FaceClear();
    lcd_shutdown();
#endif
  }
  
  void FaceDisplayImpl::FaceClear()
  {
#if WORKING_LCD
    lcd_clear_screen();
#endif
  }
  
  void FaceDisplayImpl::FaceDraw(const u16* frame)
  {
#if WORKING_LCD
    lcd_draw_frame2(frame, FACE_DISPLAY_WIDTH*FACE_DISPLAY_HEIGHT*sizeof(u16));
#endif
  }
  
  void FaceDisplayImpl::FacePrintf(const char* format, ...)
  {
    // Stub
  }

} // namespace Cozmo
} // namespace Anki

extern "C" void core_common_on_exit(void)
{
  lcd_shutdown();
}
