/**
 * File: enrolledFaceEntry.cpp
 *
 * Author: Andrew Stein
 * Date:   2/18/2016
 *
 * Description: Container for holding bookkeeping data for faces enrolled for face recognition.
 *              This class is used heavily by the FaceRecognizer and makes use of the
 *              EnrolledFaceStorage CLAD struct to serialize/deserialize for storing
 *              in the robot's NVStorage.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#include "coretech/vision/engine/enrolledFaceEntry.h"
#include "util/logging/logging.h"
#include "util/math/numericCast.h"
#include "util/global/globalDefinitions.h"
#include "util/string/stringUtils.h"

#include <json/json.h>

namespace Anki {
namespace Vision {
  
  namespace JsonKey
  {
    const char* enrollmentTime = "enrollmentTime";
    const char* lastDataUpdateTime = "lastDataUpdateTime";
    const char* name = "name";
    const char* albumEntries = "albumEntries";
    const char* albumEntry = "albumEntry";
    const char* lastSeenTime = "lastSeenTime";
  }

inline static Json::LargestInt TimeToInt64(EnrolledFaceEntry::Time time)
{
  using namespace std::chrono;
  auto jsonTime = Util::numeric_cast<Json::LargestInt>(duration_cast<seconds>(time.time_since_epoch()).count());
  return jsonTime;
}

UserName::UserName(const std::string& name)
  : _name( Util::StringToLower(name) )
  , _displayName( name )
{

}

UserName& UserName::operator=( const std::string& name )
{
  _displayName = name;
  _name = Util::StringToLower(name);
  return *this;
}

bool UserName::operator==( const UserName& rhs ) const
{
  return ( _name == rhs._name );
}

bool UserName::operator==( const std::string& rhs ) const
{
  // use a temp UserName to ensure proper formatting of the _name string
  const UserName rhsUserName( rhs );
  return ( *this == rhsUserName );
}

bool UserName::operator==( const char* rhs ) const
{
  // use a temp UserName to ensure proper formatting of the _name string
  const UserName rhsUserName( rhs );
  return ( *this == rhsUserName );
}
  
EnrolledFaceEntry::EnrolledFaceEntry(FaceID_t withID, Time enrollmentTime)
: _faceID(withID)
, _sessionOnlyAlbumEntry(-1)
, _enrollmentTime(enrollmentTime)
, _lastDataUpdateTime(std::chrono::seconds(0))
{

}

EnrolledFaceEntry::EnrolledFaceEntry()
: EnrolledFaceEntry(UnknownFaceID, Time(std::chrono::seconds(0)))
{
  
}
  
EnrolledFaceEntry::EnrolledFaceEntry(FaceID_t withID, Json::Value& json)
: EnrolledFaceEntry(withID, Time(std::chrono::seconds(0)))
{
  auto MissingFieldWarning = [withID](const char* fieldName) {
    PRINT_NAMED_WARNING("EnrolledFaceEntry.ConstructFromJson.MissingField",
                        "Missing '%s' field for ID %d", fieldName, withID);
  };
  
  Json::LargestInt numTicks = 0;
  if(!json.isMember(JsonKey::enrollmentTime)) {
    MissingFieldWarning(JsonKey::enrollmentTime);
  } else {
    numTicks = json[JsonKey::enrollmentTime].asLargestInt();
  }
  _enrollmentTime = Time(std::chrono::seconds(numTicks));
  
  if(!json.isMember(JsonKey::lastDataUpdateTime)) {
    MissingFieldWarning(JsonKey::lastDataUpdateTime);
    numTicks = 0;
  } else {
    numTicks = json[JsonKey::lastDataUpdateTime].asLargestInt();
  }
  _lastDataUpdateTime = Time(std::chrono::seconds(numTicks));
  
  ClipFutureTimesToNow("ConstructFromIDandJson");

  if(!json.isMember(JsonKey::name)) {
    MissingFieldWarning(JsonKey::name);
  } else {
    _name = json[JsonKey::name].asString();
  }
  
  if(!json.isMember(JsonKey::albumEntries)) {
    MissingFieldWarning(JsonKey::albumEntries);
    _albumEntrySeenTimes.clear();
  } else {
    const Json::Value& jsonAlbumEntries = json[JsonKey::albumEntries];
    if(!jsonAlbumEntries.isArray()) {
      PRINT_NAMED_WARNING("EnrolledFaceEntry.ConstructFromJson.AlbumEntriesNotArray", "");
    } else {

      bool sessionOnlySet = false;
      for(auto const& jsonAlbumEntry : jsonAlbumEntries)
      {
        if(!jsonAlbumEntry.isMember(JsonKey::albumEntry) || !jsonAlbumEntry.isMember(JsonKey::lastSeenTime)) {
          PRINT_NAMED_WARNING("EnrolledFaceEntry.ConstructFromJson.BadAlbumEntries", "");
          _albumEntrySeenTimes.clear();
          break;
        }
        
        const AlbumEntryID_t entry = jsonAlbumEntry[JsonKey::albumEntry].asInt();
        if(entry != UnknownAlbumEntryID)
        {
          numTicks = jsonAlbumEntry[JsonKey::lastSeenTime].asLargestInt();
          _albumEntrySeenTimes[entry] = Time(std::chrono::seconds(numTicks));
        }
        
        // First entry in Json is the session-only entry, by convention
        if(!sessionOnlySet)
        {
          _sessionOnlyAlbumEntry = entry;
          sessionOnlySet = true;
        }
        
      }
    }
  }
}
  
EnrolledFaceEntry::EnrolledFaceEntry(const EnrolledFaceStorage& message)
: _faceID(message.faceID)
, _name(message.name)
, _enrollmentTime(std::chrono::seconds(message.enrollmentTimeCount))
, _lastDataUpdateTime(std::chrono::seconds(message.lastDataUpdateTimeCount))
{
  if(message.albumEntries.size() != message.albumEntryUpdateTimes.size())
  {
    PRINT_NAMED_WARNING("EnrolledFaceEntry.Constructor.FromEnrolledFaceStorage",
                        "Mismatched album entries and update times (%zu vs %zu)",
                        message.albumEntries.size(), message.albumEntryUpdateTimes.size());
  }
  else
  {
    for(s32 iEntry=0; iEntry < message.albumEntries.size(); ++iEntry)
    {
      const AlbumEntryID_t albumEntry = message.albumEntries[iEntry];
      
      // First entry is the session-only entry, by convention
      if(iEntry==0)
      {
        _sessionOnlyAlbumEntry = albumEntry;
      }
      
      if(albumEntry != UnknownAlbumEntryID)
      {
        const Time seenTime = Time(std::chrono::seconds( message.albumEntryUpdateTimes[iEntry] ));
        _albumEntrySeenTimes[albumEntry] = seenTime;
      }
    }
    
    ClipFutureTimesToNow("ConstructFromEnrolledFaceStorage");
    
    PRINT_NAMED_INFO("EnrolledFaceEntry.ConstructFromEnrolledFaceStorage.Times",
                     "EnrollmentTime:%s LastUpdateTime:%s",
                     GetTimeString(_enrollmentTime).c_str(),
                     GetTimeString(_lastDataUpdateTime).c_str());
  }
}
  
void EnrolledFaceEntry::FillJson(Json::Value& entry) const
{
  using namespace std::chrono;
  
  // NOTE: Save times as "number of ticks since epoch"
  entry[JsonKey::name]               = _name.asString();
  entry[JsonKey::enrollmentTime]     = TimeToInt64(_enrollmentTime);
  entry[JsonKey::lastDataUpdateTime] = TimeToInt64(_lastDataUpdateTime);
  
  // First entry is the session-only entry, by convention
  Json::Value jsonAlbumEntry;
  jsonAlbumEntry[JsonKey::albumEntry]   = _sessionOnlyAlbumEntry;
  jsonAlbumEntry[JsonKey::lastSeenTime] = (_sessionOnlyAlbumEntry != UnknownAlbumEntryID ?
                                    TimeToInt64(_albumEntrySeenTimes.at(_sessionOnlyAlbumEntry)) :
                                    0);
  
  entry["albumEntries"].append(jsonAlbumEntry);
  
  for(auto & albumEntryPair : _albumEntrySeenTimes)
  {
    // Don't re-add the session-only entry
    if(albumEntryPair.first != _sessionOnlyAlbumEntry)
    {
      jsonAlbumEntry[JsonKey::albumEntry] = albumEntryPair.first;
      jsonAlbumEntry[JsonKey::lastSeenTime] = TimeToInt64(albumEntryPair.second);
      entry[JsonKey::albumEntries].append(jsonAlbumEntry);
    }
  }
  
  // NOTE: Not storing faceID because we're assuming it's being used as the key for the entire entry
}
  

EnrolledFaceStorage EnrolledFaceEntry::ConvertToEnrolledFaceStorage() const
{
  EnrolledFaceStorage message;
  using namespace std::chrono;
  
  message.enrollmentTimeCount     = duration_cast<seconds>(_enrollmentTime.time_since_epoch()).count();
  message.lastDataUpdateTimeCount = duration_cast<seconds>(_lastDataUpdateTime.time_since_epoch()).count();
  message.faceID                  = _faceID;
  message.name                    = _name.asString(); // save formatted string
  
  message.albumEntries.reserve(_albumEntrySeenTimes.size());
  message.albumEntryUpdateTimes.reserve(_albumEntrySeenTimes.size());
  
  // First entry is the session-only entry, by convention
  message.albumEntries.push_back(_sessionOnlyAlbumEntry);
  message.albumEntryUpdateTimes.push_back(_sessionOnlyAlbumEntry != UnknownAlbumEntryID ?
                                          TimeToInt64(_albumEntrySeenTimes.at(_sessionOnlyAlbumEntry)) :
                                          0);
  
  for(auto & albumEntryPair : _albumEntrySeenTimes)
  {
    // Don't re-add the session-only entry
    if(albumEntryPair.first != _sessionOnlyAlbumEntry)
    {
      message.albumEntries.push_back( albumEntryPair.first );
      message.albumEntryUpdateTimes.push_back( TimeToInt64(albumEntryPair.second) );
    }
  }
  
  return message;
}
 
void EnrolledFaceEntry::UpdatePreviousIDs()
{
  _prevID = _faceID;
  _prevTrackID = _trackID;
}
  
void EnrolledFaceEntry::SetDebugMatchingInfo(std::list<FaceRecognitionMatch>&& newDebugInfo)
{
  std::swap(_debugMatchingInfo, newDebugInfo);
}
 
std::string EnrolledFaceEntry::GetAlbumEntriesString() const
{
  std::string str;
  
  if(_albumEntrySeenTimes.empty())
  {
    str = "<none>";
  }
  else
  {
    for(auto iter = _albumEntrySeenTimes.begin(); iter != _albumEntrySeenTimes.end(); ++iter)
    {
      str += (iter == _albumEntrySeenTimes.begin()) ? "[" : " ";
      
      const AlbumEntryID_t albumEntry = iter->first;
      str += std::to_string(albumEntry);
      if(albumEntry == _sessionOnlyAlbumEntry)
      {
        str += "S";
      }
    }
    str += "]";
  }
  
  return str;
}
  
void EnrolledFaceEntry::Serialize(std::vector<u8>& buffer) const
{
  // Create the message
  const EnrolledFaceStorage message(this->ConvertToEnrolledFaceStorage());
  
  // Pack it into a temp buffer
  const size_t expectedNumBytes = message.Size();
  buffer.resize(buffer.size() + expectedNumBytes);
  const size_t numBytes = message.Pack(&(buffer[buffer.size()-expectedNumBytes]), expectedNumBytes);
  
  DEV_ASSERT_MSG(numBytes == expectedNumBytes,
                 "EnrolledFaceEntry.Serialize.WrongPackedSize",
                 "Packed %zu, Expected %zu", numBytes, expectedNumBytes);
  
  PRINT_CH_DEBUG("FaceRecognizer", "EnrolledFaceEntry.Serialize.Success",
                 "Serialized entry for '%s', ID=%d. Added %zu bytes to buffer (total length now %zu)",
                 Anki::Util::HidePersonallyIdentifiableInfo(_name.c_str()), _faceID, numBytes, buffer.size());
  
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
  EnrolledFaceStorage message;
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
  
  // Now construct this object from the unpacked message and increment the startIndex for the next call
  *this = EnrolledFaceEntry(message);
  
  // Increment by the number of bytes we read
  startIndex += numBytes;
  
  return RESULT_OK;
} // Deserialize()

void EnrolledFaceEntry::AddOrUpdateAlbumEntry(AlbumEntryID_t entry, Time updateTime, bool isSessionOnly)
{
  if(isSessionOnly)
  {
    _sessionOnlyAlbumEntry = entry;
  }
  else if(entry == _sessionOnlyAlbumEntry)
  {
    // We're being told this entry is _not_ session only anymore, so unset it
    _sessionOnlyAlbumEntry = UnknownAlbumEntryID;
  }
  
  // If we're updating the entry, we must have seen it (?)
  _albumEntrySeenTimes[entry] = updateTime;
  
  // Update the whole object's updateTime as well
  _lastDataUpdateTime = updateTime;
}
  
  
void EnrolledFaceEntry::SetAlbumEntryLastSeenTime(AlbumEntryID_t entry, Time time)
{
  auto iter = _albumEntrySeenTimes.find(entry);

  if(iter == _albumEntrySeenTimes.end())
  {
    PRINT_NAMED_ERROR("EnrolledFaceEntry.SetAlbumEntryLastSeenTime.BadEntry",
                      "AlbumEntry %d not previously added", entry);
  }
  else
  {
    iter->second = time;
  }
}

EnrolledFaceEntry::Time EnrolledFaceEntry::FindLastSeenTime() const
{
  // Return most recent album entry seen time
  Time lastSeenTime(std::chrono::seconds(0));
  
  for(auto & albumEntryPair : _albumEntrySeenTimes)
  {
    if(albumEntryPair.second > lastSeenTime)
    {
      lastSeenTime = albumEntryPair.second;
    }
  }
  
  return lastSeenTime;
}
  
  
Result EnrolledFaceEntry::MergeWith(const EnrolledFaceEntry& other,
                                    s32 maxAlbumEntriesToKeep,
                                    std::vector<AlbumEntryID_t>& entriesRemoved)
{
  Result helperResult = MergeAlbumEntriesHelper(other, maxAlbumEntriesToKeep, entriesRemoved);
  if(RESULT_OK != helperResult) {
    return helperResult;
  }
  
  // Notify that merge ID was merged into keepID if it was not brand new
  if(other._prevID == other._faceID) {
    _prevID = other._faceID;
  }
  
  // Use keepID's name unless it doesn't have one
  if(_name.empty()) {
    _name = other._name;
  }
  
  // Keep most recent lastDataUpdateTime
  _lastDataUpdateTime = std::max(_lastDataUpdateTime, other._lastDataUpdateTime);
  
  // Keep oldest enrollment time
  _enrollmentTime = std::min(_enrollmentTime, other._enrollmentTime);
  
  if(ANKI_DEVELOPER_CODE)
  {
    // Sanity check: neither "this" nor "other" should have times in the future, so
    // merged times also shouldn't
    const bool clippingOccurred = ClipFutureTimesToNow("MergeWith");
    DEV_ASSERT(!clippingOccurred, "EnrolledFaceEntry.MergeWith.FutureTimesClipped");
  }
  
  // Keep highest score
  _score = std::max(_score, other._score);
  
  return RESULT_OK;
}

Result EnrolledFaceEntry::MergeAlbumEntriesHelper(const EnrolledFaceEntry& other,
                                                  s32 maxAlbumEntriesToKeep,
                                                  std::vector<AlbumEntryID_t>& entriesRemoved)
{
  const Time otherSessionOnlyTime = (other._sessionOnlyAlbumEntry != UnknownAlbumEntryID ?
                                     other._albumEntrySeenTimes.at(other._sessionOnlyAlbumEntry) :
                                     Time(std::chrono::seconds(0)));
  
  const Time thisSessionOnlyTime  = (_sessionOnlyAlbumEntry != UnknownAlbumEntryID ?
                                     _albumEntrySeenTimes.at(this->_sessionOnlyAlbumEntry) :
                                     Time(std::chrono::seconds(0)));
  
  const bool useOtherSessionOnlyEntry = (otherSessionOnlyTime > thisSessionOnlyTime);
  
  PRINT_CH_DEBUG("FaceRecognizer", "EnrolledFaceEntry.MergeAlbumEntriesHelper.WhichSessionOnlyEntry",
                 "Keeping %s (ID:%d) entry's session-only entry (%d)",
                 useOtherSessionOnlyEntry ? "other" : "this",
                 useOtherSessionOnlyEntry ? other._faceID : _faceID,
                 useOtherSessionOnlyEntry ? other._sessionOnlyAlbumEntry : _sessionOnlyAlbumEntry);
  
  // NOTE: the -1's below are for session-only entries
  
  // First merge all non-session
  const size_t thisSizeWithoutSessionOnly = _albumEntrySeenTimes.size() - (_sessionOnlyAlbumEntry != UnknownAlbumEntryID ? 1 : 0);
  const size_t otherSizeWithoutSessionOnly = other._albumEntrySeenTimes.size() - (other._sessionOnlyAlbumEntry != UnknownAlbumEntryID ? 1 : 0);
  
  if(thisSizeWithoutSessionOnly + otherSizeWithoutSessionOnly <= maxAlbumEntriesToKeep-1)
  {
    // Simple case: keep all entries
    
    PRINT_CH_DEBUG("FaceRecognizer", "EnrolledFaceEntry.MergeAlbumEntriesHelper.KeepingBoth",
                   "ThisEntry(ID:%d) has %zu entries, OtherEntry(ID:%d) has %zu",
                   _faceID, _albumEntrySeenTimes.size(), other._faceID, other._albumEntrySeenTimes.size());
    
    // First add all non-session-only entries from "other"
    for(auto & otherAlbumEntryPair : other._albumEntrySeenTimes)
    {
      if(otherAlbumEntryPair.first != other._sessionOnlyAlbumEntry) {
        _albumEntrySeenTimes[otherAlbumEntryPair.first] = otherAlbumEntryPair.second;
      }
    }
    
    // Handle session-only entry specially
    if(useOtherSessionOnlyEntry)
    {
      // First clear this session-only entry
      if(_sessionOnlyAlbumEntry != UnknownAlbumEntryID) {
        _albumEntrySeenTimes.erase(_sessionOnlyAlbumEntry);
        entriesRemoved.emplace_back(_sessionOnlyAlbumEntry);
      }
      
      // Add in other's session-only entry
      _sessionOnlyAlbumEntry = other._sessionOnlyAlbumEntry;
      _albumEntrySeenTimes[_sessionOnlyAlbumEntry] = otherSessionOnlyTime;
    }

  }
  else
  {
    using namespace std::chrono;
    
    // Keep the most recently seen entries, sorted from most recent to least recent
    // The bool here represents whether the entry came from "this" EnrolledFaceEntry,
    // so that we know (once sorted) if it needs to be added to the removed list we return.
    struct SortingPair
    {
      AlbumEntryID_t  albumEntry;
      bool          isFromThis;
    };
    std::multimap<Time, SortingPair, std::greater<Time>> allManualEntriesSortedByTime;
    for(auto & entry : _albumEntrySeenTimes)
    {
      if(entry.first != _sessionOnlyAlbumEntry)
      {
        allManualEntriesSortedByTime.emplace(entry.second, SortingPair{entry.first, true});
      }
    }
    for(auto & entry : other._albumEntrySeenTimes)
    {
      if(entry.first != other._sessionOnlyAlbumEntry)
      {
        allManualEntriesSortedByTime.emplace(entry.second, SortingPair{entry.first, false});
      }
    }
    
    // Start with an empty list, then add the first N-1 entries from the sorted
    // list of entries
    _albumEntrySeenTimes.clear();
    auto entriesByTimeIter = allManualEntriesSortedByTime.begin();
    while(_albumEntrySeenTimes.size() < (maxAlbumEntriesToKeep-1) && entriesByTimeIter != allManualEntriesSortedByTime.end())
    {
      // Add the entry to this
      PRINT_CH_DEBUG("FaceRecognizer", "EnrolledFaceEntry.MergeAlbumEntriesHelper.KeepingFromOne",
                     "Keeping albumEntry:%d from %s (ID:%d)",
                     entriesByTimeIter->second.albumEntry,
                     entriesByTimeIter->second.isFromThis ? "this" : "other",
                     entriesByTimeIter->second.isFromThis ? _faceID : other._faceID);
      
      _albumEntrySeenTimes.emplace(entriesByTimeIter->second.albumEntry, entriesByTimeIter->first);
      
      ++entriesByTimeIter;
    }
    
    // If there's anything left in the list from "this" EnrolledFaceEntry's albumEntries,
    // put those in the removed list
    while(entriesByTimeIter != allManualEntriesSortedByTime.end())
    {
      if(entriesByTimeIter->second.isFromThis) {
        entriesRemoved.emplace_back(entriesByTimeIter->second.albumEntry);
      }
      ++entriesByTimeIter;
    }
    
    // Handle session-only entry specially
    if(useOtherSessionOnlyEntry)
    {
      if(_sessionOnlyAlbumEntry != UnknownAlbumEntryID) {
        entriesRemoved.emplace_back(_sessionOnlyAlbumEntry);
      }
      _sessionOnlyAlbumEntry = other._sessionOnlyAlbumEntry;
      _albumEntrySeenTimes[_sessionOnlyAlbumEntry] = otherSessionOnlyTime;
    }
    else if(_sessionOnlyAlbumEntry != UnknownAlbumEntryID)
    {
      // Leave session only entry as it is, but re-insert its time because we cleared above
      _albumEntrySeenTimes[_sessionOnlyAlbumEntry] = thisSessionOnlyTime;
    }
  }
  
  // Sanity checks
  if(ANKI_DEVELOPER_CODE)
  {
    if(_albumEntrySeenTimes.size() > maxAlbumEntriesToKeep)
    {
      PRINT_NAMED_WARNING("EnrolledFaceEntry.MergeWithEntriesHelper.Failure",
                          "Got %zu > %d album entries",
                          _albumEntrySeenTimes.size(), maxAlbumEntriesToKeep);
      return RESULT_FAIL;
    }
    
    for(auto albumEntry : entriesRemoved)
    {
      DEV_ASSERT_MSG(_albumEntrySeenTimes.find(albumEntry) == _albumEntrySeenTimes.end(),
                     "EnrolledFaceEntry.MergeWithEntriesHelper.EntryKeptAndInRemovedList",
                     "AlbumEntry:%d", albumEntry);
    }
  }
  
  return RESULT_OK;
}
  
std::string EnrolledFaceEntry::GetTimeString(Time time)
{
  std::string str;
  char buffer[256];
  auto t = std::chrono::system_clock::to_time_t(time);
  strftime(buffer, 255, "%F %T", localtime(&t));
  str = buffer;
  return str;
}
 
bool EnrolledFaceEntry::ClipFutureTimesToNow(const std::string& logString)
{
  bool didClip = false;
  
  // If incoming times are somehow in the future, clip them to now
  const Time currentTime = std::chrono::system_clock::now();
  if(_enrollmentTime > currentTime)
  {
    PRINT_NAMED_WARNING(("EnrolledFaceEntry." + logString + ".FutureEnrollmentTime").c_str(),
                        "Clipping future enrollment time (%s) to now (%s)",
                        GetTimeString(_enrollmentTime).c_str(), GetTimeString(currentTime).c_str());
    
    _enrollmentTime = currentTime;
    didClip = true;
  }
  if(_lastDataUpdateTime > currentTime)
  {
    PRINT_NAMED_WARNING(("EnrolledFaceEntry." + logString + ".FutureLastUpdatedTime").c_str(),
                        "Clipping future last updated time (%s) to now (%s)",
                        GetTimeString(_lastDataUpdateTime).c_str(), GetTimeString(currentTime).c_str());
    
    _lastDataUpdateTime = currentTime;
    didClip = true;
  }
  
  return didClip;
}
  
} // namespace Vision
} // namespace Anki


