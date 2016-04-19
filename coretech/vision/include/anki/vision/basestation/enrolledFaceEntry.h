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

#include "anki/vision/basestation/faceIdTypes.h"

#include <chrono>
#include <string>

// Forward declaration
namespace Json {
  class Value;
}


namespace Anki {
namespace Vision {

struct EnrolledFaceEntry
{
  FaceID_t         faceID;
  FaceID_t         prevID = UnknownFaceID;
  
  std::string               name;
  
  using Time = std::chrono::time_point<std::chrono::system_clock>;
  
  Time                      enrollmentTime;         // when first added to album
  Time                      lastDataUpdateTime;     // last time data was updated
  
  s32                       score          = 1000;  // [0,1000]
  s32                       oldestData     = 0;     // index of last data update
  s32                       numEnrollments = 0;     // how many enrollment "examples" stored
  
  bool                      isForThisSessionOnly = true;
  
  EnrolledFaceEntry(FaceID_t withID = UnknownFaceID);
  
  // Faces constructed from Json default to _not_ being for this session only
  EnrolledFaceEntry(FaceID_t withID, Json::Value& json);
  
  // NOTE: Just sums numEnrollments
  void MergeWith(EnrolledFaceEntry& otherEntry);
  
  void FillJson(Json::Value& json) const;
  
}; // class EnrolledFaceEntry
  
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_EnrolledFaceEntry_H__


