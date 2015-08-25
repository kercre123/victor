/**
 * File: trackedFace.cpp
 *
 * Author: Andrew Stein
 * Date:   8/20/2015
 *
 * Description: A container for a tracked face and any features (e.g. eyes, mouth, ...)
 *              related to it.
 *
 * Copyright: Anki, Inc. 2015
 **/

#include "anki/vision/basestation/trackedFace.h"
#include "anki/vision/basestation/camera.h"

namespace Anki {
namespace Vision {
  
  TrackedFace::TrackedFace()
  : _id(-1)
  , _name("")
  , _isBeingTracked(false)
  {
    
  }
  
  ObservedMarker TrackedFace::GetMarker(Camera& camera) const
  {
    Quad2f quad;
    GetRect().GetQuad(quad);
    
    return ObservedMarker(GetTimeStamp(), Marker::FACE_CODE, quad, camera);
  }
  
} // namespace Vision
} // namespace Anki


