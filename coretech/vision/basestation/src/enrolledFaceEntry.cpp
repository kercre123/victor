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
#include "util/logging/logging.h"

#include <json/json.h>

namespace Anki {
namespace Vision {

EnrolledFaceEntry::EnrolledFaceEntry(FaceID_t withID)
: faceID(withID)
, enrollmentTime(std::chrono::milliseconds(0))
, lastDataUpdateTime(std::chrono::milliseconds(0))
, numEnrollments(1)
, isForThisSessionOnly(true)
{

}

EnrolledFaceEntry::EnrolledFaceEntry(FaceID_t withID, Json::Value& json)
: faceID(withID)
, isForThisSessionOnly(false)
{
  auto MissingFieldWarning = [withID](const char* fieldName) {
    PRINT_NAMED_WARNING("EnrolledFaceEntry.ConstructFromJson.MissingField",
                        "Missing '%s' field for ID %d", fieldName, withID);
  };
  
  Json::LargestInt numTicks = 0;
  if(!json.isMember("enrollmentTime")) {
    MissingFieldWarning("enrollmentTime");
  } else {
    numTicks = json["enrollmentTime"].asLargestInt();
  }
  enrollmentTime = Time(std::chrono::milliseconds(numTicks));
  
  if(!json.isMember("oldestData")) {
    MissingFieldWarning("oldestData");
    oldestData = 0;
  } else {
    oldestData = json["oldestData"].asInt();
  }
  
  if(!json.isMember("lastDataUpdateTime")) {
    MissingFieldWarning("lastDataUpdateTime");
    numTicks = 0;
  } else {
    numTicks = json["lastDataUpdateTime"].asLargestInt();
  }
  lastDataUpdateTime = Time(std::chrono::milliseconds(numTicks));
  
  if(!json.isMember("name")) {
    MissingFieldWarning("name");
  } else {
    name = json["name"].asString();
  }
  
  if(!json.isMember("numEnrollments")) {
    MissingFieldWarning("numEnrollments");
    numEnrollments = 1;
  } else {
    numEnrollments = json["numEnrollments"].asInt();
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
  
  // Keep my name unless I don't have one
  if(name.empty()) {
    name = otherEntry.name;
  }
  
  enrollmentTime = std::min(enrollmentTime, otherEntry.enrollmentTime);
  score = std::max(score, otherEntry.score);
  numEnrollments += otherEntry.numEnrollments;
}
  
void EnrolledFaceEntry::FillJson(Json::Value& entry) const
{
  // NOTE: Save times as "number of ticks since epoch"
  entry["name"]               = name;
  entry["oldestData"]         = oldestData;
  entry["enrollmentTime"]     = (Json::LargestInt)enrollmentTime.time_since_epoch().count();
  entry["lastDataUpdateTime"] = (Json::LargestInt)lastDataUpdateTime.time_since_epoch().count();
  entry["numEnrollments"]     = numEnrollments;
}

} // namespace Vision
} // namespace Anki


