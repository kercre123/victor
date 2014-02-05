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

#include "anki/cozmo/robot/cozmoConfig.h"

#include "anki/cozmo/basestation/tcpComms.h"

// TODO: Get rid of this once we're sure it's working with TCP stuff
#define USE_WEBOTS_TXRX 0

webots::Supervisor commsController;


//
// main()
//

using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  const int MAX_ROBOTS = Anki::Cozmo::BlockWorld::MaxRobots;
  
  // Comms
  Anki::Cozmo::TCPComms robotComms;
  MessageHandler::getInstance()->Init(&robotComms);
  
  
#if(USE_WEBOTS_TXRX)
  webots::Receiver* rx[MAX_ROBOTS];
  webots::Emitter*  tx[MAX_ROBOTS];
#endif
  commsController.keyboardEnable(Anki::Cozmo::TIME_STEP);


  
#if(USE_WEBOTS_TXRX)
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
#endif
  
  
  
  //
  // Main Execution loop: step the world forward forever
  //
  while (commsController.step(Anki::Cozmo::TIME_STEP) != -1)
  {
    // Update time
    // (To be done from iOS eventually)
    Anki::BaseStationTimer::getInstance()->UpdateTime(SEC_TO_NANOS(commsController.getTime()));
    
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
          RobotManager::getInstance()->AddRobot(robotID, *BlockWorld::getInstance());
        }
      }
    }
    
    // If we still don't have any connected robots, don't proceed
    if (robotComms.GetNumConnectedRobots() == 0) {
      continue;
    }

    MessageHandler::getInstance()->ProcessMessages();
    
    //
    // Check for any outgoing messages from each basestation robot:
    //
    for(auto robotiD : RobotManager::getInstance()->GetRobotIDList())
    {
      Anki::Cozmo::Robot* robot = RobotManager::getInstance()->GetRobotByID(robotiD);
      while(robot->hasOutgoingMessages())
      {
        
#if(USE_WEBOTS_TXRX)
        // Buffer for the message data we're going to send:
        // (getOutgoingMessage() will copy data into it)
        unsigned char msgData[255];
        u8 msgSize = 255;
        
        robot.getOutgoingMessage(msgData, msgSize);
        if(msgSize > 0) {
          tx[i]->send(msgData, msgSize);
        }
#else
        Anki::Comms::MsgPacket p;
        p.destId = robot->get_ID();
        robot->getOutgoingMessage(p.data, p.dataLen);
        if (p.dataLen > 0) {
          robotComms.Send(p);
        }
#endif
      } // while robot i still has outgoing messages to send
      
    } // for each robot
    
    // Update the world (force robots to process their messages)
    BlockWorld::getInstance()->Update();
    
  } // while still stepping

  //delete msgInterface;
  
  return 0;
}

