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

#include "util/singleton/dynamicSingleton.h"
#include "anki/cozmo/shared/factory/faultCodes.h"

#include "clad/types/lcdTypes.h"

#include <array>
#include <memory>
#include <mutex>
#include <thread>


namespace Anki {

namespace Vision {
  class ImageRGB565;
}

namespace Cozmo {

class FaceDisplayImpl;
class FaceInfoScreenManager;

class FaceDisplay : public Util::DynamicSingleton<FaceDisplay>
{
  ANKIUTIL_FRIEND_SINGLETON(FaceDisplay); // Allows base class singleton access

public:
  void DrawToFace(const Vision::ImageRGB565& img);

  // For drawing to face in various debug modes
  void DrawToFaceDebug(const Vision::ImageRGB565& img);

  void SetFaceBrightness(LCDBrightness level);

protected:
  FaceDisplay();
  virtual ~FaceDisplay();

  void DrawToFaceInternal(const Vision::ImageRGB565& img);

private:
  std::unique_ptr<FaceDisplayImpl>  _displayImpl;

  // Members for managing the drawing thread
  std::unique_ptr<Vision::ImageRGB565>  _faceDrawImg[2];
  Vision::ImageRGB565*                  _faceDrawNextImg = nullptr;
  Vision::ImageRGB565*                  _faceDrawCurImg = nullptr;
  Vision::ImageRGB565*                  _faceDrawLastImg = nullptr;
  std::thread                           _faceDrawThread;
  std::mutex                            _faceDrawMutex;
  bool                                  _stopDrawFace = false;

  void DrawFaceLoop();
  void UpdateNextImgPtr();

  // Main loop of the fault code thread
  void FaultCodeLoop();
  void DrawFaultCode(uint16_t fault);
  void StopFaultCodeThread();

  // Stuff for controlling fault code thread
  std::thread _faultCodeThread;
  std::mutex _faultCodeMutex;
  std::condition_variable _faultCodeCondition;
  bool _faultCodeStop = false;

}; // class FaceDisplay

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMOANIM_FACE_DISPLAY_H
