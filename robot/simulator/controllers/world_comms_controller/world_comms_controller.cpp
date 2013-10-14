/*
 * File:          world_comms_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "CozmoWorldComms.h"

#include "anki/cozmo/basestation/blockWorld.h"
//#include "anki/messaging/basestation/messagingInterface.h"

int main(int argc, char **argv)
{
  //Anki::MessagingInterface* msgInterface = new Anki::MessagingInterface_TCP();
  
  //Anki::Cozmo::BlockWorld blockWorld(msgInterface);
  CozmoWorldComms comms;
  comms.Init();
  comms.run();

  //delete msgInterface;
  
  return 0;
}
