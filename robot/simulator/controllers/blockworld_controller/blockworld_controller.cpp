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

#include <webots/Supervisor.hpp>
#include "basestationKeyboardController.h"

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

// Enable this to turn on keyboard control of robot via basestation.
// If this is enabled, make sure robot-side keyboard control (see ENABLE_KEYBOARD_CONTROL) is disabled!!!
#define ENABLE_BS_KEYBOARD_CONTROL 1


namespace Anki {
  namespace Cozmo {
    namespace Sim {
      webots::Supervisor basestationController;
    }
  }
}

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
  
  
  // Allow webots to step once first to ensure that
  // the messages sent in VizManager::Init() have
  // somewhere to go.
  Sim::basestationController.step(TIME_STEP);
  VizManager::getInstance()->Init();
  
#if(ENABLE_BS_KEYBOARD_CONTROL)
  Sim::BSKeyboardController::Init(&robotMgr, &blockWorld, &behaviorMgr);
  Sim::BSKeyboardController::Enable();
#endif
  
  //
  // Main Execution loop: step the world forward forever
  //
  while (Sim::basestationController.step(TIME_STEP) != -1)
  {
    // Update time
    // (To be done from iOS eventually)
    BaseStationTimer::getInstance()->UpdateTime(SEC_TO_NANOS(Sim::basestationController.getTime()));
    
    // Read messages from all robots
    robotComms.Update();
    
    // If not already connected to a robot, connect to the
    // first one that becomes available.
    // TODO: Once we have a UI, we can select the one we want to connect to in a more reasonable way.
    if (robotComms.GetNumConnectedRobots() == 0) {
      std::vector<int> advertisingRobotIDs;
      if (robotComms.GetAdvertisingRobotIDs(advertisingRobotIDs) > 0) {
        for(auto robotID : advertisingRobotIDs) {
          printf("RobotComms connecting to robot %d.\n", robotID);
          if (robotComms.ConnectToRobotByID(robotID)) {
            printf("Connected to robot %d\n", robotID);
            robotMgr.AddRobot(robotID);
            robotMgr.GetRobotByID(robotID)->SendRequestCamCalib();
            break;
          } else {
            printf("Failed to connect to robot %d\n", robotID);
            return -1;
          }
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
    
    
    

    /////////// Update visualization ////////////
    
    // Get selected block of interest from Behavior manager
    const ObjectID_t boi = behaviorMgr.GetBlockOfInterest();
    
    // Draw all blocks we know about (and their pre-dock poses)
    //VizManager::getInstance()->EraseAllVizObjects();
    for(auto blocksByType : blockWorld.GetAllExistingBlocks()) {
      for(auto blocksByID : blocksByType.second) {
        
        // Set different color for selected block of interest
        u32 color = VIZ_COLOR_DEFAULT;
        if (blocksByID.first == boi) {
          //PRINT_INFO("Setting color for block of interest id %d type %d\n", boi.id, boi.type);
          color = VIZ_COLOR_SELECTED_OBJECT;
        }
        
        const Block* block = dynamic_cast<Block*>(blocksByID.second);
        VizManager::getInstance()->DrawCuboid(block->GetID(),
                                              //block->GetType(),
                                              block->GetSize(),
                                              block->GetPose(),
                                              color);
        
        std::vector<Pose3d> poses;
        //block->GetPreDockPoses(PREDOCK_DISTANCE_MM, poses);
        block->GetPreDockPoses(Vision::MARKER_BATTERIES, PREDOCK_DISTANCE_MM, poses);
        u32 poseID = 0;
        for(auto pose : poses) {
          VizManager::getInstance()->DrawPreDockPose(6*block->GetID()+poseID++, pose, VIZ_COLOR_PREDOCKPOSE);
          ++poseID;
        }
        
      } // FOR each ID of this type
    } // FOR each type
    
    
    // Draw all robot poses
    // TODO: Only send when pose has changed?
    for(auto robotID : robotMgr.GetRobotIDList())
    {
      Robot* robot = robotMgr.GetRobotByID(robotID);
      
      // Triangle pose marker
      VizManager::getInstance()->DrawRobot(robotID, robot->get_pose());
      
      // Full Webots CozmoBot model
      VizManager::getInstance()->DrawRobot(robotID, robot->get_pose(), robot->get_headAngle(), robot->get_liftAngle());
    }

    /////////// End visualization update ////////////
    

    // Process keyboard input
    if (Sim::BSKeyboardController::IsEnabled()) {
      Sim::BSKeyboardController::ProcessKeystroke();
    }
    
  } // while still stepping

  //delete msgInterface;
  
  return 0;
}

