/**
 * File: iGoalPersistentUpdates.h
 *
 * Author: Kevin M. Karol
 * Created: 11/14/16
 *
 * Description: Allows Goals to have a persistant update function that is ticked and
 * can live across multiple behaviors
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_AIGoalPersistentUpdate_IGoalPersistentUpdate_H__
#define __Cozmo_Basestation_AIGoalPersistentUpdate_IGoalPersistentUpdate_H__

namespace Anki {
namespace Cozmo {
  
class Robot;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIGoalEvaluator
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class IGoalPersistentUpdate
{
public:
  virtual ~IGoalPersistentUpdate(){};
  virtual void Init(Robot& robot) = 0;
  virtual void Update(Robot& robot) = 0;
  virtual void GoalEntered(Robot& robot){};
  virtual void GoalExited(Robot& robot){};
  
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
