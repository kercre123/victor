/**
 * File: enrolledFaceEntry.h
 *
 * Author: Andrew Stein
 * Date:   2/18/2016
 *
 * Description: Container for holding bookkeeping data for faces enrolled for face recognition.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Anki_Vision_EnrolledFaceEntry_H__
#define __Anki_Vision_EnrolledFaceEntry_H__

#include "anki/vision/basestation/trackedFace.h"

#include <ctime>

// Forward declaration
namespace Json {
  class Value;
}


namespace Anki {
namespace Vision {

  
struct EnrolledFaceEntry
{
  TrackedFace::ID_t         faceID;
  TrackedFace::ID_t         prevID = TrackedFace::UnknownFace;
  
  std::string               name;
  
  time_t                    enrollmentTime;         // when first added to album
  time_t                    lastDataUpdateTime;     // last time data was updated
  
  s32                       score      = 1000;      // [0,1000]
  s32                       oldestData = 0;         // index of last data update
  
  EnrolledFaceEntry(TrackedFace::ID_t withID = TrackedFace::UnknownFace);
  EnrolledFaceEntry(TrackedFace::ID_t withID, Json::Value& json);
  
  void MergeWith(EnrolledFaceEntry& otherEntry);
  
  void FillJson(Json::Value& json) const;
  
}; // class EnrolledFaceEntry
  
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_EnrolledFaceEntry_H__


