/**
 * File: AIGoalStrategyFPHiking
 *
 * Author: Raul
 * Created: 08/10/2016
 *
 * Description: Specific strategy for FreePlay Hiking goal.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "AIGoalStrategyFPHiking.h"

#include "anki/cozmo/basestation/behaviorManager.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/navMemoryMap/iNavMemoryMap.h"
#include "anki/cozmo/basestation/robot.h"

#include "anki/common/basestation/jsonTools.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AIGoalStrategyFPHiking::AIGoalStrategyFPHiking(Robot& robot, const Json::Value& config)
: Anki::Cozmo::IAIGoalStrategy(config)
, _createBeacons(false)
, _visitInterestingEdges(false)
, _gatherUsableCubesOutOfBeacons(false)
{
  using namespace JsonTools;

  _createBeacons = ParseBool(config, "createBeacons", "AIGoalStrategyFPHiking");
  _visitInterestingEdges = ParseBool(config, "visitInterestingEdges", "AIGoalStrategyFPHiking");
  _gatherUsableCubesOutOfBeacons = ParseBool(config, "gatherUsableCubesOutOfBeacons", "AIGoalStrategyFPHiking");
  
  // something has to be enabled, otherwise goal will never start
  ASSERT_NAMED(_createBeacons||_visitInterestingEdges||_gatherUsableCubesOutOfBeacons,
    "AIGoalStrategyFPHiking.NoChecksAvailable");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIGoalStrategyFPHiking::WantsToStartInternal(const Robot& robot, float lastTimeGoalRanSec) const
{
  // wants to start any time there's something to do
  const bool hasSomethingToDo = HasSomethingToDo(robot);
  const bool wantsToStart = hasSomethingToDo;
  return wantsToStart;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIGoalStrategyFPHiking::WantsToEndInternal(const Robot& robot, float lastTimeGoalStartedSec) const
{
  // wants to end if there's nothing left to do
  const bool hasSomethingToDo = HasSomethingToDo(robot);
  const bool wantsToEnd = !hasSomethingToDo;
  return wantsToEnd;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AIGoalStrategyFPHiking::HasSomethingToDo(const Robot& robot) const
{
  // rsam: note on Start/End goals. I noticed that this goal needs to be too complex to cover all possible behaviors. For
  // example, it would have to check if any of the raycasts that LookInPlace would perform will activate that behavior.
  // That means that this goal has to have a deep knowledge of what underlying behaviors do or require. I don't
  // think that's scalable. Instead, I'm gonna let goals say whether "Cozmo feels like doing them" (based on mood,
  // cooldown, max duration, etc), but any other complex checks will be done by their behaviors. If a goal picks
  // NoneBehavior, then the goal evaluator will kick the goal out nicely, and let other goal with lower priority
  // take its place.
  // In order to do that here, return that there's always something to do, and if no behaviors run, we will be asked to
  // exit hiking.
  return true;

  // check if Cozmo needs to create beacons
  if ( _createBeacons )
  {
    // ask whiteboard if there's an active one
    const AIWhiteboard& whiteboard = robot.GetBehaviorManager().GetWhiteboard();
    const AIBeacon* activeBeacon = whiteboard.GetActiveBeacon();
    const bool needsBeacon = ( nullptr == activeBeacon );
    if ( needsBeacon ) {
      return true;
    }
  }

  // check if Cozmo needs to visit interesting edges
  if ( _visitInterestingEdges )
  {
    // ask memory map if there are any interesting edges
    const INavMemoryMap* currentNavMemoryMap = robot.GetBlockWorld().GetNavMemoryMap();
    if( currentNavMemoryMap )
    {
      const bool hasInterestingEdges = currentNavMemoryMap->HasContentType( NavMemoryMapTypes::EContentType::InterestingEdge );
      if ( hasInterestingEdges ) {
        return true;
      }
    }
  }
  
  // check if Cozmo needs to bring cubes to a beacon
  if ( _gatherUsableCubesOutOfBeacons )
  {
    // ask whiteboard if there are any usable cubes we want to bring to beacons
    AIWhiteboard::ObjectInfoList wasteList;
    const AIWhiteboard& whiteboard = robot.GetBehaviorManager().GetWhiteboard();
    const bool hasAnyUsableObjectsOutOfBeacons = whiteboard.FindUsableCubesOutOfBeacons(wasteList);
    if ( hasAnyUsableObjectsOutOfBeacons ) {
      return true;
    }
  }

  // nothing to do
  return false;
}
  
} // namespace
} // namespace
