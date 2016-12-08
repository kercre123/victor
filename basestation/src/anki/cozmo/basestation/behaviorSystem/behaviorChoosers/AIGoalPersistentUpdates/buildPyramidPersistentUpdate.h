/**
 * File: buildPyramidPersistentUpdate.h
 *
 * Author: Kevin M. Karol
 * Created: 11/14/16
 *
 * Description: Persistent update function for BuildPyramid
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_AIGoalPersistentUpdate_BuildPyramidPersistentUpdate_H__
#define __Cozmo_Basestation_AIGoalPersistentUpdate_BuildPyramidPersistentUpdate_H__


#include "anki/cozmo/basestation/behaviorSystem/behaviorChoosers/AIGoalPersistentUpdates/iGoalPersistentUpdate.h"
#include "anki/cozmo/basestation/behaviors/sparkable/behaviorBuildPyramidBase.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Cozmo {


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIGoalEvaluator
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class BuildPyramidPersistentUpdate : public IGoalPersistentUpdate
{
public:
  BuildPyramidPersistentUpdate();
  virtual void Init(Robot& robot) override;
  virtual void Update(Robot& robot) override;
  virtual void GoalEntered(Robot& robot) override;

  
private:
  /// Attributes
  int _lastPyramidBaseSeenCount;
  bool _wasRobotCarryingObject;
  Signal::SmartHandle _eventHalder;
  BehaviorBuildPyramidBase::MusicState _currentState;  
  
  /// Methods
  void RequestUpdateMusicLightState(Robot& robot,
                        BehaviorBuildPyramidBase::MusicState stateToSet);
  

};
  

} // namespace Cozmo
} // namespace Anki

#endif //
