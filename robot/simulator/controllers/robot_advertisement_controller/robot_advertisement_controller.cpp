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

#include <stdio.h>

#include "anki/messaging/basestation/advertisementService.h"
#include "anki/cozmo/robot/cozmoConfig.h"

#include <webots/Supervisor.hpp>


// Physical wifi robots do not yet register/deregister with advertising service so just
// hard-coding its connection
#define FORCE_ADD_ROBOT 0
const u8 forcedRobotId = 1;
const char* forcedRobotIP = "192.168.3.34";

const int ADVERTISEMENT_TIME_STEP_MS = 60;

/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */
int main(int argc, char **argv)
{
  using namespace Anki::Comms;
  using namespace Anki::Cozmo;

  printf("Starting advertisement service\n");
  
  AdvertisementService robotAdService("RobotAdvertisementService");
  robotAdService.StartService(ROBOT_ADVERTISEMENT_REGISTRATION_PORT, ROBOT_ADVERTISING_PORT);

  AdvertisementService uiAdService("UIAdvertisementService");
  uiAdService.StartService(UI_ADVERTISEMENT_REGISTRATION_PORT, UI_ADVERTISING_PORT);
  

  // Main webots controller
  webots::Supervisor advertisementController;

  
#if(FORCE_ADD_ROBOT)
  // Force add physical robot since it's not registering by itself yet.
  AdvertisementRegistrationMsg forcedRegistrationMsg;
  forcedRegistrationMsg.id = forcedRobotId;
  forcedRegistrationMsg.port = ROBOT_RADIO_BASE_PORT;
  forcedRegistrationMsg.protocol = USE_UDP_ROBOT_COMMS == 1 ? Anki::Comms::UDP : Anki::Comms::TCP;
  forcedRegistrationMsg.enableAdvertisement = 1;
  snprintf((char*)forcedRegistrationMsg.ip, sizeof(forcedRegistrationMsg.ip), "%s", forcedRobotIP);
  
  robotAdService.ProcessRegistrationMsg(forcedRegistrationMsg);
#endif
  
  
  while(advertisementController.step(ADVERTISEMENT_TIME_STEP_MS) != -1) {
    robotAdService.Update();
    uiAdService.Update();
  }
  
  
  printf("Shutting down advertisement service\n");
  robotAdService.StopService();
  uiAdService.StopService();
  return 0;
}
