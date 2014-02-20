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
  
  // Map of robot id to port number
  typedef map<int, int>::iterator robotPortMapIt;
  map<int, int> robotPortMap;

  // Main webots controller
  webots::Supervisor advertisementController;

  
  // Start listening for clients that want to
  regServer.StartListening(ROBOT_ADVERTISEMENT_REGISTRATION_PORT);
  advertisingServer.StartListening(ROBOT_ADVERTISING_PORT);

  // Message from robots that want to (de)register for advertising.
  RobotAdvertisementRegistration regMsg;
  
  // Message to send to client basestations that want to know what robots are advertising.
  RobotAdvertisement advMsg;
  
  robotPortMapIt it;
  
  double lastAdvertisingTime = 0;
  
  while(advertisementController.step(TIME_STEP) != -1) {
  //while(1) {
    
    // Update registered robots
    int bytes_recvd = 0;
    do {
      // For now, assume that only one kind of message comes in
      bytes_recvd = regServer.Recv((char*)&regMsg, sizeof(regMsg));
  
      if (bytes_recvd == sizeof(regMsg)) {
        if (regMsg.enableAdvertisement) {
          std::cout << "Registering robot " << (int)regMsg.robotID << " on port " << regMsg.port << " with advertisement service\n";
          robotPortMap[regMsg.robotID] = regMsg.port;
        } else {
          std::cout << "Deregistering robot " << (int)regMsg.robotID << " from advertisement service\n";
          robotPortMap.erase(regMsg.robotID);
        }
      }
      
    } while (bytes_recvd > 0);

    
    // Get clients that are interested in advertising robots
    do {
      bytes_recvd = advertisingServer.Recv((char*)&regMsg, sizeof(regMsg));  //NOTE: Don't actually expect to get RobotAdvertisementRegistration message here, but just need something to put stuff in. Server automatically adds client to internal list when recv is called.
      
      if (bytes_recvd > 0) std::cout << "Received ping from robot\n";
    } while(bytes_recvd > 0);
    
    
    // Notify all clients
    if (advertisementController.getTime() - lastAdvertisingTime > ROBOT_ADVERTISING_PERIOD_S) {
      if (advertisingServer.GetNumClients() > 0 && !robotPortMap.empty())
        std::cout << "Notifying " <<  advertisingServer.GetNumClients() << " clients of advertising robots\n";
      
      for (it = robotPortMap.begin(); it != robotPortMap.end(); it++) {
        advMsg.robotID = it->first;
        advMsg.port = it->second;
        advertisingServer.Send((char*)&advMsg, sizeof(advMsg));
        
        lastAdvertisingTime = advertisementController.getTime();
      }
    }
    
   //usleep(100000);
  }
  
  
  
  printf("Shutting down advertisement service\n");
  return 0;
}
