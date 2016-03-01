/**
 * File: enrolledFaceEntry.cpp
 *
 * Author: Andrew Stein
 * Date:   2/18/2016
 *
 * Description: Container for holding bookkeeping data for faces enrolled for face recognition.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "anki/vision/basestation/enrolledFaceEntry.h"

#include <json/json.h>

namespace Anki {
namespace Vision {

EnrolledFaceEntry::EnrolledFaceEntry(TrackedFace::ID_t withID)
: faceID(withID)
, enrollmentTime(time(0))
, lastDataUpdateTime(time(0))
{
  
}

EnrolledFaceEntry::EnrolledFaceEntry(TrackedFace::ID_t withID, Json::Value& json)
: faceID(withID)
{
  auto MissingFieldWarning = [withID](const char* fieldName) {
    PRINT_NAMED_WARNING("EnrolledFaceEntry.ConstructFromJson.MissingField",
                        "Missing '%s' field for ID %d", fieldName, withID);
  };
  
  
  if(!json.isMember("enrollmentTime")) {
    MissingFieldWarning("enrollmentTime");
    enrollmentTime = time(0);
  } else {
    enrollmentTime = (time_t)json["enrollmentTime"].asLargestInt();
  }
  
  if(!json.isMember("oldestData")) {
    MissingFieldWarning("oldestData");
    oldestData = 0;
  } else {
    oldestData = json["oldestData"].asInt();
  }
  
  if(!json.isMember("lastDataUpdateTime")) {
    MissingFieldWarning("lastDataUpdateTime");
    lastDataUpdateTime = time(0);
  } else {
    lastDataUpdateTime = (time_t)json["lastDataUpdateTime"].asLargestInt();
  }
}


void EnrolledFaceEntry::MergeWith(EnrolledFaceEntry& otherEntry)
{
  // Keep "this" faceID but notify that other ID was merged into this one if the other
  // one was not brand new
  if(otherEntry.prevID == otherEntry.faceID) {
    prevID = otherEntry.faceID;
  }
  
  if(otherEntry.lastDataUpdateTime > lastDataUpdateTime)
  {
    oldestData = otherEntry.oldestData; // Does this even matter?
    lastDataUpdateTime = otherEntry.lastDataUpdateTime;
  }
  
  enrollmentTime = std::min(enrollmentTime, otherEntry.enrollmentTime);
  score = std::max(score, otherEntry.score);
}
  
void EnrolledFaceEntry::FillJson(Json::Value& entry) const
{
  entry["oldestData"]         = oldestData;
  entry["enrollmentTime"]     = (Json::LargestInt)enrollmentTime;
  entry["lastDataUpdateTime"] = (Json::LargestInt)lastDataUpdateTime;
}

} // namespace Vision
} // namespace Anki


