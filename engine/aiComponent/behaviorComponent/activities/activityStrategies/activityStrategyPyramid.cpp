/**
 * File: activityStrategyPyramid.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2/15/2017
 *
 * Description: Strategy which determines when the pyramid chooser will run in freeplay
 *
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/aiComponent/behaviorComponent/activities/activityStrategies/activityStrategyPyramid.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/behaviorExternalInterface.h"
#include "engine/aiComponent/behaviorComponent/behaviorExternalInterface/stateChangeComponent.h"
#include "engine/blockWorld/blockConfigurationManager.h"
#include "engine/blockWorld/blockWorld.h"
#include "engine/components/progressionUnlockComponent.h"
#include "anki/common/basestation/utils/timer.h"
#include "engine/robot.h"

#include "clad/types/behaviorSystem/behaviorObjectives.h"
#include "util/random/randomGenerator.h"

namespace Anki {
namespace Cozmo {

namespace{
static const int    kMinimumBlocksForPyramid     = 3;
static const double kOddsPyramidIsRunnable       = 0.4;
static const float  kUpdateRunnabilityInterval_s = 300;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActivityStrategyPyramid::ActivityStrategyPyramid(BehaviorExternalInterface& behaviorExternalInterface,
                                                 IExternalInterface* robotExternalInterface,
                                                 const Json::Value& config)
: IActivityStrategy(behaviorExternalInterface, robotExternalInterface, config)
, _nextTimeUpdateRunability_s(0)
, _wantsToRunRandomized(false)
, _pyramidBuilt(false)
{
  
  // Listen for behavior objective achieved messages for spark repetitions counter
  if(robotExternalInterface != nullptr) {
    auto helper = MakeAnkiEventUtil(*robotExternalInterface, *this, _eventHandles);
    using namespace ExternalInterface;
    helper.SubscribeEngineToGame<MessageEngineToGameTag::BehaviorObjectiveAchieved>();
  }
}

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategyPyramid::WantsToStartInternal(BehaviorExternalInterface& behaviorExternalInterface, float lastTimeActivityRanSec) const
{  
  if((behaviorExternalInterface.HasProgressionUnlockComponent()) &&
     !behaviorExternalInterface.GetProgressionUnlockComponent().IsUnlocked(UnlockId::BuildPyramid)){
    return false;
  }
  
  typedef std::vector<const ObservableObject*> BlockList;
  
  BlockList knownOrDirtyBlocks;
  BlockWorldFilter knownOrDirtyBlocksFilter;
  knownOrDirtyBlocksFilter.SetAllowedFamilies(
          {{ObjectFamily::LightCube, ObjectFamily::Block}});
  
  behaviorExternalInterface.GetBlockWorld().FindLocatedMatchingObjects(knownOrDirtyBlocksFilter,
                                                                       knownOrDirtyBlocks);
  
  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if(currentTime_s > _nextTimeUpdateRunability_s){
    _wantsToRunRandomized = behaviorExternalInterface.GetRNG().RandDbl() < kOddsPyramidIsRunnable;
    _nextTimeUpdateRunability_s = currentTime_s + kUpdateRunnabilityInterval_s;
  }
  
  const int numBases = static_cast<int>(behaviorExternalInterface.GetBlockWorld().GetBlockConfigurationManager().
                                                GetPyramidBaseCache().GetWeakBases().size());
  
  const int numPyramids = static_cast<int>(behaviorExternalInterface.GetBlockWorld().GetBlockConfigurationManager().
                                                   GetPyramidCache().GetWeakPyramids().size());
  
  const bool willRun =
        (numPyramids == 0) &&
        ((numBases > 0) ||
        (_wantsToRunRandomized && knownOrDirtyBlocks.size() >= kMinimumBlocksForPyramid));
  
  if(willRun){
    _pyramidBuilt = false;
  }
  
  return willRun;
};

  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ActivityStrategyPyramid::WantsToEndInternal(BehaviorExternalInterface& behaviorExternalInterface, float lastTimeActivityStartedSec) const
{
  return _pyramidBuilt;
};
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<>
void ActivityStrategyPyramid::HandleMessage(const ExternalInterface::BehaviorObjectiveAchieved& msg)
{
  BehaviorObjective objectiveAchieved = msg.behaviorObjective;
  if(objectiveAchieved == BehaviorObjective::BuiltPyramid){
    _pyramidBuilt = true;
  }
}

  

} // namespace
} // namespace
