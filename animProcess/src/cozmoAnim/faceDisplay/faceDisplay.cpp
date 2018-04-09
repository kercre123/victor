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
#include "util/threading/threadPriority.h"

#include "opencv2/highgui.hpp"

#include <chrono>

namespace Anki {
namespace Cozmo {

namespace {
  int _faultCodeSock = -1;
  uint16_t _fault = FaultCode::NONE;

  static const std::string kFaultURL = "support.anki.com";
}
  
FaceDisplay::FaceDisplay()
  : _displayImpl(new FaceDisplayImpl())
{
  // Set up our thread running data
  _faceDrawImg[0].reset(new Vision::ImageRGB565());
  _faceDrawImg[0]->Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  _faceDrawImg[1].reset(new Vision::ImageRGB565());
  _faceDrawImg[1]->Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  _faceDrawThread = std::thread(&FaceDisplay::DrawFaceLoop, this);

  _faultCodeThread = std::thread(&FaceDisplay::FaultCodeLoop, this);
  _faultCodeThread.detach();
}

FaceDisplay::~FaceDisplay()
{
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
    // Lock because we're about to check and change pointers
    _faceDrawMutex.lock();
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

      _displayImpl->FaceDraw(drawImage.GetRawDataPointer());

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
  // Unlink the socket name incase it already exists
  unlink(FaultCode::kFaultCodeSocketName);

  struct sockaddr_un name;
  size_t size;

  // Create the local socket
  _faultCodeSock = socket(PF_LOCAL, SOCK_DGRAM, 0);
  if(_faultCodeSock < 0)
  {
    PRINT_NAMED_WARNING("FaceDisplay.FaultCodeLoop.CreateSocketFailed", "%d", errno);
    return;
  }

  name.sun_family = AF_LOCAL;
  strncpy(name.sun_path, FaultCode::kFaultCodeSocketName, sizeof(name.sun_path));
  name.sun_path[sizeof(name.sun_path) - 1] = '\0';
  size = (offsetof(struct sockaddr_un, sun_path) + strlen(name.sun_path));

  // Bind the socket to the kFaultCodeSocketName
  int rc = bind(_faultCodeSock, (struct sockaddr*)&name, size);
  if(rc < 0)
  {
    PRINT_NAMED_WARNING("FaceDisplay.FaultCodeLoop.BindSocketFailed","%d", errno);
    return;
  }

  const ssize_t kFaultSize = sizeof(uint16_t);
  u8 buf[128];
  do
  {
    // Block until read has data
    rc = read(_faultCodeSock, buf, sizeof(buf));
    int numBytes = rc;
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

    DrawFaultCode(maxFault);
    
  } while(rc > 0);
}

void FaceDisplay::StopFaultCodeThread()
{
  // Close and unlink the socket if it exists
  if(_faultCodeSock > 0)
  {
    close(_faultCodeSock);
    unlink(FaultCode::kFaultCodeSocketName);
  }
}


} // namespace Cozmo
} // namespace Anki
