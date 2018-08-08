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

namespace Anki {
namespace Vector {

#if ANKI_CPU_PROFILER_ENABLED
  CONSOLE_VAR_RANGED(float, maxDrawTime_ms,      ANKI_CPU_CONSOLEVARGROUP, 5, 5, 32);
  CONSOLE_VAR_ENUM(u8,      kDrawFace_Logging,   ANKI_CPU_CONSOLEVARGROUP, 0, Util::CpuProfiler::CpuProfilerLogging());
#endif

namespace {
  int _faultCodeFifo = -1;
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
  // No boot anim in simulator
#ifdef SIMULATOR
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

  _faultCodeThread = std::thread(&FaceDisplay::FaultCodeLoop, this);
}

FaceDisplay::~FaceDisplay()
{
#if REMOTE_CONSOLE_ENABLED
  sDisplayImpl = nullptr;
#endif

  _stopDrawFace = true;
  _faceDrawThread.join();

  StopFaultCodeThread();
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

void FaceDisplay::DrawFaultCode(uint16_t fault)
{
  std::lock_guard<std::mutex> lock(_faceDrawMutex);

  // Image in which the fault code is drawn
  static Vision::ImageRGB img(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);

  // Copy of what was being displayed on the face before showing the fault code
  static Vision::ImageRGB565 imgBeforeFault(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);

  // Save the current image being displayed if we did not have a fault code
  // but do now and there has been an image drawn
  const bool saveCurImg = (_fault == 0 && fault != 0);

  // If fault code 0, then show the saved image
  // as long as there has been one. Otherwise
  // imgBeforeFault will be empty
  if(fault == 0)
  {
    if(_faceDrawLastImg != nullptr && _fault != 0)
    {
      UpdateNextImgPtr();
      imgBeforeFault.CopyTo(*_faceDrawNextImg);
    }
    _fault = fault;
    return;
  }

  if(saveCurImg && _faceDrawLastImg != nullptr)
  {
    _faceDrawLastImg->CopyTo(imgBeforeFault);
  }

  _fault = fault;

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

  UpdateNextImgPtr();
  _faceDrawNextImg->SetFromImageRGB(img);
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

void FaceDisplay::FaultCodeLoop()
{
  // If the fifo doesn't exist create it
  if(access(FaultCode::kFaultCodeFifoName, F_OK) == -1)
  {
    int res = mkfifo(FaultCode::kFaultCodeFifoName, S_IRUSR | S_IWUSR);
    if(res < 0)
    {
      printf("FaceDisplay.FaultCodeLoop.mkfifoFailed %d", errno);
      return;
    }
  }

  // Open fifo for read+write to avoid blocking on open.
  _faultCodeFifo = open(FaultCode::kFaultCodeFifoName, O_RDWR);
  if(_faultCodeFifo < 0)
  {
    LOG_WARNING("FaceDisplay.FaultCodeLoop.OpenFifoFailed", "%d", errno);
    return;
  }

  // Wait 10 seconds before trying to read and draw fault codes
  // in order to let things stabilize and have time to do startup fault checks.
  // If condition variable is signalled for shutdown, the wait returns immediately.

  {
    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> lock(_faultCodeMutex);
    if (!_faultCodeStop) {
      _faultCodeCondition.wait_for(lock, 10s);
    }
  }

  const ssize_t kFaultSize = sizeof(uint16_t);
  u8 buf[128];

  while (!_faultCodeStop)
  {
    // Blocks until there is data available
    const ssize_t rc = read(_faultCodeFifo, buf, sizeof(buf));
    if (rc < 0)
    {
      LOG_WARNING("FaceDisplay.FaultCodeLoop.ReadFailed", "rc %zd errno %d", rc, errno);
      close(_faultCodeFifo);
      _faultCodeFifo = -1;
      return;
    }

    ssize_t numBytes = rc;

    // Pull off kFaultSize number of bytes from the read data
    // and try to draw it, repeat until there is not enough data
    // left
    uint16_t maxFault = _fault;
    while(numBytes >= kFaultSize)
    {
      uint16_t newFault;
      memcpy(&newFault, buf, kFaultSize);
      memmove(buf, buf + kFaultSize, numBytes - kFaultSize);
      numBytes -= kFaultSize;

      // Only show largest fault code
      // or 0 which clears the fault code screen
      if(newFault > maxFault || newFault == 0)
      {
        maxFault = newFault;
      }
    }

    if (_enableFaultCodeDisplay) {

      if(!_stopBootAnim)
      {
        StopBootAnim();

        uint32_t count = 0;
        // Sleep spin waiting for the boot animation to be stopped
        // since it is stopped by a background task
        while(!_stopBootAnim)
        {
          static const uint32_t kSleepTime_ms = 10;
          std::this_thread::sleep_for(std::chrono::milliseconds(kSleepTime_ms));

          static const uint32_t kMaxWait_ms = 5000;
          if(count++ > (kMaxWait_ms / kSleepTime_ms))
          {
            // Something has gone horrible wrong, we have been waiting for the boot anim
            // to stop for kMaxWait_ms. Time to completely give up and exit
            PRINT_NAMED_ERROR("FaceDisplay.FaultCodeLoop.WaitingForBootAnimToStop",
                              "Boot anim hasn't stopped after %dms, exiting",
                              kMaxWait_ms);
            exit(1);
          }
        }
      }
      
      DrawFaultCode(maxFault);
    }
  }
}

void FaceDisplay::StopFaultCodeThread()
{
  //
  // Set stop flag and notify any waiters on condition.
  // If worker thread is waiting on condition, wait will return immediately.
  //
  {
    std::lock_guard<std::mutex> lock(_faultCodeMutex);
    _faultCodeStop = true;
    _faultCodeCondition.notify_all();
  }

  //
  // If fifo is open, write a no-op value and then close it.
  // If worker thread is blocked in read, it will wake up immediately
  // to process the no-op.
  if (_faultCodeFifo > 0)
  {
    (void) write(_faultCodeFifo, &_fault, sizeof(_fault));
    close(_faultCodeFifo);
    _faultCodeFifo = -1;
  }

  // Wait for worker thread to exit.
  if (_faultCodeThread.joinable()) {
    _faultCodeThread.join();
  }
}

void FaceDisplay::StopBootAnim()
{
  if(!_stopBootAnim)
  {
    // Have systemd stop the boot animation process, early-anim
    // Will do nothing if it is not running
    ExecCommandInBackground({"systemctl", "stop", "early-anim"},
     [this](int rc)
      {
        if(rc != 0)
        {
          PRINT_NAMED_WARNING("FaceDisplay.StopBootAnim.StopFailed", "%d", rc);

          // Asking nicely didn't work so try something more aggressive
          ExecCommandInBackground({"systemctl", "kill", "-s", "9", "early-anim"},
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
