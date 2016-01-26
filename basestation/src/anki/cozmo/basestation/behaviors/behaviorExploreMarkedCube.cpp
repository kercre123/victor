/**
 * File: behaviorExploreMarkedCube
 *
 * Author: Raul
 * Created: 01/22/16
 *
 * Description: Behavior that looks for a nearby marked cube that Cozmo has not fully explored (ie: seen in
 * all directions), and tries to see the sides that are yet to be discovered.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "behaviorExploreMarkedCube.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/navMemoryMap/navMemoryMapInterface.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreMarkedCube::BehaviorExploreMarkedCube(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  
}
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreMarkedCube::~BehaviorExploreMarkedCube()
{  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorExploreMarkedCube::IsRunnable(const Robot& robot, double currentTime_sec) const
{
  
  INavMemoryMap::BorderVector borders;
  INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
  memoryMap->CalculateBorders(NavMemoryMapTypes::EContentType::ObstacleCube, NavMemoryMapTypes::EContentType::Unknown, borders);

  if ( borders.empty() ) {
    PRINT_NAMED_INFO("RSAM", "IsRunnable NO");
  } else {
    PRINT_NAMED_INFO("RSAM", "IsRunnable YES");
  }
  return false;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExploreMarkedCube::InitInternal(Robot& robot, double currentTime_sec, bool isResuming)
{
  PRINT_NAMED_INFO("RSAM", "Init");
  return Result::RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorExploreMarkedCube::Status BehaviorExploreMarkedCube::UpdateInternal(Robot& robot, double currentTime_sec)
{
  PRINT_NAMED_INFO("RSAM", "Update");
  Status retval = Status::Complete;
  return retval;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorExploreMarkedCube::InterruptInternal(Robot& robot, double currentTime_sec, bool isShortInterrupt)
{
  return Result::RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreMarkedCube::StopInternal(Robot& robot, double currentTime_sec)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorExploreMarkedCube::HandleWhileRunning(const EngineToGameEvent& event, Robot& robot)
{
  PRINT_NAMED_INFO("RSAM", "HandleWhileRunning");
}

} // namespace Cozmo
} // namespace Anki
