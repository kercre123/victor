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
  
  static const size_t MaxNameLength = 32;
  
  // This is a helper object just for (de)serializing enrolled face entries.
  // It serves as a temporary object we can pack the members into that we want
  // to save.
  struct SerializationStruct {
    char        name[MaxNameLength];
    long long   enrollmentTimeCount;
    long long   lastDataUpdateTimeCount;
    FaceID_t    faceID;
    s32         oldestData;
    s32         numEnrollments;
    
    SerializationStruct(const u8* buffer)
    {
      memcpy((u8*)this, buffer, sizeof(SerializationStruct));
    }
    
    SerializationStruct(const EnrolledFaceEntry& entry)
    : enrollmentTimeCount(entry.enrollmentTime.time_since_epoch().count())
    , lastDataUpdateTimeCount(entry.enrollmentTime.time_since_epoch().count())
    , faceID(entry.faceID)
    , oldestData(entry.oldestData)
    , numEnrollments(entry.numEnrollments)
    {
      memset(&name, 0, MaxNameLength);
      if(entry.name.length() > MaxNameLength) {
        PRINT_NAMED_WARNING("EnrolledFaceEntry.SerializationStruct.NameTooLong",
                            "Length-%zu name will be truncated to %zu chars",
                            entry.name.length(), MaxNameLength);
        
        // Only display name in debug message, for privacy reasons
        PRINT_NAMED_DEBUG("EnrolledFaceEntry.SerializationStruct.NameTooLong_Debug",
                          "Name '%s' is longer than max %zu chars",
                          entry.name.c_str(), MaxNameLength);
      }
      snprintf(name, MaxNameLength-1, "%s", entry.name.c_str());
    }
    
    void GetEnrolledFaceEntry(EnrolledFaceEntry& entry) const
    {
      entry.faceID = faceID;
      entry.name = name;
      entry.enrollmentTime = EnrolledFaceEntry::Time(std::chrono::milliseconds(enrollmentTimeCount));
      entry.lastDataUpdateTime = EnrolledFaceEntry::Time(std::chrono::milliseconds(lastDataUpdateTimeCount));
      entry.oldestData = oldestData;
      entry.numEnrollments = numEnrollments;
      
      // If we are deserializing this entry, it must have been saved, so it must not be session-only.
      entry.isForThisSessionOnly = false;
    }
  }; // struct SerializationStruct
  
void EnrolledFaceEntry::Serialize(std::vector<u8>& buffer) const
{
  const SerializationStruct serializedStruct(*this);
  const u8* serializedData = (const u8*)(&serializedStruct);
  buffer.insert(buffer.end(), serializedData, serializedData + sizeof(serializedStruct));
} // Serialize()

Result EnrolledFaceEntry::Deserialize(const std::vector<u8>& buffer, size_t& startIndex)
{
  if(buffer.empty()) {
    PRINT_NAMED_ERROR("EnrolledFaceEntry.Deserialize.EmptyBuffer", "");
    return RESULT_FAIL;
  } else if(startIndex == buffer.size()) {
    PRINT_NAMED_ERROR("EnrolledFaceEntry.Deserialize.StartIndexAtEnd", "");
    return RESULT_FAIL;
  } else if(startIndex + sizeof(SerializationStruct) > buffer.size()) {
    PRINT_NAMED_ERROR("EnrolledFaceEntry.Deserialize.BufferTooShort",
                      "Not enough bytes from start=%zu to end=%zu. Expecting %zu.",
                      startIndex, buffer.size(), sizeof(SerializationStruct));
    return RESULT_FAIL;
  }
  
  SerializationStruct serializedStruct(&(buffer[startIndex]));
  startIndex += sizeof(serializedStruct);
  serializedStruct.GetEnrolledFaceEntry(*this);
  
  return RESULT_OK;
} // Deserialize()

  
} // namespace Vision
} // namespace Anki


