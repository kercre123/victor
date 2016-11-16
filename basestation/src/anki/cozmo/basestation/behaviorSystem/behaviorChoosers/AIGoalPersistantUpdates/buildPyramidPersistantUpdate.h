/**
 * File: buildPyramidPersistantUpdate.h
 *
 * Author: Kevin M. Karol
 * Created: 11/14/16
 *
 * Description: Persistant update function for BuildPyramid
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_AIGoalPersistantUpdate_BuildPyramidPersistantUpdate_H__
#define __Cozmo_Basestation_AIGoalPersistantUpdate_BuildPyramidPersistantUpdate_H__


#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalPersistantUpdates/iGoalPersistantUpdate.h"

namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIGoalEvaluator
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BuildPyramidPersistantUpdate : public IGoalPersistantUpdate
{
public:
  BuildPyramidPersistantUpdate();
  virtual void Update(Robot& robot) override;
  
private:
  int _lastPyramidBaseSeenCount;
  bool _wasRobotCarryingObject;
  bool _sparksPersistantCallbackSet;

};
  

} // namespace Cozmo
} // namespace Anki

#endif //
