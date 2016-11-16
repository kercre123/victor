/**
 * File: iGoalPersistantUpdates.h
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
#ifndef __Cozmo_Basestation_AIGoalPersistantUpdate_IGoalPersistantUpdate_H__
#define __Cozmo_Basestation_AIGoalPersistantUpdate_IGoalPersistantUpdate_H__

namespace Anki {
namespace Cozmo {
  
class Robot;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// AIGoalEvaluator
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class IGoalPersistantUpdate
{
public:
  virtual ~IGoalPersistantUpdate(){};
  virtual void Update(Robot& robot) = 0;
};
  

} // namespace Cozmo
} // namespace Anki

#endif //
