/**
 * File: victorCrashReporter.cpp
 *
 * Description: Implementation of crash report API
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "victorCrashReporter.h"
#include "google_breakpad.h"
#include <assert.h>

namespace Anki {
namespace Victor {

void InstallCrashReporter(const char * filenamePrefix)
{
  assert(filenamePrefix != nullptr);
  GoogleBreakpad::InstallGoogleBreakpad(filenamePrefix);
}

void UninstallCrashReporter()
{
  GoogleBreakpad::UnInstallGoogleBreakpad();
}

} // end namespace Victor
} // end namespace Anki
