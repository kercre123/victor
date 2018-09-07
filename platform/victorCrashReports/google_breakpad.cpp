/**
 * File: google_breakpad.cpp
 *
 * Author: chapados
 * Created: 10/08/2014
 *
 * Description: Google breakpad platform-specific methods
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "google_breakpad.h"

#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/logging/DAS.h"

#if (defined(VICOS) && defined(USE_GOOGLE_BREAKPAD))
#include <client/linux/handler/exception_handler.h>
#include <client/linux/handler/minidump_descriptor.h>

#include <chrono>
#include <sstream>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#endif

namespace GoogleBreakpad {

#if (defined(VICOS) && defined(USE_GOOGLE_BREAKPAD))
// Google Breakpad setup
namespace {

#define LOG_CHANNEL "GoogleBreakpad"

static char dumpTag[BUFSIZ];
static char dumpName[BUFSIZ];
static char dumpPath[1024];
static int fd = -1;
static google_breakpad::ExceptionHandler* exceptionHandler;

std::string GetDateTimeString()
{
  using ClockType = std::chrono::system_clock;
  const auto now = ClockType::now();
  const auto numSecs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
  const auto millisLeft = std::chrono::duration_cast<std::chrono::milliseconds>((now - numSecs).time_since_epoch());

  const auto currTime_t = ClockType::to_time_t(now);
  struct tm localTime; // This is local scoped to make it thread safe
  // For some reason this is giving this error message: "__bionic_open_tzdata: couldn't find any tzdata when looking for GMT!"
  localtime_r(&currTime_t, &localTime);

  // Use the old fashioned strftime for thread safety, instead of std::put_time
  char formatTimeBuffer[256];
  strftime(formatTimeBuffer, sizeof(formatTimeBuffer), "%FT%H-%M-%S-", &localTime);

  std::ostringstream stringStream;
  stringStream << formatTimeBuffer << std::setfill('0') << std::setw(3) << millisLeft.count();

  return stringStream.str();
}

bool DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                  void* context, bool succeeded)
{
  LOG_INFO("GoogleBreakpad.DumpCallback",
           "Dump path: '%s', fd = %d, context = %p, succeeded = %s",
           dumpPath, descriptor.fd(), context, succeeded ? "true" : "false");
  if (descriptor.fd() == fd && fd >= 0) {
    (void) close(fd); fd = -1;
  }

  // Report the crash to DAS
  DASMSG(robot_crash, "robot.crash", "Robot service crash");
  DASMSG_SET(s1, dumpTag, "Service name");
  DASMSG_SET(s2, dumpName, "Crash name");
  DASMSG_SEND_ERROR();

  // Return false (not handled) so breakpad will chain to next handler.
  return false;
}

} // anon namespace


void InstallGoogleBreakpad(const char* filenamePrefix)
{
  const std::string & path = "/data/data/com.anki.victor/cache/crashDumps/";
  Anki::Util::FileUtils::CreateDirectory(path);

  const std::string & crashTag = filenamePrefix;
  const std::string & crashName = crashTag + "-" + GetDateTimeString() + ".dmp";
  const std::string & crashFile = path + crashName;

  // Save these strings for later
  (void) strncpy(dumpTag, crashTag.c_str(), sizeof(dumpTag));
  (void) strncpy(dumpName, crashName.c_str(), sizeof(dumpName));
  (void) strncpy(dumpPath, crashFile.c_str(), sizeof(dumpPath));

  fd = open(crashFile.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0600);
  google_breakpad::MinidumpDescriptor descriptor(fd);
  descriptor.set_sanitize_stacks(true);
  exceptionHandler = new google_breakpad::ExceptionHandler(descriptor, NULL, DumpCallback, NULL, true, -1);
}

void UnInstallGoogleBreakpad()
{
  delete exceptionHandler;
  exceptionHandler = nullptr;
  if (fd >= 0) {
    (void) close(fd);
    fd = -1;
  }
  if (dumpPath[0] != '\0') {
    struct stat dumpStat;
    memset(&dumpStat, 0, sizeof(dumpStat));
    int rc = stat(dumpPath, &dumpStat);
    if (!rc && !dumpStat.st_size) {
      (void) unlink(dumpPath);
    }
  }
}

#else

void InstallGoogleBreakpad(const char *path) {}
void UnInstallGoogleBreakpad() {}

#endif

} // namespace GoogleBreakpad
