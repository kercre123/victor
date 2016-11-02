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


#include "anki/cozmo/basestation/visionModeSchedule.h"

namespace Anki {
namespace Cozmo {

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
VisionModeSchedule::VisionModeSchedule(int onFrequency)
: VisionModeSchedule(std::vector<bool>(onFrequency,false))
{
  if(onFrequency == 0)
  {
    // Special case
    _schedule.push_back(false);
  }
  else
  {
    _schedule[0] = true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool VisionModeSchedule::CheckTimeToProcessAndAdvance()
{
  const bool isTimeToProcess = _schedule[_index++];
  
  if(_index == _schedule.size())
  {
    _index = 0;
  }
  
  return isTimeToProcess;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Static member initialization
AllVisionModesSchedule::ScheduleArray AllVisionModesSchedule::sDefaultSchedules = AllVisionModesSchedule::InitDefaultSchedules();

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AllVisionModesSchedule::AllVisionModesSchedule()
: _schedules(sDefaultSchedules)
{
  
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AllVisionModesSchedule::AllVisionModesSchedule(const std::list<std::pair<VisionMode,VisionModeSchedule>>& schedules)
: AllVisionModesSchedule()
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
  std::fill(defaultInitialArray.begin(), defaultInitialArray.end(), VisionModeSchedule(true));
  return defaultInitialArray;
}
  
VisionModeSchedule& AllVisionModesSchedule::GetScheduleForMode(VisionMode mode)
{
  return _schedules[(size_t)mode];
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AllVisionModesSchedule::SetDefaultSchedule(VisionMode mode, VisionModeSchedule&& schedule)
{
  sDefaultSchedules[(size_t)mode] = std::move(schedule);
}
  
  

} // namespace Cozmo
} // namespace Anki
