/**
 * File: visionModeSchedule.cpp
 *
 * Author: Andrew Stein
 * Date:   10-28-2016
 *
 * Description: Container for keeping up with whether it is time to do a particular
 *              type of vision processing.
 *
 * Copyright: Anki, Inc. 2016
 **/


#include "engine/vision/visionModeSchedule.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Vector {

VisionModeSchedule::VisionModeSchedule()
: VisionModeSchedule(true)
{

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VisionModeSchedule::VisionModeSchedule(std::vector<bool>&& initSchedule)
: _schedule(std::move(initSchedule))
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VisionModeSchedule::VisionModeSchedule(bool alwaysOnOrOff)
: VisionModeSchedule(std::vector<bool>{alwaysOnOrOff})
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VisionModeSchedule::VisionModeSchedule(int onFrequency, int frameOffset)
: VisionModeSchedule(std::vector<bool>(onFrequency,false))
{
  DEV_ASSERT((frameOffset < onFrequency), "VisionModeSchedule.ReceivedOutOfBoundsFrameOffset");

  if(onFrequency == 0)
  {
    // Special case
    _schedule.push_back(false);
  }
  else
  {
    _schedule[frameOffset] = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionModeSchedule::IsTimeToProcess(u32 index) const
{
  const bool isTimeToProcess = _schedule[index % _schedule.size()];
  return isTimeToProcess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionModeSchedule::WillEverRun() const
{
  for(bool isScheduled : _schedule)
  {
    if(isScheduled) {
      return true;
    }
  }
  
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Static member initialization
AllVisionModesSchedule::ScheduleArray AllVisionModesSchedule::sDefaultSchedules = AllVisionModesSchedule::InitDefaultSchedules();

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AllVisionModesSchedule::AllVisionModesSchedule(bool useDefaults)
{
  if(useDefaults) {
    _schedules = sDefaultSchedules;
  } else {
    _schedules.fill(VisionModeSchedule(false));
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AllVisionModesSchedule::AllVisionModesSchedule(const std::list<std::pair<VisionMode,VisionModeSchedule>>& schedules,
                                               bool useDefaultsForUnspecified)
: AllVisionModesSchedule(useDefaultsForUnspecified)
{
  for(auto schedIter = schedules.begin(); schedIter != schedules.end(); ++schedIter)
  {
    _schedules[(size_t)schedIter->first] = schedIter->second;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AllVisionModesSchedule::ScheduleArray AllVisionModesSchedule::InitDefaultSchedules()
{
  ScheduleArray defaultInitialArray;
  std::fill(defaultInitialArray.begin(), defaultInitialArray.end(), VisionModeSchedule(false));
  return defaultInitialArray;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
VisionModeSchedule& AllVisionModesSchedule::GetScheduleForMode(VisionMode mode)
{
  return _schedules[(size_t)mode];
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const VisionModeSchedule& AllVisionModesSchedule::GetScheduleForMode(VisionMode mode) const
{
  return _schedules[(size_t)mode];
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AllVisionModesSchedule::IsTimeToProcess(VisionMode mode, u32 index) const
{
  const bool isTimeToProcess = GetScheduleForMode(mode).IsTimeToProcess(index);
  return isTimeToProcess;
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AllVisionModesSchedule::SetDefaultSchedule(VisionMode mode, VisionModeSchedule&& schedule)
{
  sDefaultSchedules[(size_t)mode] = std::move(schedule);
}
  
  

} // namespace Vector
} // namespace Anki
