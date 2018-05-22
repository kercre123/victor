/**
* File: vic-dasmgr.cpp
*
* Description: Victor DAS Manager service app
*
* Copyright: Anki, inc. 2018
*
*/
#include "dasManager.h"

#include "util/logging/logging.h"
#include "util/logging/victorLogger.h"

#include <signal.h>
#include <stdlib.h>

#define LOG_PROCNAME "vic-dasmgr"
#define LOG_CHANNEL  LOG_PROCNAME

namespace
{
  bool gShutdown = false;
}

void Shutdown(int signum)
{
  LOG_DEBUG("main.Shutdown", "Shutdown on signal %d", signum);
  gShutdown = true;
}

int main(int argc, const char * argv[])
{
  signal(SIGTERM, Shutdown);

  // Set up logging
  auto logger = std::make_unique<Anki::Util::VictorLogger>(LOG_PROCNAME);
  Anki::Util::gLoggerProvider = logger.get();
  Anki::Util::gEventProvider = logger.get();

  // Say hello
  LOG_DEBUG("main.hello", "Hello world");

  DASMSG(dasmgr_main_hello, "dasmgr.main.hello", "Sent at application start");
  DASMSG_SET(s1, "s1", "string 1");
  DASMSG_SET(i1, 1, "int 1");
  DASMSG_SEND();

  // Process log records until shutdown or error
  Anki::Victor::DASManager dasManager;

  const int status = dasManager.Run(gShutdown);

  // Say goodbye & we're done
  LOG_DEBUG("main.goodbye", "Goodbye world (exit %d)", status);
  Anki::Util::gLoggerProvider = nullptr;
  Anki::Util::gEventProvider = nullptr;

  exit(status);

}
