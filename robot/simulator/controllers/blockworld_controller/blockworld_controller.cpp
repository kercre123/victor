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
#endif

#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/robot.h"
//#include "anki/messaging/basestation/messagingInterface.h"

#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/cozmo/messageProtocol.h"

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
