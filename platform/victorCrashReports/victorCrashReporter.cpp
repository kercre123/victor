/**
 * File: victorCrashReporter.cpp
 *
 * Description: Implementation of crash report API
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "victorCrashReporter.h"

#ifdef USE_GOOGLE_BREAKPAD
#include "google_breakpad.h"
#endif

#ifdef USE_TOMBSTONE_HOOKS
#include "tombstoneHooks.h"
#endif

namespace Anki {
namespace Victor {

void InstallCrashReporter(const char * filenamePrefix)
{
  #ifdef USE_TOMBSTONE_HOOKS
  Anki::Victor::InstallTombstoneHooks();
  #endif

  #ifdef USE_GOOGLE_BREAKPAD
  GoogleBreakpad::InstallGoogleBreakpad(filenamePrefix);
  #endif

}

void UninstallCrashReporter()
{
  #ifdef USE_GOOGLE_BREAKPAD
  GoogleBreakpad::UnInstallGoogleBreakpad();
  #endif

  #ifdef USE_TOMBSTONE_HOOKS
  Anki::Victor::UninstallTombstoneHooks();
  #endif

}

} // end namespace Victor
} // end namespace Anki
