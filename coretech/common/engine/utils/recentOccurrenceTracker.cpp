/**
 * File: recentOccurrenceTracker.cpp
 *
 * Author: Brad Neuman
 * Created: 2018-06-16
 *
 * Description: Simple tool to track arbitrary occurrences to see if they happen X times within Y seconds
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#include "coretech/common/engine/utils/recentOccurrenceTracker.h"

#include "coretech/common/engine/utils/timer.h"
#include "util/logging/logging.h"

#include "json/json.h"

#include <limits>

namespace Anki {

namespace {
static const char* kRecentOccurrenceKey = "recentOccurrence";
static const char* kNumberOfTimesKey = "numberOfTimes";
static const char* kAmountOfSecondsKey = "amountOfSeconds";
}

RecentOccurrenceHandle::RecentOccurrenceHandle(RecentOccurrenceTracker* tracker,
                                             int numberOfTimes,
                                             float amountOfSeconds)
  : _requiredNumber(numberOfTimes)
  , _requiredTime_s(amountOfSeconds)
  , _tracker(tracker)
{
}

RecentOccurrenceHandle::~RecentOccurrenceHandle()
{
  // since this tracker handle is gone, the tracker may not need to carry as many events anymore, so let it
  // recompute it's limits
  if( _tracker != nullptr ) {
    _tracker->RecomputeLimits();
  }
}

bool RecentOccurrenceHandle::AreConditionsMet() const
{
  if( ANKI_VERIFY(_tracker != nullptr,
                  "RecentOccurrenceHandle.AreConditionsMet.NullTracker",
                  "") ) {
    const bool met = _tracker->Check(_requiredNumber, _requiredTime_s);
    return met;
  }
  return false;
}

void RecentOccurrenceHandle::TrackerDestructing()
{
  _tracker = nullptr;
}

RecentOccurrenceTracker::RecentOccurrenceTracker(const std::string& debugName)
  : _debugName(debugName)
{
  _timeProvider = [](){ return BaseStationTimer::getInstance()->GetCurrentTimeInSeconds(); };
}

RecentOccurrenceTracker::~RecentOccurrenceTracker()
{
  // if there are any handles out there, let them know that the tracker is going away
  int numHandles = 0;
  for( auto& handleWeak : _handles ) {
    if( auto handle = handleWeak.lock() ) {
      handle->TrackerDestructing();
      numHandles++;
    }
  }

  if( numHandles > 0 ) {
    PRINT_NAMED_WARNING("RecentOccurrenceTracker.Destructor.WithHandles",
                        "Tracker '%s' destructing but still has %d active handles",
                        _debugName.c_str(),
                        numHandles);
  }
}


void RecentOccurrenceTracker::AddOccurrence()
{
  const float currTime_s = _timeProvider();

  _occurrences.push_back(currTime_s);
  Prune(currTime_s);

  if( _occurrences.empty() ) {
    // pruned to nothing, which probably means there's no handles
    PRINT_NAMED_WARNING("RecentOccurrenceTracker.AddOccurrence.NotTracked",
                        "Tracker '%s' added an occurrence, but pruned it away. Has %zu handles (some may be null)",
                        _debugName.c_str(),
                        _handles.size());
  }
}

void RecentOccurrenceTracker::Reset()
{
  _occurrences.clear();
}

RecentOccurrenceTracker::Handle RecentOccurrenceTracker::GetHandle(int numberOfTimes, float amountOfSeconds)
{
  std::shared_ptr<RecentOccurrenceHandle> handle(new RecentOccurrenceHandle(this, numberOfTimes, amountOfSeconds));
  if( numberOfTimes > _maxNumEvents ) {
    _maxNumEvents = numberOfTimes;
  }
  if( amountOfSeconds > _maxAgeEvents ) {
    _maxAgeEvents = amountOfSeconds;
  }

  _handles.push_back(handle);

  return handle;
}

bool RecentOccurrenceTracker::ParseConfig(const Json::Value& config, int& numberOfTimes, float& amountOfSeconds)
{
  if( config[kRecentOccurrenceKey].isObject() ) {

    // default if nothing is specified is to return true if this has happened at all (ever)
    // if only a number or only a time is specified, that should work as well
    numberOfTimes = config[kRecentOccurrenceKey].get(kNumberOfTimesKey, 1).asInt();
    amountOfSeconds = config[kRecentOccurrenceKey].get(kAmountOfSecondsKey,
                                                       std::numeric_limits<float>::max()).asFloat();

    return true;
  }

  return false;
}

void RecentOccurrenceTracker::GetConfigJsonKeys(std::set<const char*>& expectedKeys)
{
  expectedKeys.insert( kRecentOccurrenceKey );
}

bool RecentOccurrenceTracker::Check(int numberOfTimes, float amountOfSeconds) const
{
  const float currTime_s = _timeProvider();

  if( _occurrences.empty() ) {
    // never happened at all, so only true if we didn't want it to happen
    return (numberOfTimes <= 0);
  }

  if( _occurrences.size() < numberOfTimes ) {
    // not enough events have happened (at all)
    return false;
  }

  const float firstTimeValid = currTime_s - amountOfSeconds;

  // count how many have happened since firstTimeValid

  // note: could use a binary search here, or cut off the search early and compare to size, but this is much simpler
  int num = 0;
  for( float time : _occurrences ) {
    num += time >= firstTimeValid ? 1 : 0;
  }

  return (num >= numberOfTimes);
}

void RecentOccurrenceTracker::Prune(float currTime_s)
{
  // first prune on number (don't need to store more than _maxNumEvents)
  while( _occurrences.size() > _maxNumEvents && !_occurrences.empty() ) {
    _occurrences.pop_front();
  }

  // now remove anything we're storing that's older than the max time we care about
  const float oldestToSave = currTime_s - _maxAgeEvents;
  while( !_occurrences.empty() && _occurrences.front() < oldestToSave ) {
    _occurrences.pop_front();
  }
}

void RecentOccurrenceTracker::RecomputeLimits()
{
  _maxNumEvents = 0;
  _maxAgeEvents = 0.0f;

  auto handleIter = _handles.begin();
  while( handleIter != _handles.end() ) {
    if( auto handle = handleIter->lock() ) {
      if( handle->_requiredNumber > _maxNumEvents ) {
        _maxNumEvents = handle->_requiredNumber;
      }
      if( handle->_requiredTime_s > _maxAgeEvents ) {
        _maxAgeEvents = handle->_requiredTime_s;
      }

      handleIter++;
    }
    else {
      // weak pointer lock failed, so handle must be deleted
      handleIter = _handles.erase(handleIter);
    }
  }
}

}
