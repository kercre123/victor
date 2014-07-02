/*
 * File:          robot_advertisement_controller.cpp
 * Date:
 * Description:   A standalone service which simluated robots can register or deregister with to emulate
 *                BTLE advertising (UDP port = ROBOT_ADVERTISEMENT_REGISTRATION_PORT). 
 *                Any basestation (simulated or real) using TCP comms to emulate BTLE can connect to this 
 *                service (UDP port = ROBOT_ADVERTISING_PORT) to see which robots are available.
 *
 *                Advertising message type is RobotAdvertisement.
 *                Advertisement registration message is RobotAdvertisementRegistration.
 *
 * Author:        
 * Modifications: 
 */

#include <iostream>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <map>
#include "anki/messaging/shared/UdpServer.h"
#include "anki/cozmo/robot/cozmoConfig.h"

#include <webots/Supervisor.hpp>


// Physical wifi robots do not yet register/deregister with advertising service so just
// hard-coding its connection
#define FORCE_ADD_ROBOT 0
//const u8 forcedRobotId = 34;
const char* forcedRobotIP = "192.168.3.34";

using namespace std;

/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */
int main(int argc, char **argv)
{
  using namespace Anki::Cozmo;

  printf("Starting advertisement service\n");

  
  
  UdpServer regServer;
  UdpServer advertisingServer;
  
  // Map of robot id to RobotAdvertisement message
  typedef map<int, RobotAdvertisement>::iterator robotConnectionInfoMapIt;
  map<int, RobotAdvertisement> robotConnectionInfoMap;

  // Main webots controller
  webots::Supervisor advertisementController;

  
  // Start listening for clients that want to
  regServer.StartListening(ROBOT_ADVERTISEMENT_REGISTRATION_PORT);
  advertisingServer.StartListening(ROBOT_ADVERTISING_PORT);

  // Message from robots that want to (de)register for advertising.
  RobotAdvertisementRegistration regMsg;
  
  robotConnectionInfoMapIt it;
  
  double lastAdvertisingTime = 0;
  
  
#if(FORCE_ADD_ROBOT)
  // Force add physical robot since it's not registering by itself yet.
  robotConnectionInfoMap[forcedRobotId].robotID = forcedRobotId;
  robotConnectionInfoMap[forcedRobotId].port = ROBOT_RADIO_BASE_PORT;
  snprintf((char*)robotConnectionInfoMap[forcedRobotId].robotAddr,
           sizeof(robotConnectionInfoMap[forcedRobotId].robotAddr),
           "%s", forcedRobotIP);
#endif
  
  
  while(advertisementController.step(TIME_STEP) != -1) {
  //while(1) {
    
    // Update registered robots
    int bytes_recvd = 0;
    do {
      // For now, assume that only one kind of message comes in
      bytes_recvd = regServer.Recv((char*)&regMsg, sizeof(regMsg));
  
      if (bytes_recvd == sizeof(regMsg)) {
        if (regMsg.enableAdvertisement) {
          std::cout << "Registering robot " << (int)regMsg.robotID
                    << " on host " << regMsg.robotAddr
                    << " at port " << regMsg.port
                    << " with advertisement service\n";
          
          robotConnectionInfoMap[regMsg.robotID].robotID = regMsg.robotID;
          robotConnectionInfoMap[regMsg.robotID].port = regMsg.port;
          memcpy(robotConnectionInfoMap[regMsg.robotID].robotAddr, regMsg.robotAddr, sizeof(RobotAdvertisement::robotAddr));
          
        } else {
          std::cout << "Deregistering robot " << (int)regMsg.robotID << " from advertisement service\n";
          robotConnectionInfoMap.erase(regMsg.robotID);
        }
      } else if (bytes_recvd > 0){
        printf("Recvd datagram with %d bytes. Expecting %d bytes.\n", bytes_recvd, (int)sizeof(regMsg));
      }
      
    } while (bytes_recvd > 0);

    
    // Get clients that are interested in advertising robots
    do {
      bytes_recvd = advertisingServer.Recv((char*)&regMsg, sizeof(regMsg));  //NOTE: Don't actually expect to get RobotAdvertisementRegistration message here, but just need something to put stuff in. Server automatically adds client to internal list when recv is called.
      
      if (bytes_recvd > 0) std::cout << "Received ping from basestation\n";
    } while(bytes_recvd > 0);
    
    
    // Notify all clients
    if (advertisementController.getTime() - lastAdvertisingTime > ROBOT_ADVERTISING_PERIOD_S) {
      if (advertisingServer.GetNumClients() > 0 && !robotConnectionInfoMap.empty())
#if(FORCE_ADD_ROBOT == 0)
        std::cout << "Notifying " <<  advertisingServer.GetNumClients() << " clients of advertising robots\n";
#endif
      
      for (it = robotConnectionInfoMap.begin(); it != robotConnectionInfoMap.end(); it++) {
        //std::cout << "Advertising: Robot " << it->second.robotID
        //          << " on host " << it->second.robotAddr
        //          << " at port " << it->second.port
        //          << "(size=" << sizeof(RobotAdvertisement) << ")\n";
        advertisingServer.Send((char*)&(it->second), sizeof(RobotAdvertisement));
        
        lastAdvertisingTime = advertisementController.getTime();
      }
    }
    
   //usleep(100000);
  }
  
  
  
  printf("Shutting down advertisement service\n");
  return 0;
}
