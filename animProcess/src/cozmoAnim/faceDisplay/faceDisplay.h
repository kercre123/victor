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

#ifndef ANKI_COZMOANIM_FACE_DISPLAY_H
#define ANKI_COZMOANIM_FACE_DISPLAY_H
#include "anki/common/types.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/vision/basestation/image.h"
#include "util/singleton/dynamicSingleton.h"

#include <array>
#include <memory>
#include <mutex>
#include <thread>


namespace Anki {
namespace Cozmo {

class FaceDisplayImpl;
  
class FaceDisplay : public Util::DynamicSingleton<FaceDisplay>
{
  ANKIUTIL_FRIEND_SINGLETON(FaceDisplay); // Allows base class singleton access

public:

  // Various methods for drawing to the face. Not expected to be called from 
  // multiple threads (sometimes uses static scratch image for drawing)
  void ClearFace();
  void DrawToFace(const Vision::ImageRGB& img);
  void DrawToFace(const Vision::ImageRGB565& img);
  void DrawTextOnScreen(const std::vector<std::string>& textVec, 
                        const ColorRGBA& textColor,
                        const ColorRGBA& bgColor,
                        const Point2f& loc = {0, 0},
                        u32 textSpacing_pix = 10,
                        f32 textScale = 3.f);

protected:
  FaceDisplay();
  virtual ~FaceDisplay();

private:
  std::unique_ptr<FaceDisplayImpl>  _displayImpl;

  Vision::ImageRGB                  _scratchDrawingImg;

  // Members for managing the drawing thread
  Vision::ImageRGB565               _faceDrawImg[2];
  Vision::ImageRGB565*              _faceDrawNextImg = nullptr;
  Vision::ImageRGB565*              _faceDrawCurImg = nullptr;
  std::thread                       _faceDrawThread;
  std::mutex                        _faceDrawMutex;
  bool                              _stopDrawFace = false;
    
  void DrawFaceLoop();

}; // class FaceDisplay
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMOANIM_FACE_DISPLAY_H
