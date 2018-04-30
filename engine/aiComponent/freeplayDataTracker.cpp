/**
 * File: freeplayDataTracker.cpp
 *
 * Author: Brad Neuman
 * Created: 2017-08-03
 *
 * Description: Component to track things that happen in freeplay and send DAS events
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/freeplayDataTracker.h"

#include "util/logging/logging.h"
#include "util/math/math.h"

#include <cmath>
#include <sstream>

namespace {
static constexpr const float kUpdatePeriod_s = 30.0f;
}

namespace Anki {
namespace Cozmo {


const char* FreeplayDataTracker::FreeplayPauseFlagToString(FreeplayPauseFlag flag)
{

#define HANDLE_CASE(v) case FreeplayPauseFlag::v: return #v

  switch(flag) {
    HANDLE_CASE(GameControl);
    HANDLE_CASE(Spark);
    HANDLE_CASE(OffTreads);
    HANDLE_CASE(OnCharger);
  }
}

FreeplayDataTracker::FreeplayDataTracker()
: IDependencyManagedComponent<AIComponentID>(this, AIComponentID::FreeplayDataTracker)
{
  _timeToSendData_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kUpdatePeriod_s;
}

void FreeplayDataTracker::InitDependent(Cozmo::Robot* robot, const AICompMap& dependentComps)
{
  // Toggle flag to "start" the tracking process - legacy assumption that freeplay is not
  // active on app start - full fix requires a deeper update to the data tracking system
  // that is outside of scope for this PR but should be addressed in VIC-626
  SetFreeplayPauseFlag(true, FreeplayPauseFlag::OffTreads);
  SetFreeplayPauseFlag(false, FreeplayPauseFlag::OffTreads);
}

void FreeplayDataTracker::UpdateDependent(const AICompMap& dependentComps)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if( currTime_s >= _timeToSendData_s ) {
    SendData();
  }
}

void FreeplayDataTracker::ForceUpdate()
{
  SendData();
}

void FreeplayDataTracker::SetFreeplayPauseFlag(bool val, FreeplayPauseFlag flag)
{
  if( val ) {
    SetFreeplayPauseFlag(flag);
  }
  else {
    ClearFreeplayPauseFlag(flag);
  }
}


void FreeplayDataTracker::SetFreeplayPauseFlag(FreeplayPauseFlag flag)
{
  const BaseStationTime_t currTime_ns = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();

  const bool wasActive = IsActive();

  if( wasActive ) {
    // we are currently in active freeplay, but are about to be paused. accumulate previous active time (if any)
    if( _countFreeplayFromTime_ns > 0 ) {
      _timeData_ns += (currTime_ns - _countFreeplayFromTime_ns);
    }
  }

  _pausedFlags.insert(flag);

  if( wasActive ) {
    // Print debug info here after modifying paused flags (so the info is up to date)
    PRINT_CH_DEBUG("Behaviors",
                   "FreeplayDataTracker.Pause",
                   "Pausing at time %llu, currently have %llu accumulated. State: %s",
                   currTime_ns,
                   _timeData_ns,
                   GetDebugStateStr().c_str());
  }
}

void FreeplayDataTracker::ClearFreeplayPauseFlag(FreeplayPauseFlag flag)
{
  if( IsActive() ) {
    PRINT_CH_DEBUG("Behaviors", "FreeplayDataTracker.Resume.NotPaused",
                   "Trying to resume for flag %s, but not paused for any flag",
                   FreeplayPauseFlagToString(flag));
    // return early so we don't update time
    return;
  }

  const size_t num_removed = _pausedFlags.erase(flag);

  if( num_removed == 0 ) {
    PRINT_CH_DEBUG("Behaviors", "FreeplayDataTracker.Resume.NotPaused",
                   "Trying to resume for flag %s, but not paused for that flag. State: %s",
                   FreeplayPauseFlagToString(flag),
                   GetDebugStateStr().c_str());
  }

  // if we removed the last paused flag, we are now active
  const bool freeplayResumed = IsActive();

  if( freeplayResumed ) {
    const BaseStationTime_t currTime_ns = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();
    _countFreeplayFromTime_ns = currTime_ns;

    PRINT_CH_DEBUG("Behaviors", "FreeplayDataTracker.Resume",
                   "Resuming at time %llu, currently have %llu accumulated. State: %s",
                   currTime_ns,
                   _timeData_ns,
                   GetDebugStateStr().c_str());
  }
}

void FreeplayDataTracker::SendData()
{
  const BaseStationTime_t currTime_ns = BaseStationTimer::getInstance()->GetCurrentTimeInNanoSeconds();

  // if we are currently active, then accumulate time now
  if( IsActive() && _countFreeplayFromTime_ns > 0 ) {

    // freeplay starts off disabled, so starting time should always be more than 0 nanoseconds
    //DEV_ASSERT(_countFreeplayFromTime_ns > 0, "FreeplayDataTracker.FreeplayStartedAtZero");

    _timeData_ns += (currTime_ns - _countFreeplayFromTime_ns);
    PRINT_CH_DEBUG("Behaviors",
                   "FreeplayDataTracker.SendData.Accumulate",
                   "Sending at time %llu, currently have %llu accumulated. State: %s",
                   currTime_ns,
                   _timeData_ns,
                   GetDebugStateStr().c_str());
  }

  // send data if there is any
  if( _timeData_ns > 0 ) {
    const int freeplayTime_s = std::round( Util::NanoSecToSec( _timeData_ns ) );

    static const float kFudgeFactor = 1.2f;
    if( freeplayTime_s > kUpdatePeriod_s * kFudgeFactor ) {
      // This shouldn't happen. This means we somehow didn't set the data, or have a bad value in the time
      // data
      PRINT_NAMED_ERROR("FreeplayDataTracker.SendData.DataTooHigh",
                        "Trying to send a freeplay time of %d sec (%llu nanos), but update period is %f",
                        freeplayTime_s,
                        _timeData_ns,
                        kUpdatePeriod_s);
      // don't send the data in this case
    }
    else {
      // data looks good, send it up

      // send data with integer seconds. Note that this may round to 0, but send it anyway (so we know some
      // freeplay occurred, but less than 0.5 seconds worth)
      Util::sInfoF("robot.active_freeplay_time",
                   {},
                   "%d",
                   freeplayTime_s);
    }
  }

  // reset timers
  _timeData_ns = 0;
  if( IsActive() ) {
    _countFreeplayFromTime_ns = currTime_ns;
  }

  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  _timeToSendData_s = currTime_s + kUpdatePeriod_s;
}

bool FreeplayDataTracker::IsActive() const
{
  return _pausedFlags.empty();
}

std::string FreeplayDataTracker::GetDebugStateStr() const
{
  if( IsActive() ) {
    return "active";
  }

  std::stringstream ss;
  ss << "paused( ";

  for( auto flag : _pausedFlags ) {
    ss << FreeplayPauseFlagToString(flag) << ' ';
  }

  ss << ')';

  return ss.str();
}

}
}
