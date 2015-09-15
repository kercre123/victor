/*
 * File:          webotsCtrlViz.cpp
 * Date:          03-19-2014
 * Description:   Interface for basestation to all visualization functions in Webots including 
 *                cozmo_physics draw functions, display window text printing, and other custom display
 *                methods.
 * Author:        Kevin Yoon
 * Modifications: 
 */

#include "vizControllerImpl.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/types/vizTypes.h"
#include "clad/vizInterface/messageViz.h"
#include "anki/messaging/shared/UdpServer.h"
#include "anki/messaging/shared/UdpClient.h"
#include <webots/Supervisor.hpp>
#include <webots/ImageRef.hpp>
#include <webots/Display.hpp>
#include <cstdio>
#include <string>


using namespace Anki::Cozmo;

int main(int argc, char **argv)
{
  webots::Supervisor vizSupervisor;
  VizControllerImpl vizController(vizSupervisor);
  const size_t maxPacketSize{(size_t)VizConstants::MaxMessageSize};
  char data[maxPacketSize];
  size_t numBytesRecvd;
  
  // Setup server to listen for commands
  UdpServer server;
  server.StartListening((uint16_t)VizConstants::VIZ_SERVER_PORT);
  
  
  // Setup client to forward relevant commands to cozmo_physics plugin
  UdpClient physicsClient;
  physicsClient.Connect("127.0.0.1", (uint16_t)VizConstants::PHYSICS_PLUGIN_SERVER_PORT);

  vizController.Init();
  
  //
  // Main Execution loop
  //
  while (vizSupervisor.step(Anki::Cozmo::TIME_STEP) != -1)
  {
    // Any messages received?
    while ((numBytesRecvd = server.Recv(data, maxPacketSize)) > 0) {
      physicsClient.Send(data, numBytesRecvd);
      vizController.ProcessMessage(VizInterface::MessageViz(data, numBytesRecvd));
    } // while server.Recv
    
  } // while step

  
  return 0;
}

