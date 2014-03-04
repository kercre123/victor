/*
 * File:          world_comms_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include <cstdio>
#include <queue>
#include <unistd.h>

//#include "CozmoWorldComms.h"
#include <webots/Supervisor.hpp>

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/basestation/general.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "messageHandler.h"
#include "anki/cozmo/basestation/robot.h"
//#include "anki/messaging/basestation/messagingInterface.h"
#include "vizManager.h"
#include "pathPlanner.h"
#include "behaviorManager.h"

#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/cozmo/basestation/tcpComms.h"


webots::Supervisor basestationController;


//
// main()
//

using namespace Anki;
using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  
  // Instantiate all the modules we need
  TCPComms robotComms;
  BlockWorld blockWorld;
  RobotManager robotMgr;
  MessageHandler msgHandler;
  PathPlanner pathPlanner;
  BehaviorManager behaviorMgr;
  
  // Initialize the modules by telling them about each other:
  msgHandler.Init(&robotComms, &robotMgr, &blockWorld);
  robotMgr.Init(&msgHandler, &blockWorld, &pathPlanner);
  blockWorld.Init(&robotMgr);
  behaviorMgr.Init(&robotMgr, &blockWorld);
  
  VizManager::getInstance()->Init();
  
  basestationController.keyboardEnable(TIME_STEP);
  
  //
  // Main Execution loop: step the world forward forever
  //
  while (basestationController.step(TIME_STEP) != -1)
  {
    // Update time
    // (To be done from iOS eventually)
    BaseStationTimer::getInstance()->UpdateTime(SEC_TO_NANOS(basestationController.getTime()));
    
    // Read messages from all robots
    robotComms.Update();
    
    // If not already connected to a robot, connect to the
    // first one that becomes available.
    // TODO: Once we have a UI, we can select the one we want to connect to in a more reasonable way.
    if (robotComms.GetNumConnectedRobots() == 0) {
      vector<int> advertisingRobotIDs;
      if (robotComms.GetAdvertisingRobotIDs(advertisingRobotIDs) > 0) {
        for(auto robotID : advertisingRobotIDs) {
          printf("RobotComms connecting to robot %d.\n", robotID);
          robotComms.ConnectToRobotByID(robotID);
          robotMgr.AddRobot(robotID);
        }
      }
    }
    
    // If we still don't have any connected robots, don't proceed
    if (robotComms.GetNumConnectedRobots() == 0) {
      continue;
    }

    //MessageHandler::getInstance()->ProcessMessages();
    msgHandler.ProcessMessages();
    
    //
    // Check for any outgoing messages from each basestation robot:
    //
    for(auto robotiD : robotMgr.GetRobotIDList())
    {
      Robot* robot = robotMgr.GetRobotByID(robotiD);
      while(robot->hasOutgoingMessages())
      {
        Comms::MsgPacket p;
        p.destId = robot->get_ID();
        robot->getOutgoingMessage(p.data, p.dataLen);
        if (p.dataLen > 0) {
          robotComms.Send(p);
        }
      } // while robot i still has outgoing messages to send
      
    } // for each robot
    
    // Update the world (force robots to process their messages)
    blockWorld.Update();
    
    // Update the behavior manager.
    // TODO: This object encompasses, for the time-being, what some higher level
    // module(s) would do.  e.g. Some combination of game state, build planner,
    // personality planner, etc.
    behaviorMgr.Update();
    
  } // while still stepping

  //delete msgInterface;
  
  return 0;
}

