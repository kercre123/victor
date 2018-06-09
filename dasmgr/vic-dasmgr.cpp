/**
* File: vic-dasmgr.cpp
*
* Description: Victor DAS Manager service app
*
* Copyright: Anki, inc. 2018
*
*/
#include "dasManager.h"

#include "platform/victorCrashReports/victorCrashReporter.h"

#include "util/logging/logging.h"
#include "util/logging/DAS.h"
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
  // Set up crash reporter
  Anki::Victor::InstallCrashReporter(LOG_PROCNAME);

  // Set up logging
  auto logger = std::make_unique<Anki::Util::VictorLogger>(LOG_PROCNAME);
  Anki::Util::gLoggerProvider = logger.get();
  Anki::Util::gEventProvider = logger.get();

  // Set up signal handler
  signal(SIGTERM, Shutdown);

  // Say hello
  LOG_DEBUG("main.hello", "Hello world");

  DASMSG(dasmgr_main_hello, "dasmgr.main.hello", "Sent at application start");
  DASMSG_SET(s1, "s1", "Test string 1");
  DASMSG_SET(s2, "s2", "Test string 2");
  DASMSG_SET(s3, "s3", "Test string 3");
  DASMSG_SET(s4, "s4", "Test string 4");
  DASMSG_SET(i1, 111, "Test int 1");
  DASMSG_SET(i2, 222, "Test int 2");
  DASMSG_SET(i3, 333, "Test int 3");
  DASMSG_SET(i4, 444, "Test int 4");
  DASMSG_SEND();

  DASMSG(dasmgr_main_debug, "dasmgr.main.debug", "Test event");
  DASMSG_SET(s1, "This is a debug event", "Test field");
  DASMSG_SEND_DEBUG();

  DASMSG(dasmgr_main_warning, "dasmgr.main.warning", "Test event");
  DASMSG_SET(s1, "This is a warning event", "Test field");
  DASMSG_SEND_WARNING();

  DASMSG(dasmgr_main_error, "dasmgr.main.error", "Test event");
  DASMSG_SET(s1, "This is an error event", "Test field");
  DASMSG_SEND_ERROR();

  // Process log records until shutdown or error
  Anki::Victor::DASManager dasManager;

  const int status = dasManager.Run(gShutdown);

  // Say goodbye & we're done
  LOG_DEBUG("main.goodbye", "Goodbye world (exit %d)", status);

  Anki::Util::gLoggerProvider = nullptr;
  Anki::Util::gEventProvider = nullptr;

  Anki::Victor::UninstallCrashReporter();

  exit(status);

}
