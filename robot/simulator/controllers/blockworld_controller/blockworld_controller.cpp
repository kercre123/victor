/*
 * File:          world_comms_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include <cstdio>
#include <queue>

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
                  std::vector<Anki::Cozmo::Robot> &robots, int i_robot);


int main(int argc, char **argv)
{
  //Anki::MessagingInterface* msgInterface = new Anki::MessagingInterface_TCP();
  
  Anki::Cozmo::BlockWorld blockWorld;
  //CozmoWorldComms comms;
  //comms.Init();
  //comms.run();
  
  std::vector<Anki::Cozmo::Robot> robots(1);

  const int MAX_ROBOTS = 4;
  
#if USE_WEBOTS_CPP_INTERFACE
  webots::Supervisor commsController;
  webots::Receiver* rx[MAX_ROBOTS];
  webots::Emitter*  tx[MAX_ROBOTS];
#else
  wb_robot_init();
  WbDeviceTag rx[MAX_ROBOTS];
  WbDeviceTag tx[MAX_ROBOTS];
#endif
  
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
  
#if USE_WEBOTS_CPP_INTERFACE
  while (commsController.step(Anki::Cozmo::TIME_STEP) != -1)
#else
  while(wb_robot_step(Anki::Cozmo::TIME_STEP) != -1)
#endif
  {
    // Receive messages:
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
        
        processPacket(data, dataSize, robots, i);
        
        rx[i]->nextPacket();
        
      } // while receiver queue not empty
#else
      while(wb_receiver_get_queue_length(rx[i]) > 0)
      {
        // Get head packet
        data = (unsigned char *)wb_receiver_get_data(rx[i]);
        dataSize = wb_receiver_get_data_size(rx[i]);
        
        processPacket(data, dataSize, robots, i);
        
        wb_receiver_next_packet(rx[i]);
        
      } // while receiver queue not empty
      
#endif // USE_WEBOTS_CPP_INTERFACE
      
    } // for each receiver
    
  } // while still stepping the commsController

  //delete msgInterface;
  
  return 0;
}


int processPacket(const unsigned char *data, const int dataSize,
                  std::vector<Anki::Cozmo::Robot> &robots, int i_robot)
{
  
  if(dataSize > 4) {
    
    if(data[0] == COZMO_WORLD_MSG_HEADER_BYTE_1 &&
       data[1] == COZMO_WORLD_MSG_HEADER_BYTE_2 &&
       data[2] == i_robot+1)
    {
      fprintf(stdout, "Valid 0xBEEF message header from robot %d received.\n", i_robot+1);
      
      // The next byte should be the first byte of the message struct that
      // was sent and will contain the size of the message.
      const u8 msgSize = data[3];
      
      if(dataSize < msgSize) {
        fprintf(stdout, "Less data than expected in packet.\n");
        return EXIT_FAILURE;
      }
      else {
        
        // First byte in the msgBuffer should be the type of message
        CozmoMsg_Command cmd = static_cast<CozmoMsg_Command>(data[4]);
        
        switch(cmd)
        {
          case MSG_V2B_CORE_BLOCK_MARKER_OBSERVED:
          case MSG_V2B_CORE_MAT_MARKER_OBSERVED:
          {
            // Pass these right along to the robot object:
            robots[i_robot].queueMessage(data+3, msgSize);
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
