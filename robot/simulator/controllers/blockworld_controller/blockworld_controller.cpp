/*
 * File:          world_comms_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include <cstdio>
#include <queue>

// Set to zero to use the regular C interface, which we may need to avoid
// difficulties linking against our libraries which C++11 standard libs, unlike
// the webots CppController lib, which appears to be linked against an older
// version of the standard libs.
#define USE_WEBOTS_CPP_INTERFACE 0

//#include "CozmoWorldComms.h"

#if USE_WEBOTS_CPP_INTERFACE
#include <webots/Supervisor.hpp>
#else
#include <webots/emitter.h>
#include <webots/receiver.h>
#include <webots/robot.h>
#include <webots/supervisor.h>
#endif

#include "anki/common/basestation/math/pose.h"

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
//#include "anki/messaging/basestation/messagingInterface.h"

#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/cozmo/messageProtocol.h"

Anki::Pose3d getNodePose(WbNodeRef node)
{
  
  if(node == NULL) {
    fprintf(stdout, "Cannot GetNodePose for an empty name/node.");
    CORETECH_ASSERT(false);
  }
  
  WbFieldRef rotField = wb_supervisor_node_get_field(node, "rotation");
  if(rotField == NULL) {
    fprintf(stdout, "Could not find 'rotation' field for node.\n");
    CORETECH_ASSERT(false);
  }
  const double *rotation = wb_supervisor_field_get_sf_rotation(rotField);
  
  WbFieldRef transField = wb_supervisor_node_get_field(node, "translation");
  if(transField == NULL) {
    fprintf(stdout, "Could not find 'translation' field for node.\n");
    CORETECH_ASSERT(false);
  }
  const double *translation = wb_supervisor_field_get_sf_vec3f(transField);
  
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


// Populate lists of block/robot nodes for later use
void initWorldNodes(std::map<std::string, WbNodeRef>&       nameToNodeLUT,
                    std::vector< std::vector<WbNodeRef> >&  blockNodes,
                    std::vector<WbNodeRef>&                 robotNodes)
{
  blockNodes.resize(Anki::Cozmo::BlockWorld::MaxBlockTypes);
  
  WbNodeRef rootNode = wb_supervisor_node_get_root();
  if(rootNode == NULL) {
    fprintf(stdout, "Root node not found -- is the supervisor node initialized?\n");
    CORETECH_ASSERT(false);
  }
  
  WbFieldRef sceneNodes = wb_supervisor_node_get_field(rootNode, "children");
  
  int numSceneNodes = wb_supervisor_field_get_count(sceneNodes);

  for(int i_node = 0; i_node < numSceneNodes; ++i_node)
  {
    WbNodeRef sceneObject = wb_supervisor_field_get_mf_node(sceneNodes, i_node);
    WbFieldRef nameField = wb_supervisor_node_get_field(sceneObject, "name");
    
    if(nameField != NULL) {
      const char *objName = wb_supervisor_field_get_sf_string(nameField);

      if(objName != NULL) {
        
        if(!strncmp(objName, "Block", 5) && strncmp(objName, "BlockWorld", 10)) {
          
          const char blockNumStr[4] = {objName[5], objName[6], objName[7], '\0'};
          int blockID = atoi(blockNumStr);
          
          CORETECH_ASSERT(blockID > 0 && blockID <= Anki::Cozmo::BlockWorld::MaxBlockTypes);

          blockNodes[blockID].push_back(sceneObject);
          
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


int processPacket(const unsigned char *data, const int dataSize,
                  Anki::Cozmo::BlockWorld& blockWorld, int i_robot);


int main(int argc, char **argv)
{
  //Anki::MessagingInterface* msgInterface = new Anki::MessagingInterface_TCP();
  
  Anki::Cozmo::BlockWorld::ZAxisPointsUp = false; // b/c this is Webots
  
  Anki::Cozmo::BlockWorld blockWorld;
  
  const int MAX_ROBOTS = Anki::Cozmo::BlockWorld::MaxRobots;
  
#if USE_WEBOTS_CPP_INTERFACE
  webots::Supervisor commsController;
  webots::Receiver* rx[MAX_ROBOTS];
  webots::Emitter*  tx[MAX_ROBOTS];
#else
  wb_robot_init();
  WbDeviceTag rx[MAX_ROBOTS];
  WbDeviceTag tx[MAX_ROBOTS];
#endif
  
  
  //
  // Initialize the node lists for the world this controller lives in
  //  (this needs to happen after wb_robot_init!)
  //
  std::map<std::string, WbNodeRef>       nameToNodeLUT;
  std::vector< std::vector<WbNodeRef> >  blockNodes;
  std::vector<WbNodeRef>                 robotNodes;
  initWorldNodes(nameToNodeLUT, blockNodes, robotNodes);
  
  size_t numBlocks = 0;
  for(auto blockList : blockNodes) {
    numBlocks += blockList.size();
  }
  fprintf(stdout, "Found %lu robots and %lu blocks in the world.\n",
          numBlocks, robotNodes.size());

  
  
  //
  // Initialize World Transmitters/Receivers
  // (one for each robot, up to MAX_ROBOTS)
  //
  char rxRadioName[12], txRadioName[12];
  
  for(int i=0; i<MAX_ROBOTS; ++i) {
    snprintf(rxRadioName, 11, "radio_rx_%d", i+1);
    snprintf(txRadioName, 11, "radio_tx_%d", i+1);
    
#if USE_WEBOTS_CPP_INTERFACE
    tx[i] = commsController.getEmitter(txRadioName);
    rx[i] = commsController.getReceiver(rxRadioName);
    rx[i]->enable(Anki::Cozmo::TIME_STEP);
#else
    tx[i] = wb_robot_get_device(txRadioName);
    rx[i] = wb_robot_get_device(rxRadioName);
    wb_receiver_enable(rx[i], Anki::Cozmo::TIME_STEP);
#endif
  }
 
  //
  // Main Execution loop: step the world forward forever
  //
#if USE_WEBOTS_CPP_INTERFACE
  while (commsController.step(Anki::Cozmo::TIME_STEP) != -1)
#else
  while(wb_robot_step(Anki::Cozmo::TIME_STEP) != -1)
#endif
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
#if USE_WEBOTS_CPP_INTERFACE
      while (rx[i]->getQueueLength() > 0)
      {
        // Get head packet
        data = (unsigned char *)rx[i]->getData();
        dataSize = rx[i]->getDataSize();
        
        processPacket(data, dataSize, blockWorld, i);
        
        rx[i]->nextPacket();
        
      } // while receiver queue not empty
#else
      while(wb_receiver_get_queue_length(rx[i]) > 0)
      {
        // Get head packet
        data = (unsigned char *)wb_receiver_get_data(rx[i]);
        dataSize = wb_receiver_get_data_size(rx[i]);
        
        processPacket(data, dataSize, blockWorld, i);
        
        wb_receiver_next_packet(rx[i]);
        
      } // while receiver queue not empty
      
#endif // USE_WEBOTS_CPP_INTERFACE
      
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
          wb_emitter_send(tx[i], msgData, msgSize);
        }
        
      } // while robot i still has outgoing messages to send
      
    } // for each robot
    
    // Update the world (force robots to process their messages)
    blockWorld.update();
    
    // Loop over all the blocks in the world and update the display in Webots
    for(size_t ofType = 0; ofType < blockNodes.size(); ++ofType)
    {
      if(not blockNodes[ofType].empty()) {
        
        for(size_t i_block = 0; i_block < blockNodes[ofType].size(); ++i_block)
        {
          WbNodeRef webotBlock = blockNodes[ofType][i_block];
          Anki::Pose3d blockPose( getNodePose(webotBlock) );
          
          // Find closest block in blockWorld
          float minDist = -1.f;
          //std::vector<Anki::Cozmo::Block>::const_iterator closest;
          const Anki::Cozmo::Block* closest = NULL;
        
          const std::vector<Anki::Cozmo::Block>& worldBlocks = blockWorld.get_blocks(ofType);
          for(auto worldBlockIter = worldBlocks.begin();
              worldBlockIter != worldBlocks.end(); ++worldBlockIter)
          {
            const float d = Anki::computeDistanceBetween(blockPose, worldBlockIter->get_pose());
          
            if((closest == NULL || d < minDist) ) // && d < worldBlockIter->get_minDim())
            {
              minDist = d;
              closest = &(*worldBlockIter);
            }
          } // for each blockWorld block of this type
          
          if(closest != NULL) {
            
            // Update the closest block's observation node with the specified pose
            WbFieldRef rotField = wb_supervisor_node_get_field(webotBlock, "obsRotation");
            if(rotField == NULL) {
              fprintf(stdout,
                      "Could not find 'obsRotation' field for block %zu of type %zu.",
                      i_block, ofType);
              
              CORETECH_ASSERT(false);
            }
            
            WbFieldRef translationField = wb_supervisor_node_get_field(webotBlock, "obsTranslation");
            if(translationField == NULL) {
                   fprintf(stdout, "Could not find 'obsTranslation' field for block %zu of type %zu.", i_block, ofType);
              CORETECH_ASSERT(false);
            }
            
            WbFieldRef transparencyField = wb_supervisor_node_get_field(webotBlock, "obsTransparency");
            if(transparencyField == NULL) {
                   fprintf(stdout, "Could not find 'obsTransparency' field for block %zu of type %zu.", i_block, ofType);
              CORETECH_ASSERT(false);
            }
            

            // The observation is a child of the block in Webots, so its pose is w.r.t.
            // the block's pose.  But the incoming observed pose is w.r.t. the world.
            // Adjust accordingly before updating the observed node's pose for display
            // in Webots:
            Anki::Pose3d obsPose( blockPose.getInverse() * closest->get_pose() );
            
            Anki::Vec3f   rotAxis( obsPose.get_rotationAxis() );
            Anki::Radians rotAngle(obsPose.get_rotationAngle());
            
            // Convert to Webots coordinate system:
            const double webotRotation[4] = {
              static_cast<double>(-rotAxis.x()),
              static_cast<double>( rotAxis.z()),
              static_cast<double>( rotAxis.y()),
              rotAngle.ToDouble()
            };
            
            wb_supervisor_field_set_sf_rotation(rotField, webotRotation);
            
            Anki::Vec3f translation(obsPose.get_translation());
            translation *= 1.f/1000.f; // convert from mm to meters
            const double webotTranslation[3] = {
              static_cast<double>(-translation.x()),
              static_cast<double>( translation.z()),
              static_cast<double>( translation.y())
            };
            
            wb_supervisor_field_set_sf_vec3f(translationField, webotTranslation);
            
            wb_supervisor_field_set_sf_float(transparencyField, 0.);
            
          } // if closest block found (not NULL)
          
        } // for each webot block of this type
        
      } // if there are any blocks nodes of this type
    } // for each block type
   
    
  } // while still stepping

  //delete msgInterface;
  
#if !USE_WEBOTS_CPP_INTERFACE
  wb_robot_cleanup();
#endif
  
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
      
      // The next byte should be the first byte of the message struct that
      // was sent and will contain the size of the message.
      const u8 msgSize = data[3];
      
      if(dataSize < msgSize) {
        fprintf(stdout, "Valid header, but less data than expected in packet.\n");
        return EXIT_FAILURE;
      }
      else if(msgSize == 0) {
        fprintf(stdout, "Valid header, but zero-sized message received. (?) Ignoring.\n");
        return EXIT_FAILURE;
      }
      else {
        
        // First byte in the msgBuffer should be the type of message
        CozmoMsg_Command cmd = static_cast<CozmoMsg_Command>(data[4]);
        
        switch(cmd)
        {
          case MSG_V2B_CORE_ROBOT_AVAILABLE:
          {
            const CozmoMsg_RobotAvailable* msgIn = reinterpret_cast<const CozmoMsg_RobotAvailable*>(data+3);
            
            // Create a new robot
            blockWorld.addRobot(i_robot);
            
            break;
          }
            
          case MSG_V2B_CORE_BLOCK_MARKER_OBSERVED:
          case MSG_V2B_CORE_MAT_MARKER_OBSERVED:
          case MSG_V2B_CORE_HEAD_CAMERA_CALIBRATION:
          case MSG_V2B_CORE_MAT_CAMERA_CALIBRATION:
          {
            fprintf(stdout, "Passing block/mat/calib message to robot.\n");
            
            // Pass these right along to the robot object to process:
            blockWorld.getRobot(i_robot).queueIncomingMessage(data+3, msgSize+1);
            
            break;
          }
          
          default:
          {
            fprintf(stdout, "Unknown message type received: %d\n", cmd);
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
