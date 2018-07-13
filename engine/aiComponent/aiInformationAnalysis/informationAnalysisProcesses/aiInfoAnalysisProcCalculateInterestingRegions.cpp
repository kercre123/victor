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
#include "engine/aiComponent/aiInformationAnalysis/informationAnalysisProcesses/aiInfoAnalysisProcCalculateInterestingRegions.h"
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalyzer.h"

#include "engine/blockWorld/blockWorld.h"
#include "engine/navMap/mapComponent.h"
#include "engine/robot.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/logging/logging.h"

#include <type_traits>

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace
{

// Configuration of memory map to check for interesting regions
constexpr MemoryMapTypes::FullContentArray typesToExploreInterestingBordersFrom =
{
  {MemoryMapTypes::EContentType::Unknown               , true },
  {MemoryMapTypes::EContentType::ClearOfObstacle       , true },
  {MemoryMapTypes::EContentType::ClearOfCliff          , true },
  {MemoryMapTypes::EContentType::ObstacleObservable    , false},
  {MemoryMapTypes::EContentType::ObstacleProx          , false},
  {MemoryMapTypes::EContentType::ObstacleUnrecognized  , false},
  {MemoryMapTypes::EContentType::Cliff                 , false},
  {MemoryMapTypes::EContentType::InterestingEdge       , false},
  {MemoryMapTypes::EContentType::NotInterestingEdge    , false}
};
static_assert(MemoryMapTypes::IsSequentialArray(typesToExploreInterestingBordersFrom),
  "This array does not define all types once and only once.");

};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AIInfoAnalysisProcCalculateInterestingRegions(AIInformationAnalyzer& analyzer, Robot& robot)
{
  ANKI_CPU_PROFILE("InfoAnalysisProcCalculateInterestingRegions");
  
  // calculate regions
  INavMap* memoryMap = robot.GetMapComponent().GetCurrentMemoryMap();
  INavMap::BorderRegionVector visionEdges, proxEdges;
  
  analyzer._interestingRegions.clear();
  
  memoryMap->CalculateBorders(MemoryMapTypes::EContentType::InterestingEdge, typesToExploreInterestingBordersFrom, visionEdges);
  if (!visionEdges.empty()) {
   analyzer._interestingRegions.insert(end(analyzer._interestingRegions), begin(visionEdges), end(visionEdges));
  }
                       
  memoryMap->CalculateBorders(MemoryMapTypes::EContentType::ObstacleProx, typesToExploreInterestingBordersFrom, proxEdges);
  if (!proxEdges.empty()) {
    analyzer._interestingRegions.insert(end(analyzer._interestingRegions), begin(proxEdges), end(proxEdges));
  }
}



} // namespace
} // namespace
