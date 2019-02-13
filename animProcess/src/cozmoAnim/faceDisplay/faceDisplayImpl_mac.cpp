/**
 * File: faceDisplayImpl_mac.cpp
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

#ifndef WEBOTS

#include "cozmoAnim/faceDisplay/faceDisplayImpl.h"
#include "coretech/vision/engine/colorPixelTypes.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {

#pragma mark --- Simulated Hardware Method Implementations ---
  
  FaceDisplayImpl::FaceDisplayImpl()
  {
  }

  FaceDisplayImpl::~FaceDisplayImpl() = default;

  void FaceDisplayImpl::FaceClear()
  {
  }
  
  void FaceDisplayImpl::FaceDraw(const u16* frame)
  {
  }
  
  void FaceDisplayImpl::FacePrintf(const char* format, ...)
  {
  }

  void FaceDisplayImpl::SetFaceBrightness(int level)
  {
    // not supported for mac
  }

} // namespace Vector
} // namespace Anki

#endif // ndef WEBOTS
