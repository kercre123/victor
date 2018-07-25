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
#include "../shared/ctrlCommonInitialization.h"
#include "anki/cozmo/shared/cozmoEngineConfig.h"
#include "clad/types/vizTypes.h"
#include "clad/vizInterface/messageViz.h"
#include "coretech/messaging/shared/UdpServer.h"
#include "coretech/messaging/shared/UdpClient.h"
#include <webots/Supervisor.hpp>
#include <webots/ImageRef.hpp>
#include <webots/Display.hpp>
#include <cstdio>
#include <string>


int main(int argc, char **argv)
{
  using namespace Anki;
  using namespace Anki::Cozmo;

  // parse commands
  WebotsCtrlShared::ParsedCommandLine params = WebotsCtrlShared::ParseCommandLine(argc, argv);
  // create platform
  const Anki::Util::Data::DataPlatform& dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0], "webotsCtrlViz");
  // initialize logger
  WebotsCtrlShared::DefaultAutoGlobalLogger autoLogger(dataPlatform, params.filterLog, params.colorizeStderrOutput);

  webots::Supervisor vizSupervisor;
  VizControllerImpl vizController(vizSupervisor);
  const size_t maxPacketSize{(size_t)VizConstants::MaxMessageSize};
  uint8_t data[maxPacketSize]{0};
  ssize_t numBytesRecvd;
  
  // Setup server to listen for commands
  UdpServer server("webotsCtrlViz");
  server.StartListening((uint16_t)VizConstants::VIZ_SERVER_PORT);
  
  // Setup client to forward relevant commands to cozmo_physics plugin
  UdpClient physicsClient;
  physicsClient.Connect("127.0.0.1", (uint16_t)VizConstants::PHYSICS_PLUGIN_SERVER_PORT);
  
  vizController.Init();
  
  //
  // Main Execution loop
  //
  while (vizSupervisor.step(Anki::Cozmo::BS_TIME_STEP_MS) != -1)
  {
    // Any messages received?
    while ((numBytesRecvd = server.Recv((char*)data, maxPacketSize)) > 0) {
      physicsClient.Send((char*)data, (int)numBytesRecvd);
      vizController.ProcessMessage(VizInterface::MessageViz(data, numBytesRecvd));
    } // while server.Recv
    
  } // while step

  
  return 0;
}

