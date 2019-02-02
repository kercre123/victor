/**
 * File: trackedPet.h
 *
 * Author: Andrew Stein
 * Date:   05/09/2016
 *
 * Description: A container for a tracked pet.
 *
 * Copyright: Anki, Inc. 2016
 **/
 
#ifndef __Anki_Vision_TrackedPet_H__
#define __Anki_Vision_TrackedPet_H__

#include "coretech/common/shared/math/rect.h"
#include "coretech/common/shared/math/rect_impl.h"
#include "coretech/common/engine/math/pose.h"
#include "coretech/common/shared/math/radians.h"

#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/faceIdTypes.h"

#include "clad/types/petTypes.h"

namespace Anki {
namespace Vision {
  
class TrackedPet
{
public:
  
  // Constructor:
  TrackedPet(s32 ID) : _id(ID) { }
  
  TimeStamp_t GetTimeStamp()        const          { return _timestamp; }
  void        SetTimeStamp(TimeStamp_t timestamp)  { _timestamp = timestamp; }
  
  float       GetScore()            const          { return _score; }
  void        SetScore(float score)                { _score = score; }
  
  FaceID_t    GetID()               const          { return _id; }
  void        SetID(FaceID_t newID)                { _id = newID; }
  
  PetType     GetType()             const          { return _type; }
  void        SetType(PetType type)                { _type = type; }

  // Roll w.r.t. the original observer (i.e. the camera at observation time)
  Radians     GetHeadRoll()         const          { return _roll; }
  void        SetHeadRoll(Radians roll)            { _roll = roll; }

  s32         GetNumTimesObserved() const          { return _numTimesObserved; }
  void        SetNumTimesObserved(s32 N)           { _numTimesObserved = N;    }
  
  // Location of detection in the image
  const Rectangle<f32>&  GetRect() const                 { return _rect; }
  void                   SetRect(Rectangle<f32>&& rect)  { _rect = rect; }
  
  // Returns true if tracking is happening vs. false if pet was just detected
  bool IsBeingTracked() const                      { return _isBeingTracked; }
  void SetIsBeingTracked(bool tf)                  { _isBeingTracked = tf;   }
  
private:
  
  s32            _id               = -1;
  s32            _numTimesObserved = 0;
  f32            _score            = 0.f;
  TimeStamp_t    _timestamp        = 0;
  PetType        _type             = PetType::Unknown;
  bool           _isBeingTracked   = false;
  
  Rectangle<f32> _rect;
  Radians        _roll; 
  
}; // class TrackedPet
  
} // namespace Vision
} // namespace Anki

#endif
