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
, lastSeenTime(std::chrono::milliseconds(0))
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
  
  if(!json.isMember("nextDataToUpdate")) {
    MissingFieldWarning("nextDataToUpdate");
    nextDataToUpdate = 0;
  } else {
    nextDataToUpdate = json["nextDataToUpdate"].asInt();
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
  
  if(!json.isMember("lastSeenTime")) {
    MissingFieldWarning("lastSeenTime");
    numTicks = 0;
  } else {
    numTicks = json["lastSeenTime"].asLargestInt();
  }
  lastSeenTime = Time(std::chrono::milliseconds(numTicks));
  
}
  
EnrolledFaceEntry::EnrolledFaceEntry(const ExternalInterface::EnrolledFaceMessage& message)
: faceID(message.faceID)
, name(message.name)
, enrollmentTime(std::chrono::milliseconds(message.enrollmentTimeCount))
, lastDataUpdateTime(std::chrono::milliseconds(message.lastDataUpdateTimeCount))
, lastSeenTime(std::chrono::milliseconds(message.lastSeenTimeCount))
, nextDataToUpdate(message.nextDataToUpdate)
, numEnrollments(message.numEnrollments)
, isForThisSessionOnly(false)
{
  
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
    nextDataToUpdate = otherEntry.nextDataToUpdate; // Does this even matter?
    lastDataUpdateTime = otherEntry.lastDataUpdateTime;
  }
  
  // Keep most recent lastSeenTime
  if(otherEntry.lastSeenTime > lastSeenTime) {
    lastSeenTime = otherEntry.lastSeenTime;
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
  
  using namespace std::chrono;
  
  // NOTE: Save times as "number of ticks since epoch"
  entry["name"]               = name;
  entry["nextDataToUpdate"]   = nextDataToUpdate;
  entry["enrollmentTime"]     = (Json::LargestInt)duration_cast<milliseconds>(enrollmentTime.time_since_epoch()).count();
  entry["lastDataUpdateTime"] = (Json::LargestInt)duration_cast<milliseconds>(lastDataUpdateTime.time_since_epoch()).count();
  entry["lastSeenTime"]       = (Json::LargestInt)duration_cast<milliseconds>(lastSeenTime.time_since_epoch()).count();
  entry["numEnrollments"]     = numEnrollments;
  // NOTE: Not storing faceID because we're assuming it's being used as the key for the entire entry
}
  


EnrolledFaceEntry::operator ExternalInterface::EnrolledFaceMessage() const
{
  ExternalInterface::EnrolledFaceMessage message;
  using namespace std::chrono;
  
  message.enrollmentTimeCount     = duration_cast<milliseconds>(this->enrollmentTime.time_since_epoch()).count();
  message.lastDataUpdateTimeCount = duration_cast<milliseconds>(this->enrollmentTime.time_since_epoch()).count();
  message.lastSeenTimeCount       = duration_cast<milliseconds>(this->lastSeenTime.time_since_epoch()).count();
  message.faceID                  = this->faceID;
  message.nextDataToUpdate        = this->nextDataToUpdate;
  message.numEnrollments          = this->numEnrollments;
  message.name                    = this->name;
  
  return message;
}
 
  
void EnrolledFaceEntry::Serialize(std::vector<u8>& buffer) const
{
  // Create the message
  const ExternalInterface::EnrolledFaceMessage message(*this);
  
  // Pack it into a temp buffer
  const size_t expectedNumBytes = message.Size();
  buffer.resize(buffer.size() + expectedNumBytes);
  const size_t numBytes = message.Pack(&(buffer[buffer.size()-expectedNumBytes]), expectedNumBytes);
  
  ASSERT_NAMED_EVENT(numBytes == expectedNumBytes,
                     "EnrolledFaceEntry.Serialize.WrongPackedSize",
                     "Packed %zu, Expected %zu", numBytes, expectedNumBytes);
  
  PRINT_NAMED_DEBUG("EnrolledFaceEntry.Serialize.Success",
                    "Serialized entry for '%s', ID=%d. Added %zu bytes to buffer (total length now %zu)",
                    name.c_str(), faceID, numBytes, buffer.size());
  
} // Serialize()

  
Result EnrolledFaceEntry::Deserialize(const std::vector<u8>& buffer, size_t& startIndex)
{
  if(buffer.empty()) {
    PRINT_NAMED_WARNING("EnrolledFaceEntry.Deserialize.EmptyBuffer", "");
    return RESULT_FAIL;
  } else if(startIndex == buffer.size()) {
    PRINT_NAMED_WARNING("EnrolledFaceEntry.Deserialize.StartIndexAtEnd", "");
    return RESULT_FAIL;
  }
  
  // Deserialize using CLAD features
  ExternalInterface::EnrolledFaceMessage message;
  const size_t expectedNumBytes = message.Size();
  if(startIndex + expectedNumBytes > buffer.size()) {
    PRINT_NAMED_WARNING("EnrolledFaceEntry.Deserialize.BufferTooShort",
                        "Not enough bytes from start=%zu to end=%zu. Expecting %zu.",
                        startIndex, buffer.size(), expectedNumBytes);
    return RESULT_FAIL;
  }
  
  const size_t numBytes = message.Unpack(&(buffer[startIndex]), buffer.size() - startIndex);
  if(numBytes < expectedNumBytes) { // Note: numBytes can be > expectedNumBytes b/c of variable name length
    PRINT_NAMED_WARNING("EnrolledFaceEntry.Deserialize.WrongNumBytesUnPacked",
                        "Unpacked %zu from buffer. Expecting >= %zu.",
                        numBytes, expectedNumBytes);
    return RESULT_FAIL;
  }
  
  // Now set this object's members and increment the startIndex for the next call
  faceID               = message.faceID;
  name                 = message.name;
  enrollmentTime       = Time(std::chrono::milliseconds(message.enrollmentTimeCount));
  lastDataUpdateTime   = Time(std::chrono::milliseconds(message.lastDataUpdateTimeCount));
  lastSeenTime         = Time(std::chrono::milliseconds(message.lastSeenTimeCount));
  nextDataToUpdate     = message.nextDataToUpdate;
  numEnrollments       = message.numEnrollments;
  isForThisSessionOnly = false;
  
  // Increment by the number of bytes we read
  startIndex += numBytes;
  
  return RESULT_OK;
} // Deserialize()

  
} // namespace Vision
} // namespace Anki


