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


#include "anki/common/basestation/array2d_impl.h"
#include "cozmoAnim/faceDisplay/faceDisplay.h"
#include "cozmoAnim/faceDisplay/faceDisplayImpl.h"
#include "util/threading/threadPriority.h"

#include <chrono>

namespace Anki {
namespace Cozmo {

FaceDisplay::FaceDisplay()
  : _displayImpl(new FaceDisplayImpl())
{
  _scratchDrawingImg.Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);

  // Set up our thread running data
  _faceDrawImg[0].Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  _faceDrawImg[1].Allocate(FACE_DISPLAY_HEIGHT, FACE_DISPLAY_WIDTH);
  _faceDrawThread = std::thread(&FaceDisplay::DrawFaceLoop, this);
}

FaceDisplay::~FaceDisplay()
{
  _stopDrawFace = true;
  _faceDrawThread.join();
}

void FaceDisplay::DrawToFace(const Vision::ImageRGB& img)
{
  std::lock_guard<std::mutex> lock(_faceDrawMutex);
  if (_faceDrawNextImg == nullptr)
  {
    _faceDrawNextImg = &_faceDrawImg[0];
    if (_faceDrawCurImg == _faceDrawNextImg)
    {
      _faceDrawNextImg = &_faceDrawImg[1];
    }
  }
  _faceDrawNextImg->SetFromImageRGB(img);
}

void FaceDisplay::DrawToFace(const Vision::ImageRGB565& img)
{
  std::lock_guard<std::mutex> lock(_faceDrawMutex);
  if (_faceDrawNextImg == nullptr)
  {
    _faceDrawNextImg = &_faceDrawImg[0];
    if (_faceDrawCurImg == _faceDrawNextImg)
    {
      _faceDrawNextImg = &_faceDrawImg[1];
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

// Draws each element of the textVec on a separate line (spacing determined by textSpacing_pix)
// in textColor with a background of bgColor.
void FaceDisplay::DrawTextOnScreen(const std::vector<std::string>& textVec, 
                                   const ColorRGBA& textColor,
                                   const ColorRGBA& bgColor,
                                   const Point2f& loc,
                                   u32 textSpacing_pix,
                                   f32 textScale)
{
  _scratchDrawingImg.FillWith( {bgColor.b(), bgColor.g(), bgColor.r()} );

  const f32 textLocX = loc.x();
  f32 textLocY = loc.y();
  // TODO: Expose line and location(?) as arguments
  const u8  textLineThickness = 8;

  for(const auto& text : textVec)
  {
    _scratchDrawingImg.DrawText(
      {textLocX, textLocY},
      text.c_str(),
      textColor,
      textScale,
      textLineThickness);

    textLocY += textSpacing_pix;
  }

  DrawToFace(_scratchDrawingImg);
}

void FaceDisplay::ClearFace()
{
  const auto& clearColor = NamedColors::BLACK;
  _scratchDrawingImg.FillWith( {clearColor.b(), clearColor.g(), clearColor.r()} );
  DrawToFace(_scratchDrawingImg);
}

} // namespace Cozmo
} // namespace Anki
