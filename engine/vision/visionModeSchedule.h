/**
 * File: visionModeSchedule.h
 *
 * Author: Andrew Stein
 * Date:   10-28-2016
 *
 * Description: Container for keeping up with whether it is time to do a particular
 *              type of vision processing.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Anki_Cozmo_Basestation_VisionModeSchedule_H__
#define __Anki_Cozmo_Basestation_VisionModeSchedule_H__

#include "clad/types/visionModes.h"

#include <array>
#include <list>
#include <vector>

namespace Anki {
namespace Cozmo {

class VisionModeSchedule
{
public:
  VisionModeSchedule(); // Default: always scheduled to run
  explicit VisionModeSchedule(std::vector<bool>&& initSchedule);
  explicit VisionModeSchedule(bool alwaysOnOrOff);
  explicit VisionModeSchedule(int onFrequency, int frameOffset = 0);
  
  // Checks schedule and advances for next query (so should be called once per image/tick)
  bool CheckTimeToProcessAndAdvance();
  
private:
  std::vector<bool> _schedule;
  int               _index = 0;
  
}; // class VisionModeSchedule
  
  
class AllVisionModesSchedule
{
public:
  using ModeScheduleList = std::list<std::pair<VisionMode, VisionModeSchedule>>;

  // If initWithDefaults=true, all modes' schedules are set to current defaults.
  // Otherwise, everything starts disabled.
  AllVisionModesSchedule(bool initWithDefaults = true);
  
  // Initialize specified modes with given schedules, and initialize any unspecified
  // modes' schedules to the current defaults. (If useDefaultsForUnspecified=false,
  // any unspecified modes will be disabled.)
  //
  // Example for setting one mode:
  //   AllVisionModesSchedule({{VisionMode::DetectingMarkers, VisionModeSchedule(1)}})
  //
  // Example for setting two modes:
  //   AllVisionModesSchedule({{VisionMode::DetectingPets,          VisionModeSchedule({true, false})},
  //                           {VisionMode::DetectingOverheadEdges, VisionModeSchedule({false, true})}});
  //
  AllVisionModesSchedule(const ModeScheduleList& schedules,
                         bool useDefaultsForUnspecified = true);
  
  // Get the schedule for a specific mode
  VisionModeSchedule& GetScheduleForMode(VisionMode mode);
  const VisionModeSchedule& GetScheduleForMode(VisionMode mode) const;
  
  // Returns whether it is time to process a mode and advances that mode for the next query
  // (So should only be called once per image/tick, for each mode)
  bool CheckTimeToProcessAndAdvance(VisionMode mode);
  
  // Change the defaults to use for unspecified modes
  static void SetDefaultSchedule(VisionMode mode, VisionModeSchedule&& schedule);
  
private:
  
  using ScheduleArray = std::array<VisionModeSchedule, (size_t)VisionMode::Count>;
  
  ScheduleArray _schedules;
  
  static ScheduleArray sDefaultSchedules;
  static ScheduleArray InitDefaultSchedules();

}; // class AllVisionModesSchedule
  

} // namespace Cozmo
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_VisionModeSchedule_H__ */
