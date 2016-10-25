/**
 * File: google_breakpad.cpp
 *
 * Author: chapados
 * Created: 10/08/2014
 *
 * Description: Google breakpad platform-specific methods used by RushHour instance
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/cozmo/csharp-binding/breakpad/google_breakpad.h"

#include "util/logging/logging.h"

#if (defined(ANDROID) && defined(USE_GOOGLE_BREAKPAD))
#include <client/linux/handler/exception_handler.h>
#include <client/linux/handler/minidump_descriptor.h>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#endif

namespace GoogleBreakpad {

#if (defined(ANDROID) && defined(USE_GOOGLE_BREAKPAD))
// Google Breakpad setup
namespace {

static char dumpPath[1024] = {'\0'};
static int fd = -1;
static google_breakpad::ExceptionHandler* exceptionHandler;

bool DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                  void* context, bool succeeded)
{
  PRINT_NAMED_INFO("GoogleBreakpad.DumpCallback",
                   "Dump path: '%s', fd = %d, context = %p, succeeded = %s",
                   dumpPath, descriptor.fd(), context, succeeded ? "true" : "false");
  if (descriptor.fd() == fd && fd >= 0) {
    (void) close(fd); fd = -1;
  }
  return succeeded;
}

} // anon namespace

void InstallGoogleBreakpad(const char *path)
{
  (void) strlcpy(dumpPath, path, sizeof(dumpPath));
  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_EXCL, 0600);
  google_breakpad::MinidumpDescriptor descriptor(fd);
  exceptionHandler = new google_breakpad::ExceptionHandler(descriptor, NULL, DumpCallback, NULL, true, -1);
}

void UnInstallGoogleBreakpad()
{
  delete exceptionHandler; exceptionHandler = nullptr;
  if (fd >= 0) {
    (void) close(fd); fd = -1;
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
