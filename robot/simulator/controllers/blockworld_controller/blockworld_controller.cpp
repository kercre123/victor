/*
 * File:          world_comms_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include <cstdio>
#include <queue>


//#include "CozmoWorldComms.h"
#include <webots/Supervisor.hpp>

#include "anki/common/basestation/math/pose.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
//#include "anki/messaging/basestation/messagingInterface.h"

#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/cozmo/messageProtocol.h"

webots::Supervisor commsController;

class SimBlock
{
public:

  SimBlock(webots::Node* nodeIn, Anki::Cozmo::BlockType typeIn)
  : node(nodeIn), type(typeIn), selected(false), bwBlock(NULL)
  {
    transparencyField = node->getField("obsTransparency");
    rotField = node->getField("obsRotation");
    translationField = node->getField("obsTranslation");
    colorField = node->getField("obsColor");

    if(transparencyField == NULL) {
      fprintf(stdout, "Could not find 'obsTransparency' field for Webot block.");
      CORETECH_ASSERT(false);
    }
    
    if(rotField == NULL) {
      fprintf(stdout,
              "Could not find 'obsRotation' field for Webot block.");
      CORETECH_ASSERT(false);
    }
    
    if(translationField == NULL) {
      fprintf(stdout, "Could not find 'obsTranslation' field for Webot block.");
      CORETECH_ASSERT(false);
    }
    
    if(colorField == NULL) {
      fprintf(stdout, "Could not find 'obsColor' field for Webot block.");
      CORETECH_ASSERT(false);
    }

  }
  
  void updatePose(const Anki::Pose3d& blockPose)
  {
    // Update the closest block's observation node with the specified pose
    
    // The observation is a child of the block in Webots, so its pose is w.r.t.
    // the block's pose.  But the incoming observed pose is w.r.t. the world.
    // Adjust accordingly before updating the observed node's pose for display
    // in Webots:
    Anki::Pose3d obsPose( blockPose.getInverse() * bwBlock->get_pose() );
    
    Anki::Vec3f   rotAxis( obsPose.get_rotationAxis() );
    Anki::Radians rotAngle(obsPose.get_rotationAngle());
    
    // Convert to Webots coordinate system:
    const double webotRotation[4] = {
      static_cast<double>(rotAxis.x()),
      static_cast<double>(rotAxis.z()),
      static_cast<double>(rotAxis.y()),
      rotAngle.ToDouble()
    };
    
    rotField->setSFRotation(webotRotation);
    
    Anki::Vec3f translation(obsPose.get_translation());
    translation *= 1.f/1000.f; // convert from mm to meters
    const double webotTranslation[3] = {
      static_cast<double>(translation.x()),
      static_cast<double>(translation.z()),
      static_cast<double>(translation.y())
    };
    
    translationField->setSFVec3f(webotTranslation);
    
  } // updatePose()
  
  void draw(void) const
  {
    if(bwBlock != NULL) {
      transparencyField->setSFFloat(0.f);
      
      if(selected) {
        const double selectedColor[3] = {1., 1., 1.};
        colorField->setSFColor(selectedColor);
      } else {
        const double UNselectedColor[3] = {1., 1., 0.};
        colorField->setSFColor(UNselectedColor);
      }
    } else {
      // Don't display
      transparencyField->setSFFloat(1.f);
    }
  } // draw()
  
  webots::Node* node;

  Anki::Cozmo::BlockType type;
  bool      selected;
  
  const Anki::Cozmo::Block* bwBlock;
  
private:
  
  webots::Field* transparencyField;
  webots::Field* rotField;
  webots::Field* translationField;
  webots::Field* colorField;
};

//
// Helper function definitions (implementations below)
//

// Populate lists of block/robot nodes for later use
void initWorldNodes(std::map<std::string, webots::Node*>&  nameToNodeLUT,
                    std::vector<SimBlock>&             simBlocks,
                    std::vector<webots::Node*>&            robotNodes);

Anki::Pose3d getNodePose(webots::Node* node);
  

void ProcessKeystroke(Anki::Cozmo::BlockWorld& blockWorld,
                      std::vector<SimBlock>&   blockNodes);

int processPacket(const unsigned char *data, const int dataSize,
                  Anki::Cozmo::BlockWorld& blockWorld, int i_robot);



//
// main()
//

int main(int argc, char **argv)
{
  //Anki::MessagingInterface* msgInterface = new Anki::MessagingInterface_TCP();
  
  //Anki::Cozmo::BlockWorld::ZAxisPointsUp = false; // b/c this is Webots
  
  Anki::Cozmo::BlockWorld blockWorld;
  
  const int MAX_ROBOTS = Anki::Cozmo::BlockWorld::MaxRobots;
  
  webots::Receiver* rx[MAX_ROBOTS];
  webots::Emitter*  tx[MAX_ROBOTS];
  commsController.keyboardEnable(Anki::Cozmo::TIME_STEP);
 
  
  //
  // Initialize the node lists for the world this controller lives in
  //  (this needs to happen after wb_robot_init!)
  //
  std::map<std::string, webots::Node*>  nameToNodeLUT;
  std::vector<webots::Node*>            robotNodes;
  std::vector<SimBlock>             simBlocks;
  initWorldNodes(nameToNodeLUT, simBlocks, robotNodes);
  
  fprintf(stdout, "Found %lu robots and %lu blocks in the world.\n",
          robotNodes.size(), simBlocks.size());

  
  //
  // Initialize World Transmitters/Receivers
  // (one for each robot, up to MAX_ROBOTS)
  //
  char rxRadioName[12], txRadioName[12];
  
  for(int i=0; i<MAX_ROBOTS; ++i) {
    snprintf(rxRadioName, 11, "radio_rx_%d", i+1);
    snprintf(txRadioName, 11, "radio_tx_%d", i+1);
    
    tx[i] = commsController.getEmitter(txRadioName);
    rx[i] = commsController.getReceiver(rxRadioName);
    rx[i]->enable(Anki::Cozmo::TIME_STEP);
  }
 
  //
  // Main Execution loop: step the world forward forever
  //
  while (commsController.step(Anki::Cozmo::TIME_STEP) != -1)
  {
    //
    // Check for any incoming messages from each physical robot:
    //
    for(int i=0; i<MAX_ROBOTS; ++i)
    {
      int dataSize;
      const unsigned char* data;
      
      // Read receiver for as long as it is not empty.
      // Put its bytes into a byte queue.
      while (rx[i]->getQueueLength() > 0)
      {
        // Get head packet
        data = (unsigned char *)rx[i]->getData();
        dataSize = rx[i]->getDataSize();
        
        processPacket(data, dataSize, blockWorld, i);
        
        rx[i]->nextPacket();
        
      } // while receiver queue not empty
      
    } // for each receiver
    
    //
    // Check for any outgoing messages from each basestation robot:
    //
    for(int i=0; i<blockWorld.getNumRobots(); ++i)
    {
      CORETECH_ASSERT(i < MAX_ROBOTS);
      Anki::Cozmo::Robot& robot = blockWorld.getRobot(i);
      while(robot.hasOutgoingMessages())
      {
        // Buffer for the message data we're going to send:
        // (getOutgoingMessage() will copy data into it)
        unsigned char msgData[255];
        u8 msgSize = 255;
        
        robot.getOutgoingMessage(msgData, msgSize);
        if(msgSize > 0) {
          tx[i]->send(msgData, msgSize);
        }
        
      } // while robot i still has outgoing messages to send
      
    } // for each robot
    
    // Update the world (force robots to process their messages)
    blockWorld.update();
    
    // Loop over all the blocks in the world and update the display in Webots
    for(SimBlock& webotBlock : simBlocks)
    {
      Anki::Pose3d blockPose( getNodePose(webotBlock.node) );
      
      // Find closest block in blockWorld
      float minDist = -1.f;
      
      webotBlock.bwBlock = NULL;
      
      const std::vector<Anki::Cozmo::Block>& worldBlocks = blockWorld.get_blocks(webotBlock.type);
      for(auto worldBlockIter = worldBlocks.begin();
          worldBlockIter != worldBlocks.end(); ++worldBlockIter)
      {
        const float d = Anki::computeDistanceBetween(blockPose, worldBlockIter->get_pose());
        
        if((webotBlock.bwBlock == NULL || d < minDist) ) // && d < worldBlockIter->get_minDim())
        {
          minDist = d;
          webotBlock.bwBlock = &(*worldBlockIter);
        }
      } // for each blockWorld block of this type
      
      if(webotBlock.bwBlock == NULL) {
        // No block in the world found to match this simulated block
        // If this block was selected, unselect it.
        webotBlock.selected = false;
      }
      else {
        webotBlock.updatePose(blockPose);
      }
      
      webotBlock.draw();
      
    } // for each webot block
    
   
    ProcessKeystroke(blockWorld, simBlocks);
    
  } // while still stepping

  //delete msgInterface;
  
  return 0;
}

// TODO: this should probably get moved out of this simulator-specific file
int processPacket(const unsigned char *data, const int dataSize,
                  Anki::Cozmo::BlockWorld& blockWorld, int i_robot)
{
  
  if(dataSize > 4) {
    
    if(data[0] == COZMO_WORLD_MSG_HEADER_BYTE_1 &&
       data[1] == COZMO_WORLD_MSG_HEADER_BYTE_2 &&
       data[2] == i_robot+1)
    {
      //fprintf(stdout, "Valid 0xBEEF message header from robot %d received.\n", i_robot+1);
      
      // First byte in the msgBuffer should be the type of message
      Messages::ID msgID = static_cast<Messages::ID>(data[3]);
      
      // The next byte should be the first byte of the message struct that
      // was sent and will contain the size of the message.
      const u8 msgSize = MessageTable[msgID].size;
      
      if(dataSize < msgSize) {
        fprintf(stdout, "Valid header, but less data than expected in packet.\n");
        return EXIT_FAILURE;
      }
      else if(msgSize == 0) {
        fprintf(stdout, "Valid header, but zero-sized message received. (?) Ignoring.\n");
        return EXIT_FAILURE;
      }
      else {
        
        // TODO: Update this to use dispatch functions instead of switch statement
        switch(msgID)
        {
          case RobotAvailable_ID:
          {
            //const CozmoMsg_RobotAvailable* msgIn = reinterpret_cast<const CozmoMsg_RobotAvailable*>(data+3);
            
            // Create a new robot
            blockWorld.addRobot(i_robot);
            
            break;
          }
            
          case BlockMarkerObserved_ID:
          case MatMarkerObserved_ID:
          case HeadCameraCalibration_ID:
          case MatCameraCalibration_ID:
          {
            fprintf(stdout, "Passing block/mat/calib message to robot.\n");
            
            // Pass these right along to the robot object to process:
            blockWorld.getRobot(i_robot).queueIncomingMessage(data+3, msgSize+1);
            
            break;
          }
          
          default:
          {
            fprintf(stdout, "Unknown message type received: %d\n", msgID);
            return EXIT_FAILURE;
          }
        } // switch(cmd)
      }
      
    } else {
      fprintf(stdout, "Data packet too small to be a valid message.\n");
      return EXIT_FAILURE;
      
    } // if/else valid message header
    
  } // if at least 4 bytes in receive queue

  return EXIT_SUCCESS;
  
} // processPacket()

//Check the keyboard keys and issue robot commands
void ProcessKeystroke(Anki::Cozmo::BlockWorld&  blockWorld,
                      std::vector<SimBlock>&    simBlocks)
{
  static size_t selectedRobot = 0;
  static std::vector<SimBlock>::iterator selectedBlock = simBlocks.begin();
/*
  while(selectedBlock->bwBlock==NULL && selectedBlock != simBlocks.end()) {
    ++selectedBlock;
  }
  */
  //Why do some of those not match ASCII codes?
  //Numbers, spacebar etc. work, letters are different, why?
  //a, z, s, x, Space
  const s32 CKEY_DOCK       = static_cast<s32>('D');
  const s32 CKEY_SELECT_0   = static_cast<s32>('1');
  const s32 CKEY_SELECT_1   = static_cast<s32>('2');
  const s32 CKEY_SELECT_2   = static_cast<s32>('3');
  const s32 CKEY_SELECT_3   = static_cast<s32>('4');
  const s32 CKEY_SELECT_4   = static_cast<s32>('5');
  
  const s32 CKEY_SELECT_PREV_BLOCK = static_cast<s32>('[');
  const s32 CKEY_SELECT_NEXT_BLOCK = static_cast<s32>(']');
  
  const s32 key = commsController.keyboardGetKey();
  
  switch (key)
  {
    case CKEY_DOCK:
    {
      if(selectedBlock != simBlocks.end()) {
        fprintf(stdout, "Commanding selected robot to dock.\n");
        blockWorld.commandRobotToDock(selectedRobot, *(selectedBlock->bwBlock));
      }
      break;
    }
    case CKEY_SELECT_0:
    {
      fprintf(stdout, "Selecting robot 0.\n");
      selectedRobot = 0;
      break;
    }
    case CKEY_SELECT_1:
    {
      fprintf(stdout, "Selecting robot 1.\n");
      selectedRobot = 1;
      break;
    }
    case CKEY_SELECT_2:
    {
      fprintf(stdout, "Selecting robot 2.\n");
      selectedRobot = 2;
      break;
    }
    case CKEY_SELECT_3:
    {
      fprintf(stdout, "Selecting robot 3.\n");
      selectedRobot = 3;
      break;
    }
    case CKEY_SELECT_4:
    {
      fprintf(stdout, "Selecting robot 4.\n");
      selectedRobot = 4;
      break;
    }
    case CKEY_SELECT_NEXT_BLOCK:
    {
      // Unselect current block
      selectedBlock->selected = false;
      selectedBlock->draw();

      // Move selector to next block with a correpsonding BlockWorld block
      do {
        ++selectedBlock;
        if(selectedBlock == simBlocks.end()) {
          selectedBlock = simBlocks.begin();
        }
      } while(selectedBlock->bwBlock == NULL);
      
      // Draw newly-selected block
      selectedBlock->selected = true;
      selectedBlock->draw();
      break;
    }
    case CKEY_SELECT_PREV_BLOCK:
    {
      // Unselect current block
      selectedBlock->selected = false;
      selectedBlock->draw();
      
      // Move selector to next block with a correpsonding BlockWorld block
      do {
        if(selectedBlock == simBlocks.begin()) {
          selectedBlock = simBlocks.end();
        }
        --selectedBlock;
      } while(selectedBlock->bwBlock == NULL);
      
      // Draw newly-selected block
      selectedBlock->selected = true;
      selectedBlock->draw();
      break;
    }
      
      
  } // switch(key)
  
} // ProcessKeyStroke()

void initWorldNodes(std::map<std::string, webots::Node*>&  nameToNodeLUT,
                    std::vector<SimBlock>&                 simBlocks,
                    std::vector<webots::Node*>&            robotNodes)
{
  
  webots::Node* rootNode = commsController.getRoot();
  
  if(rootNode == NULL) {
    fprintf(stdout, "Root node not found -- is the supervisor node initialized?\n");
    CORETECH_ASSERT(false);
  }
  
  webots::Field* sceneNodes = rootNode->getField("children");
  int numSceneNodes = sceneNodes->getCount();
  
  for(int i_node = 0; i_node < numSceneNodes; ++i_node)
  {
    webots::Node* sceneObject = sceneNodes->getMFNode(i_node);
    webots::Field* nameField = sceneObject->getField("name");
    
    if(nameField != NULL) {
      const char *objName = nameField->getSFString().c_str();
      
      if(objName != NULL) {
        
        if(!strncmp(objName, "Block", 5) && strncmp(objName, "BlockWorld", 10)) {
          
          const char blockNumStr[4] = {objName[5], objName[6], objName[7], '\0'};
          Anki::Cozmo::BlockType blockID = static_cast<Anki::Cozmo::BlockType>(atoi(blockNumStr));
          
          CORETECH_ASSERT(blockID > 0 && blockID <= Anki::Cozmo::BlockWorld::MaxBlockTypes);
          
          simBlocks.emplace_back(sceneObject, blockID);
          
          std::string objNameStr(objName);
          if(nameToNodeLUT.count(objNameStr) > 0) {
            fprintf(stdout, "Duplicate object named '%s' found in node tree!", objName);
          } else {
            nameToNodeLUT[objNameStr] = sceneObject;
          }
        }
        else if(!strncmp(objName, "Cozmo", 5)) {
          
          robotNodes.push_back(sceneObject);
          
          std::string objNameStr(objName);
          
          if(nameToNodeLUT.count(objNameStr) > 0) {
            fprintf(stdout, "Duplicate object named '%s' found in node tree!", objName);
          } else {
            nameToNodeLUT[objNameStr] = sceneObject;
          }
        }
        
      } // if objName not NULL
    } // if nameField not NULL
  } // for each scene node
  
} // initWorldNodes()

Anki::Pose3d getNodePose(webots::Node* node)
{
  if(node == NULL) {
    fprintf(stdout, "Cannot GetNodePose for an empty name/node.");
    CORETECH_ASSERT(false);
  }
  
  webots::Field* rotField = node->getField("rotation");
  if(rotField == NULL) {
    fprintf(stdout, "Could not find 'rotation' field for node.\n");
    CORETECH_ASSERT(false);
  }
  const double *rotation = rotField->getSFRotation();
  
  webots::Field* transField = node->getField("translation");
  if(transField == NULL) {
    fprintf(stdout, "Could not find 'translation' field for node.\n");
    CORETECH_ASSERT(false);
  }
  const double *translation = transField->getSFVec3f();
  
  // Note the coordinate change here: Webot has y pointing up,
  // while Matlab has z pointing up.  That swap induces a
  // right-hand to left-hand coordinate change (I think).  Also,
  // things in Matlab are defined in millimeters, while Webot is
  // in meters.
  Anki::Radians rotAngle(rotation[3]);
  Anki::Vec3f rotAxis(-static_cast<float>(rotation[0]),
                      static_cast<float>(rotation[2]),
                      static_cast<float>(rotation[1]));
  Anki::Vec3f transVec(-static_cast<float>(translation[0]),
                       static_cast<float>(translation[2]),
                       static_cast<float>(translation[1]));
  transVec *= 1000.f;
  
  return Anki::Pose3d(rotAngle, rotAxis, transVec);
  
} // GetNodePose()

// TODO: Define these elsewhere and then actually use them in handling messages above
void Anki::Cozmo::CommandHandler::ProcessClearPathMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessSetMotionMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessRobotAvailableMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessMatMarkerObservedMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessRobotAddedToWorldMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessSetPathSegmentArcMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessDockingErrorSignalMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessSetPathSegmentLineMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessBlockMarkerObservedMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessTemplateInitializedMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessTotalBlocksDetectedMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessMatCameraCalibrationMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessAbsLocalizationUpdateMessage(unsigned char const*) {}

void Anki::Cozmo::CommandHandler::ProcessHeadCameraCalibrationMessage(unsigned char const*) {}
