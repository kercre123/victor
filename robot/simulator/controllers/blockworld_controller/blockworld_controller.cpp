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
#include <fstream>


#include <webots/Supervisor.hpp>
#include "basestationKeyboardController.h"

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/rotatedRect_impl.h"
#include "anki/common/basestation/utils/timer.h"
#include "anki/common/basestation/general.h"
#include "anki/common/basestation/math/quad_impl.h"
#include "anki/common/basestation/math/point_impl.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "messageHandler.h"
#include "anki/cozmo/basestation/robot.h"
//#include "anki/messaging/basestation/messagingInterface.h"
#include "vizManager.h"
#include "pathPlanner.h"
#include "behaviorManager.h"
#include "anki/common/basestation/jsonTools.h"

#include "anki/cozmo/robot/cozmoConfig.h"
#include "anki/common/basestation/platformPathManager.h"

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
  BehaviorManager behaviorMgr;

  // read planner motion primitives
  Json::Value mprims;
  const std::string subPath("coretech/planning/matlab/cozmo_mprim.json");
  const std::string jsonFilename = PREPEND_SCOPED_PATH(Config, subPath);

  Json::Reader reader;
  std::ifstream jsonFile(jsonFilename);
  reader.parse(jsonFile, mprims);
  jsonFile.close();

  LatticePlanner pathPlanner(&blockWorld, mprims);
  
  // Initialize the modules by telling them about each other:
  msgHandler.Init(&robotComms, &robotMgr, &blockWorld);
  robotMgr.Init(&msgHandler, &blockWorld, &pathPlanner);
  blockWorld.Init(&robotMgr);
  behaviorMgr.Init(&robotMgr, &blockWorld);
  
  
  // Allow webots to step once first to ensure that
  // the messages sent in VizManager::Init() have
  // somewhere to go.
  Sim::basestationController.step(BS_TIME_STEP);
  VizManager::getInstance()->Init();
  
#if(ENABLE_BS_KEYBOARD_CONTROL)
  Sim::BSKeyboardController::Init(&robotMgr, &blockWorld, &behaviorMgr);
  Sim::BSKeyboardController::Enable();
#endif
  
  //
  // Main Execution loop: step the world forward forever
  //
  while (Sim::basestationController.step(BS_TIME_STEP) != -1)
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
            robotMgr.GetRobotByID(robotID)->SendInit();
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
      
    // Draw observed markers, but only if images are being streamed
    blockWorld.DrawObsMarkers();
    
    // Update the world (force robots to process their messages)
    uint32_t numBlocksObserved = 0;
    blockWorld.Update(numBlocksObserved);
    
    // Update the behavior manager.
    // TODO: This object encompasses, for the time-being, what some higher level
    // module(s) would do.  e.g. Some combination of game state, build planner,
    // personality planner, etc.
    behaviorMgr.Update();
    
    
    

    /////////// Update visualization ////////////
    { // Update Block-of-Interest display
      
      // Get selected block of interest from Behavior manager
      static ObjectID_t prev_boi = 0;      // Previous block of interest
      static size_t prevNumPreDockPoses = 0;  // Previous number of predock poses
      // TODO: store previous block's color and restore it when unselecting
      
      // Draw current block of interest
      const ObjectID_t boi = behaviorMgr.GetObjectOfInterest();
      
      DockableObject* block = dynamic_cast<DockableObject*>(blockWorld.GetObjectByID(boi));
      if(block != nullptr) {

        // Get predock poses
        std::vector<Block::PoseMarkerPair_t> poses;
        block->GetPreDockPoses(PREDOCK_DISTANCE_MM, poses);
        
        // Erase previous predock pose marker for previous block of interest
        if (prev_boi != boi || poses.size() != prevNumPreDockPoses) {
          PRINT_INFO("BOI %d (prev %d), numPoses %d (prev %zu)\n", boi, prev_boi, (u32)poses.size(), prevNumPreDockPoses);
          VizManager::getInstance()->EraseVizObjectType(VIZ_OBJECT_PREDOCKPOSE);
          
          // Return previous selected block to original color (necessary in the
          // case that this block isn't currently being observed, meaning its
          // visualization won't have updated))
          DockableObject* prevBlock = dynamic_cast<DockableObject*>(blockWorld.GetObjectByID(prev_boi));
          if(prevBlock != nullptr && prevBlock->GetLastObservedTime() < BaseStationTimer::getInstance()->GetCurrentTimeStamp()) {
            prevBlock->Visualize(VIZ_COLOR_DEFAULT);
          }
          
          prev_boi = boi;
          prevNumPreDockPoses = poses.size();
        }

        // Draw cuboid for current selection, with predock poses
        block->Visualize(VIZ_COLOR_SELECTED_OBJECT, PREDOCK_DISTANCE_MM);
        
      } else {
        // block == nullptr (no longer exists, delete its predock poses)
        VizManager::getInstance()->EraseVizObjectType(VIZ_OBJECT_PREDOCKPOSE);
      }
      
    } // if blocks were updated
    
    // Draw all robot poses
    // TODO: Only send when pose has changed?
    for(auto robotID : robotMgr.GetRobotIDList())
    {
      Robot* robot = robotMgr.GetRobotByID(robotID);
      
      // Triangle pose marker
      VizManager::getInstance()->DrawRobot(robotID, robot->GetPose());
      
      // Full Webots CozmoBot model
      VizManager::getInstance()->DrawRobot(robotID, robot->GetPose(), robot->GetHeadAngle(), robot->GetLiftAngle());
      
      // Robot bounding box
      using namespace Quad;
      Quad2f quadOnGround2d = robot->GetBoundingQuadXY();
      Quad3f quadOnGround3d(Point3f(quadOnGround2d[TopLeft].x(),     quadOnGround2d[TopLeft].y(),     0.5f),
                            Point3f(quadOnGround2d[BottomLeft].x(),  quadOnGround2d[BottomLeft].y(),  0.5f),
                            Point3f(quadOnGround2d[TopRight].x(),    quadOnGround2d[TopRight].y(),    0.5f),
                            Point3f(quadOnGround2d[BottomRight].x(), quadOnGround2d[BottomRight].y(), 0.5f));

      VizManager::getInstance()->DrawRobotBoundingBox(robot->GetID(), quadOnGround3d, VIZ_COLOR_ROBOT_BOUNDING_QUAD);
      
      if(robot->IsCarryingObject()) {
        DockableObject* carryBlock = dynamic_cast<DockableObject*>(blockWorld.GetObjectByID(robot->GetCarryingObject()));
        if(carryBlock == nullptr) {
          PRINT_NAMED_ERROR("BlockWorldController.CarryBlockDoesNotExist", "Robot %d is marked as carrying block %d but that block no longer exists.\n", robot->GetID(), robot->GetCarryingObject());
          robot->SetCarryingObject(ANY_OBJECT);
        } else {
          carryBlock->Visualize(VIZ_COLOR_DEFAULT);
        }
      }
    }

    /////////// End visualization update ////////////
    

    // Process keyboard input
    if (Sim::BSKeyboardController::IsEnabled()) {
      Sim::BSKeyboardController::Update();
    }
    
  } // while still stepping

  //delete msgInterface;
  
  return 0;
}

