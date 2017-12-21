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
#include "cozmoAnim/faceDisplay/faceDebugDraw.h"
#include "anki/common/basestation/array2d_impl.h"
#include "anki/vision/basestation/image.h"
#include "util/threading/threadPriority.h"

#include <chrono>

namespace Anki {
namespace Cozmo {

FaceDisplay::FaceDisplay()
  : _displayImpl(new FaceDisplayImpl())
  , _faceDebugDraw(new FaceDebugDraw())
{
  // Set up our thread running data
  _faceDrawImg[0].reset(new Vision::ImageRGB565());
  _faceDrawImg[0]->Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  _faceDrawImg[1].reset(new Vision::ImageRGB565());
  _faceDrawImg[1]->Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  _faceDrawThread = std::thread(&FaceDisplay::DrawFaceLoop, this);
}

FaceDisplay::~FaceDisplay()
{
  _stopDrawFace = true;
  _faceDrawThread.join();
}

void FaceDisplay::DrawToFaceDebug(const Vision::ImageRGB& img)
{
  if (_faceDebugDraw->GetDrawState() == FaceDebugDraw::DrawState::None)
  {
    return;
  }

  DrawToFaceInternal(img);
}

void FaceDisplay::DrawToFace(const Vision::ImageRGB565& img)
{
  if (_faceDebugDraw->GetDrawState() != FaceDebugDraw::DrawState::None)
  {
    return;
  }

  DrawToFaceInternal(img);
}

void FaceDisplay::DrawToFaceInternal(const Vision::ImageRGB& img)
{
  std::lock_guard<std::mutex> lock(_faceDrawMutex);
  if (_faceDrawNextImg == nullptr)
  {
    _faceDrawNextImg = _faceDrawImg[0].get();
    if (_faceDrawCurImg == _faceDrawNextImg)
    {
      _faceDrawNextImg = _faceDrawImg[1].get();
    }
  }
  _faceDrawNextImg->SetFromImageRGB(img);
}

void FaceDisplay::DrawToFaceInternal(const Vision::ImageRGB565& img)
{
  std::lock_guard<std::mutex> lock(_faceDrawMutex);
  if (_faceDrawNextImg == nullptr)
  {
    _faceDrawNextImg = _faceDrawImg[0].get();
    if (_faceDrawCurImg == _faceDrawNextImg)
    {
      _faceDrawNextImg = _faceDrawImg[1].get();
    }
  }
  img.CopyTo(*_faceDrawNextImg);
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

      _displayImpl->FaceDraw(drawImage.GetRawDataPointer());

      // Done with this image, clear the pointer
      {
        std::lock_guard<std::mutex> lock(_faceDrawMutex);
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

} // namespace Cozmo
} // namespace Anki
