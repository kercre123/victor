/**
 * File: recentOccurrenceTracker.h
 *
 * Author: Brad Neuman
 * Created: 2018-06-16
 *
 * Description: Simple tool to track arbitrary occurrences to see if they happen X times within Y seconds
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Coretech_Common_Engine_Utils_RecentOccurrenceTracker_H__
#define __Coretech_Common_Engine_Utils_RecentOccurrenceTracker_H__

#include "json/json-forwards.h"

#include <deque>
#include <set>
#include <vector>

namespace Anki {

class RecentOccurrenceTracker;

class RecentOccurrenceHandle
{
public:

  ~RecentOccurrenceHandle();

  // checks if conditions are met now (using basestation time)
  bool AreConditionsMet() const;

private:
  friend class RecentOccurrenceTracker;
  RecentOccurrenceHandle() = delete;
  RecentOccurrenceHandle(RecentOccurrenceTracker* tracker, int numberOfTimes, float amountOfSeconds);

  void TrackerDestructing();

  int _requiredNumber;
  float _requiredTime_s;

  // store a raw pointer to the tracker that will get reset when the tracker is destructed (it will call
  // TrackerDestructing() on all of it's handles)
  RecentOccurrenceTracker* _tracker;
};

class RecentOccurrenceTracker
{
  friend class RecentOccurrenceHandle;
public:

  using Handle = std::shared_ptr<RecentOccurrenceHandle>;

  explicit RecentOccurrenceTracker(const std::string& debugName);

  // don't allow copies (tracker handles aren't equipped to have multiple owners)
  RecentOccurrenceTracker(const RecentOccurrenceTracker& other) = delete;
  RecentOccurrenceTracker& operator=(const RecentOccurrenceTracker& other) = delete;
  
  ~RecentOccurrenceTracker();

  // parse the json config (including handling defaults) into numberOfTimes and amountOfSeconds and return
  // true if successfully parsed, return false otherwise
  static bool ParseConfig(const Json::Value& config, int& numberOfTimes, float& amountOfSeconds);

  // to verify json keys that are used, this function can be called to populate keys used by ParseConfig
  static void GetConfigJsonKeys(std::set<const char*>& expectedKeys);

  // set a time provider, which is a function that should return monotonically increasing values in
  // seconds. This is not meant to be a hyper-precise timer (hence using floats). The default is to use
  // BaseStationTimer internally
  using TimeProvider = std::function<float()>;
  void SetTimeProvider(TimeProvider provider) { _timeProvider = provider; }

  // Call this function to add an occurrence "now" (using the provided timer, or BaseStationTimer by default)
  void AddOccurrence();

  // Reset occurrences (start counting again at zero)
  void Reset();
  
  // Get a handle that will evaluate true when this condition has occurred numberOfTimes in
  // amountOfSeconds. Note that unless someone is holding a handle, events will not be tracked. When handles
  // are held, events will only be tracked as long (and as many) as neccesary
  Handle GetHandle(int numberOfTimes, float amountOfSeconds);

  // the number of events currently tracked
  size_t GetCurrentSize() const { return _occurrences.size(); }
  
private:

  std::deque<float> _occurrences;
  size_t _maxNumEvents = 0;
  float _maxAgeEvents = 0.0f;
  std::string _debugName;

  TimeProvider _timeProvider;
  
  std::vector<std::weak_ptr<RecentOccurrenceHandle>> _handles;

  // to be called by the handle, only valid if we're storing enough data for the check to work
  bool Check(int numberOfTimes, float amountOfSeconds) const;

  void Prune(float currTime_s);

  void RecomputeLimits();
};


}

#endif
