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
    if(blockWorld.DidBlocksChange())
    {
      // Get selected block of interest from Behavior manager
      static ObjectID_t prev_boi = 0;      // Previous block of interest
      static u32 prevNumPreDockPoses = 0;  // Previous number of predock poses
      
      // Get current block of interest
      const ObjectID_t boi = behaviorMgr.GetBlockOfInterest();
      
      // Draw all blocks we know about
      for(auto blocksByType : blockWorld.GetAllExistingBlocks()) {
        for(auto blocksByID : blocksByType.second) {
          
          const Block* block = dynamic_cast<Block*>(blocksByID.second);
          
          u32 color = VIZ_COLOR_DEFAULT;
          
          // Special treatment for block of interest
          if (blocksByID.first == boi) {
            // Set different color
            color = VIZ_COLOR_SELECTED_OBJECT;
            
            // Get predock poses
            std::vector<Block::PoseMarkerPair_t> poses;
            block->GetPreDockPoses(PREDOCK_DISTANCE_MM, poses);
            
            // Erase previous predock pose marker for previous block of interest
            if (prev_boi != boi || poses.size() != prevNumPreDockPoses) {
              PRINT_INFO("BOI %d (prev %d), numPoses %d (prev %d)\n", boi, prev_boi, (u32)poses.size(), prevNumPreDockPoses);
              VizManager::getInstance()->EraseVizObjectType(VIZ_PREDOCKPOSE);
              prev_boi = boi;
              prevNumPreDockPoses = poses.size();
            }
            
            // Draw predock poses
            u32 poseID = 0;
            for(auto pose : poses) {
              VizManager::getInstance()->DrawPreDockPose(6*block->GetID()+poseID++, pose.first, VIZ_COLOR_PREDOCKPOSE);
              ++poseID;
            }
          }
          
          // Draw cuboid
          VizManager::getInstance()->DrawCuboid(block->GetID(),
                                                //block->GetType(),
                                                block->GetSize(),
                                                block->GetPose(),
                                                color);
          
          // Draw blocks' projected quads on the mat
          {
            using namespace Quad;
            // TODO:(bn) ask andrew why its like this
            // TODO:(bn) real parameters
            float paddingRadius = 65.0;
            float blockRadius = 44.0;
            float paddingFactor = (paddingRadius + blockRadius) / blockRadius;
            Quad2f quadOnGround2d = block->GetBoundingQuadXY(paddingFactor);

            RotatedRectangle boundingRect;
            boundingRect.ImportQuad(quadOnGround2d);

            Quad2f rectOnGround2d(boundingRect.GetQuad());
            // Quad2f rectOnGround2d(quadOnGround2d);
            
            Quad3f quadOnGround3d(Point3f(rectOnGround2d[TopLeft].x(),     rectOnGround2d[TopLeft].y(),     0.5f),
                                  Point3f(rectOnGround2d[BottomLeft].x(),  rectOnGround2d[BottomLeft].y(),  0.5f),
                                  Point3f(rectOnGround2d[TopRight].x(),    rectOnGround2d[TopRight].y(),    0.5f),
                                  Point3f(rectOnGround2d[BottomRight].x(), rectOnGround2d[BottomRight].y(), 0.5f));
            
            VizManager::getInstance()->DrawQuad(block->GetID(), quadOnGround3d, VIZ_COLOR_BLOCK_BOUNDING_QUAD);
          }
          
          
        } // FOR each ID of this type
      } // FOR each type
      
    } // if locks were updated
    
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

