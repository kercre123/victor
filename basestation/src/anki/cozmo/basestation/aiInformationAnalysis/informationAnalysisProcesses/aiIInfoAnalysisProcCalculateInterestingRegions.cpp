/**
 * File: aiInfoAnalysisProcCalculateInterestingRegions
 *
 * Author: Raul
 * Created: 09/13/16
 *
 * Description: Process to analyze memory map and generate information about the interesting regions.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "aiInfoAnalysisProcCalculateInterestingRegions.h"
#include "../aiInformationAnalyzer.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/navMemoryMap/iNavMemoryMap.h"
#include "anki/cozmo/basestation/robot.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"

#include <type_traits>

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace
{

// Configuration of memory map to check for interesting regions
constexpr NavMemoryMapTypes::FullContentArray typesToExploreInterestingBordersFrom =
{
  {NavMemoryMapTypes::EContentType::Unknown               , true },
  {NavMemoryMapTypes::EContentType::ClearOfObstacle       , true },
  {NavMemoryMapTypes::EContentType::ClearOfCliff          , true },
  {NavMemoryMapTypes::EContentType::ObstacleCube          , false},
  {NavMemoryMapTypes::EContentType::ObstacleCubeRemoved   , false},
  {NavMemoryMapTypes::EContentType::ObstacleCharger       , false},
  {NavMemoryMapTypes::EContentType::ObstacleChargerRemoved, false},
  {NavMemoryMapTypes::EContentType::ObstacleUnrecognized  , false},
  {NavMemoryMapTypes::EContentType::Cliff                 , false},
  {NavMemoryMapTypes::EContentType::InterestingEdge       , false},
  {NavMemoryMapTypes::EContentType::NotInterestingEdge    , false}
};
static_assert(NavMemoryMapTypes::ContentValueEntry::IsValidArray(typesToExploreInterestingBordersFrom),
  "This array does not define all types once and only once.");

};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIInfoAnalysisProcCalculateInterestingRegions(AIInformationAnalyzer& analyzer, Robot& robot)
{
  ANKI_CPU_PROFILE("InfoAnalysisProcCalculateInterestingRegions");
  
  // calculate regions
  INavMemoryMap* memoryMap = robot.GetBlockWorld().GetNavMemoryMap();
  memoryMap->CalculateBorders(NavMemoryMapTypes::EContentType::InterestingEdge, typesToExploreInterestingBordersFrom, analyzer._interestingRegions);
}

} // namespace
} // namespace
