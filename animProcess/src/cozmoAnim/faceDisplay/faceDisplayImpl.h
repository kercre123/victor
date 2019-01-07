/**
 * File: faceDisplay.h
 *
 * Author: Kevin Yoon
 * Created: 07/20/2017
 *
 * Description:
 *               Defines interface to face display
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef ANKI_COZMOANIM_FACE_DISPLAY_IMPL_H
#define ANKI_COZMOANIM_FACE_DISPLAY_IMPL_H
#include "coretech/common/shared/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"


#ifdef SIMULATOR
namespace webots {
  class Supervisor;
}
#endif

namespace Anki {
namespace Vector {
  
class FaceDisplayImpl
{
public:
  FaceDisplayImpl();
  ~FaceDisplayImpl();
  
  // Clears the face display
  void FaceClear();
  
  // Draws frame to face display
  // 'frame' is a buffer of FACE_DISPLAY_WIDTH x FACE_DISPLAY_HEIGHT u16s where each pixel
  // is a RGB_565 value. You can use OpenCV to create the frame like so.
  //
  //      Vision::ImageRGB testImg;
  //      testImg.Load("testPattern.jpg");
  //      testImg.Resize(FaceDisplay::FACE_DISPLAY_HEIGHT, FaceDisplay::FACE_DISPLAY_WIDTH);
  //      cv::Mat img565;
  //      cv::cvtColor(testImg.get_CvMat_(), img565, cv::COLOR_RGB2BGR565);
  //      FaceDisplay::getInstance()->FaceDraw(reinterpret_cast<u16*>(img565.ptr()));
  void FaceDraw(const u16* frame);

  // Print text to face display
  void FacePrintf(const char *format, ...);

  // set face display brightness (int 0..20)
  void SetFaceBrightness(int level);
  
#ifdef SIMULATOR
  // Assign Webots supervisor
  // Webots processes must do this before creating FaceDisplay for the first time.
  // Unit test processes must call SetSupervisor(nullptr) to run without a supervisor.
  static void SetSupervisor(webots::Supervisor* sup);
#endif
  
private:
  
}; // class FaceDisplayImpl
  
} // namespace Vector
} // namespace Anki

#endif // ANKI_COZMOANIM_FACE_DISPLAY_IMPL_H
