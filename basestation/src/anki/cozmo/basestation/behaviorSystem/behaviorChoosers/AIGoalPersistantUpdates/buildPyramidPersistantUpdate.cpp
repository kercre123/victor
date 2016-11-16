/**
 * File: buildPyramidPersistantUpdate.cpp
 *
 * Author: Kevin M. Karol
 * Created: 11/14/16
 *
 * Description: Persistant update function for BuildPyramid
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalPersistantUpdates/buildPyramidPersistantUpdate.h"

#include "anki/cozmo/basestation/behaviors/sparkable/behaviorBuildPyramidBase.h"

namespace Anki {
namespace Cozmo {

BuildPyramidPersistantUpdate::BuildPyramidPersistantUpdate()
: _lastPyramidBaseSeenCount(0)
, _wasRobotCarryingObject(false)
, _sparksPersistantCallbackSet(false)
{

}

  
void BuildPyramidPersistantUpdate::Update(Robot& robot)
{
  BehaviorBuildPyramidBase::SparksPersistantMusicLightControls(robot,
        _lastPyramidBaseSeenCount, _wasRobotCarryingObject, _sparksPersistantCallbackSet);
}

} // namespace Cozmo
} // namespace Anki

