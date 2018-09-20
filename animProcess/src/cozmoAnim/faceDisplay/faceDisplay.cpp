/**
 * File: faceDisplay.cpp
 *
 * Author: Kevin Yoon
 * Created: 12/12/2017
 *
 * Description:
 *               Defines interface to face display
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/faceDisplay/faceDisplayImpl.h"
#include "cozmoAnim/faceDisplay/faceInfoScreenManager.h"
#include "coretech/common/engine/array2d_impl.h"
#include "coretech/vision/engine/image.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/threading/threadPriority.h"
#include "cozmoAnim/execCommand/exec_command.h"

#include "opencv2/highgui.hpp"

#include <chrono>
#include <errno.h>
#include <unistd.h>

#define LOG_CHANNEL "FaceDisplay"

// Whether or not we need to manually stop the boot animation process, vic-bootAnim
#define MANUALLY_STOP_BOOT_ANIM 0

namespace Anki {
namespace Vector {

#if ANKI_CPU_PROFILER_ENABLED
  CONSOLE_VAR_RANGED(float, maxDrawTime_ms,      ANKI_CPU_CONSOLEVARGROUP, 5, 5, 32);
  CONSOLE_VAR_ENUM(u8,      kDrawFace_Logging,   ANKI_CPU_CONSOLEVARGROUP, 0, Util::CpuProfiler::CpuProfilerLogging());
#endif

namespace {
  uint16_t _fault = FaultCode::NONE;

  static const std::string kFaultURL = "support.anki.com";

#if REMOTE_CONSOLE_ENABLED
  FaceDisplayImpl* sDisplayImpl = nullptr;

  void SetFaceBrightness(ConsoleFunctionContextRef context) {
    if( nullptr == sDisplayImpl ) {
      return;
    }

    const int val = ConsoleArg_GetOptional_Int(context, "val", 1);

    if( val >= 0 && val <= 20 ) {
      sDisplayImpl->SetFaceBrightness(val);
    }
    else {
      LOG_WARNING("FaceDisplay.SetFaceBrightness.Invalid",
                  "Brightness value %d is invalid, refusing to set",
                  val);
    }
  }

  CONSOLE_FUNC(SetFaceBrightness, "FaceDisplay", int val);
#endif
}
  
FaceDisplay::FaceDisplay()
: _stopBootAnim(false)
{
  // Don't try to stop the boot anim in sim
  // or if we are not supposed to manunually stop it
  // (systemd will stop it for us)
#if defined(SIMULATOR) || !MANUALLY_STOP_BOOT_ANIM
  _stopBootAnim = true;
#endif
	
  // The boot anim process may be using the display so don't actually create
  // a FaceDisplay until we know for sure that process is no longer running
  _displayImpl.reset(nullptr);
  
  // Set up our thread running data
  _faceDrawImg[0].reset(new Vision::ImageRGB565());
  _faceDrawImg[0]->Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  _faceDrawImg[1].reset(new Vision::ImageRGB565());
  _faceDrawImg[1]->Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  _faceDrawThread = std::thread(&FaceDisplay::DrawFaceLoop, this);
}

FaceDisplay::~FaceDisplay()
{
#if REMOTE_CONSOLE_ENABLED
  sDisplayImpl = nullptr;
#endif

  _stopDrawFace = true;
  _faceDrawThread.join();
}

void FaceDisplay::UpdateNextImgPtr()
{
  if(_faceDrawNextImg == nullptr)
  {
    _faceDrawNextImg = _faceDrawImg[0].get();
    if(_faceDrawCurImg == _faceDrawNextImg)
    {
      _faceDrawNextImg = _faceDrawImg[1].get();
    }
  }
}

void FaceDisplay::DrawToFaceDebug(const Vision::ImageRGB565& img)
{
  // We want to allow FaceInfoScreenManager to draw in the None screen in particular
  // in order to clear since there are no eyes to clear it for us.
  if (!FACTORY_TEST && !FaceInfoScreenManager::getInstance()->IsActivelyDrawingToScreen())
  {
    return;
  }

  DrawToFaceInternal(img);
}

void FaceDisplay::SetFaceBrightness(LCDBrightness level)
{
  if(_displayImpl != nullptr)
  {
    _displayImpl->SetFaceBrightness(EnumToUnderlyingType(level));
  }
}

void FaceDisplay::DrawToFace(const Vision::ImageRGB565& img)
{
  if (FaceInfoScreenManager::getInstance()->IsActivelyDrawingToScreen())
  {
    return;
  }

  DrawToFaceInternal(img);
}

void FaceDisplay::DrawToFaceInternal(const Vision::ImageRGB565& img)
{
  // Don't update images and pointers while the boot animation is still playing
  if(!_stopBootAnim)
  {
    return;
  }

  std::lock_guard<std::mutex> lock(_faceDrawMutex);
  // Prevent drawing if we have a fault code
  if(_fault == 0)
  {
    UpdateNextImgPtr();
    img.CopyTo(*_faceDrawNextImg);
  }
}

void FaceDisplay::DrawFaceLoop()
{
  Anki::Util::SetThreadName(pthread_self(), "DrawFaceLoop");
  while (!_stopDrawFace)
  {
    ANKI_CPU_TICK("FaceDisplay::DrawFaceLoop", maxDrawTime_ms, Util::CpuProfiler::CpuProfilerLoggingTime(kDrawFace_Logging));

    // Lock because we're about to check and change pointers
    _faceDrawMutex.lock();

    if(_stopBootAnim && _displayImpl == nullptr)
    {      
     // Actually create a FaceDisplay which will open a connection to the LCD
     // now that no other process is using it
     _displayImpl.reset(new FaceDisplayImpl());

#if REMOTE_CONSOLE_ENABLED
     sDisplayImpl = _displayImpl.get();
 #endif
    }
    
    if (_faceDrawNextImg != nullptr)
    {
      _faceDrawCurImg = _faceDrawNextImg;
      _faceDrawNextImg = nullptr;
    }
    if (_faceDrawCurImg != nullptr)
    {
      // Grab a reference to the image we're going to draw so we can release the mutex
      const auto& drawImage = *_faceDrawCurImg;
      _faceDrawMutex.unlock();

      // Note: for VIC-1873 it's possible to take a copy of the face buffer here
      //       and then pass to screen capture for converting to .tga or .gif
      //       as per the original animationStreamer version.

      //       However, it should be noted having the code here is a significant
      //       impact to performance, both visually on the robot and dropped frames
      //       in the .gif file

      // Only draw to the face once the boot anim has been stopped
      if(_displayImpl != nullptr && _stopBootAnim)
      {
        _displayImpl->FaceDraw(drawImage.GetRawDataPointer());
      }

      // Done with this image, clear the pointer
      {
        std::lock_guard<std::mutex> lock(_faceDrawMutex);
        _faceDrawLastImg = _faceDrawCurImg;
        _faceDrawCurImg = nullptr;
      }
    }
    else
    {
      _faceDrawMutex.unlock();

      // Sleep before checking again whether we've got an image to draw
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }
}

void FaceDisplay::StopBootAnim()
{
  if(!_stopBootAnim)
  {
    // Have systemd stop the boot animation process, vic-bootAnim
    // Will do nothing if it is not running
    ExecCommandInBackground({"systemctl", "stop", "vic-bootAnim"},
     [this](int rc)
      {
        if(rc != 0)
        {
          PRINT_NAMED_WARNING("FaceDisplay.StopBootAnim.StopFailed", "%d", rc);

          // Asking nicely didn't work so try something more aggressive
          ExecCommandInBackground({"systemctl", "kill", "-s", "9", "vic-bootAnim"},
            [this](int rc)
            {
              // Killing didn't work for some reason so error and show fault code
              if(rc != 0)
              {
                PRINT_NAMED_ERROR("FaceDisplay.StopBootAnim.KillFailed", "%d", rc);
                FaultCode::DisplayFaultCode(FaultCode::STOP_BOOT_ANIM_FAILED);
              }
              else
              {
                _stopBootAnim = true;
              }
            });
        }
        else
        {
          _stopBootAnim = true;
        }
      });
  }
}


} // namespace Vector
} // namespace Anki
