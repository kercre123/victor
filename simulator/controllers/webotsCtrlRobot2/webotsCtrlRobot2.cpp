/*
 * File:          webotsCtrlRobot2.cpp
 * Date:
 * Description:   Cozmo 2.0 robot process
 * Author:
 * Modifications:
 */


#include "../shared/ctrlCommonInitialization.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "simulator/robot/sim_overlayDisplay.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/shared/factory/emr.h"
#include <cstdio>
#include <sstream>

#include <webots/Supervisor.hpp>

/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */

namespace Anki {
  namespace Vector {
    namespace Sim {
      extern webots::Supervisor* CozmoBot;
    }
  }
}


int main(int argc, char **argv)
{
  using namespace Anki;
  using namespace Anki::Vector;

  // The simulated EMR is created at runtime which allows us to determine if the simulated robot
  // is Whiskey or Vector. As such it needs to be created in a shared location so all webots
  // controllers that need access to it can easily access it. We already create and write files
  // to the playbackLogs dir so it felt like the best place to stick the EMR.
  // All other controllers that will be accessing the EMR need to 'step' their supervisors
  // immediately in their 'main' function. This forces them to block until this 'main' is called
  // and the EMR is created. Once we 'step' our supervisor will that unblock the others and ensure
  // they are accessing a valid EMR.
  const std::string emrPath = "../../../_build/mac/Debug/playbackLogs/emr";
  FILE* f = fopen(emrPath.c_str(), "w+");
  if(f != nullptr)
  {
    Anki::Vector::Factory::EMR emr;
    memset(&emr, 0, sizeof(emr));

    if(Sim::CozmoBot != nullptr &&
       Sim::CozmoBot->getSelf() != nullptr &&
       Sim::CozmoBot->getSelf()->getField("name")->getSFString() == "WhiskeyBot")
    {
      // HW_VER >= 7 is Whiskey
      emr.fields.HW_VER = 7;
    }
    
    (void)fwrite(&emr, sizeof(emr), 1, f);
    (void)fclose(f);
  }
  else
  {
    // Logger not yet created, use fprintf to log to webots console
    fprintf(stderr, "Failed to open simulated EMR file %d", errno);
  }

  // Placeholder for SIGTERM flag
  int shutdownSignal = 0;

  // parse commands
  WebotsCtrlShared::ParsedCommandLine params = WebotsCtrlShared::ParseCommandLine(argc, argv);
  // create platform
  const Anki::Util::Data::DataPlatform& dataPlatform = WebotsCtrlShared::CreateDataPlatformBS(argv[0], "webotsCtrlRobot2");
  // initialize logger
  WebotsCtrlShared::DefaultAutoGlobalLogger autoLogger(dataPlatform, params.filterLog, params.colorizeStderrOutput);

  if(Robot::Init(&shutdownSignal) != Anki::RESULT_OK) {
    fprintf(stdout, "Failed to initialize Vector::Robot!\n");
    return -1;
  }

  Sim::OverlayDisplay::Init();

  HAL::Step();

  while(Robot::step_MainExecution() == Anki::RESULT_OK)
  {
    HAL::UpdateDisplay();
    HAL::Step();
  }

  return 0;
}
