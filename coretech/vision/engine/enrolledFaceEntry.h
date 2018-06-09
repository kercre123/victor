/**
 * File: enrolledFaceEntry.h
 *
 * Author: Andrew Stein
 * Date:   2/18/2016
 *
 * Description: Container for holding bookkeeping data for faces enrolled for face recognition.
 *              This class is used heavily by the FaceRecognizer and makes use of the
 *              EnrolledFaceStorage CLAD struct to serialize/deserialize for storing
 *              in the robot's NVStorage.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Anki_Vision_EnrolledFaceEntry_H__
#define __Anki_Vision_EnrolledFaceEntry_H__

#include "coretech/vision/engine/faceIdTypes.h"

#include "clad/types/enrolledFaceStorage.h"

#include "util/global/globalDefinitions.h"

#include <chrono>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>

// Forward declaration
namespace Json {
  class Value;
}

namespace Anki {
namespace Vision {

// Literally a data type that represents the name of a user (which is generally mapped with a face)
// Giving it a concrete data type to abstract away the comparison of phonetically similar names
// * essentially an std::string wrapper that handles comparison for us *
class UserName
{
public:
  UserName( const std::string& name );
  UserName() { } // empty string

  // allowing default copy/move constructor/assignment functions

  UserName& operator=( const std::string& name );

  bool operator==( const UserName& rhs ) const;
  bool operator==( const std::string& rhs ) const;
  bool operator==( const char* rhs ) const;

  // allow implicit cast to std::string
  operator std::string() const { return _name; }

  // simulate a minimal set of std::string functions since that's how it's expected to be used
  bool                empty() const { return _name.empty(); }
  const char*         c_str() const { return _name.c_str(); }
  const std::string&  asString() const { return _name; }

  // use this for any debugging or clad messages where we want to obfuscate the names of the users
  const char*         piiGuardedString() const { return Util::HidePersonallyIdentifiableInfo( _name.c_str() ); }


private:
  std::string _name;
};

class EnrolledFaceEntry
{
public:
  
  // Aliases for better readability
  using AlbumEntryID_t   = s32;
  using TrackingID_t     = s32;
  using RecognitionScore = s32;
  using Time = std::chrono::time_point<std::chrono::system_clock>;
  
  static const AlbumEntryID_t UnknownAlbumEntryID = -1;
  
  // ---------------------------------------------------------------------------
  // Construction
  //
  
  EnrolledFaceEntry();
  
  EnrolledFaceEntry(FaceID_t withID, Time enrollmentTime);
  
  // Faces constructed from Json default to _not_ being for this session only
  EnrolledFaceEntry(FaceID_t withID, Json::Value& json);
  
  // To/from from an EnrolledFaceStorage
  explicit EnrolledFaceEntry(const EnrolledFaceStorage& message);
  EnrolledFaceStorage ConvertToEnrolledFaceStorage() const;
  
  void FillJson(Json::Value& json) const;
  
  
  // ---------------------------------------------------------------------------
  // Getters and setters
  //
  
  FaceID_t GetFaceID() const { return _faceID; }
  FaceID_t GetPreviousFaceID() const { return _prevID; }
  
  TrackingID_t   GetTrackingID() const { return _trackID; }
  TrackingID_t   GetPreviousTrackingID() const { return _prevTrackID; }
  
  const UserName& GetName() const { return _name; }
  
  RecognitionScore  GetScore() const { return _score; }
  
  void SetTrackingID(TrackingID_t newID) { _prevTrackID = _trackID; _trackID = newID; }
  
  void ClearTrackingID() { _prevTrackID = UnknownFaceID; _trackID = UnknownFaceID; }
  
  void SetName(const std::string& newName) { _name = newName; }
  
  void SetScore(RecognitionScore newScore) { _score = newScore; }
  
  // Returns a map of album entries fo last seen times
  const std::map<AlbumEntryID_t, Time>& GetAlbumEntries() const { return _albumEntrySeenTimes; }
  
  void AddOrUpdateAlbumEntry(AlbumEntryID_t entry, Time updateTime, bool isSessionOnly);
  
  void RemoveAlbumEntry(AlbumEntryID_t entry) { _albumEntrySeenTimes.erase(entry); }
  
  void ClearAlbumEntries() { _albumEntrySeenTimes.clear(); _sessionOnlyAlbumEntry = UnknownAlbumEntryID; }
  
  void SetAlbumEntryLastSeenTime(AlbumEntryID_t entry, Time time);
  
  AlbumEntryID_t  GetSessionOnlyAlbumEntry() const { return _sessionOnlyAlbumEntry; }
  
  // Get when this enrolled face was added to our album
  Time GetEnrollmentTime() const { return _enrollmentTime; }
  
  const std::list<FaceRecognitionMatch>& GetDebugMatchingInfo() const { return _debugMatchingInfo; }
  void SetDebugMatchingInfo(std::list<FaceRecognitionMatch>&& newDebugInfo);
  
  // Return a string containing the album entries, with session-only marked with "S"
  std::string GetAlbumEntriesString() const;
  
  // ---------------------------------------------------------------------------
  // Basic methods
  //
  
  bool WasFaceIDJustUpdated() const { return _faceID != _prevID; }
  
  // Update previous face ID and tracking ID to match current, so this entry
  // no longer comes up as "new"
  void UpdatePreviousIDs();
  
  // Find most recent time _any_ associated album entry was seen or updated
  // NOTE: called "Find" because this is not a "free" getter. It iterates over
  //       all album entries, so store result if used repeatedly.
  Time FindLastSeenTime() const;
  
  // Get most recent time this any of this entry's album data was updated
  Time GetLastUpdateTime() const { return _lastDataUpdateTime; }
  
  bool IsForThisSessionOnly() const { return _name.empty(); }
  
  // Update with bookkeeping from "other" EnrolledFaceEntry.
  // NOTE: maxAlbumEntriesToKeep includes the session-only entry!
  Result MergeWith(const EnrolledFaceEntry& other, s32 maxAlbumEntriesToKeep,
                   std::vector<s32>& entriesRemoved);
  
  static std::string GetTimeString(Time time);
  
  // ---------------------------------------------------------------------------
  // Serialization
  //
  
  // Does not clear input vector, just adds to it
  void Serialize(std::vector<u8>& buffer) const;
  
  // Modifies startIndex to point to the next entry in the buffer.
  // When startIndex == buffer.size(), everything has been read from the buffer.
  // Returns RESULT_OK if serialization succeeds.
  Result Deserialize(const std::vector<u8>& buffer, size_t& startIndex);
  
  
protected:
  
  Result MergeAlbumEntriesHelper(const EnrolledFaceEntry& other, s32 maxAlbumEntriesToKeep,
                                 std::vector<AlbumEntryID_t>& entriesRemoved);
  
  // Helper to make sure times aren't in the future when loaded/initialized
  bool ClipFutureTimesToNow(const std::string& logString);
  
  FaceID_t            _faceID = UnknownFaceID; // The ID used for recognition
  FaceID_t            _prevID = UnknownFaceID; // The previous ID if the ID just changed (e.g. due to merge)
  
  TrackingID_t        _trackID     = UnknownFaceID;  // The last associated tracker ID
  TrackingID_t        _prevTrackID = UnknownFaceID;  // The previous track ID in case it changed
  
  UserName            _name;
  RecognitionScore    _score = 1000;  // [0,1000]
  
  AlbumEntryID_t      _sessionOnlyAlbumEntry;
  Time                _enrollmentTime;         // When this enrolled face was added to our album
  Time                _lastDataUpdateTime;     // Last time we updated any album entry associated with this face

  // Stores album entries (effectively as a sorted, unique set) and the last time
  // we saw (i.e. matched to) that specific album entry.
  std::map<AlbumEntryID_t, Time> _albumEntrySeenTimes;
  
  std::list<FaceRecognitionMatch> _debugMatchingInfo;
  
}; // class EnrolledFaceEntry
  
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_EnrolledFaceEntry_H__


