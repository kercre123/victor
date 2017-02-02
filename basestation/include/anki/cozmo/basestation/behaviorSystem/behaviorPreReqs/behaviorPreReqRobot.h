/**
 * File: behaviorPreReqRobot.h
 *
 * Author: Kevin M. Karol
 * Created: 12/09/16
 *
 * Description: Defines pre requisites that can be passed into
 * behavior's isRunnable() that don't require data
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqRobot_H__
#define __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqRobot_H__

namespace Anki {
namespace Cozmo {

class BehaviorPreReqRobot {
public:
  BehaviorPreReqRobot(const Robot& robot)
  :_robot(robot){}
  
  const Robot& GetRobot() const { return _robot;}
  
private:
  const Robot& _robot;
  
};

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_BehaviorSystem_BehaviorPreReqRobot_H__


