/**
 * File: victorCrashReporter.h
 *
 * Description: Declaration of crash report API
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __victorCrashReporter_h
#define __victorCrashReporter_h

namespace Anki {
namespace Victor {

//
// Install signal and exception handlers.
// FilenamePrefix may not be null.
//
void InstallCrashReporter(const char * filenamePrefix);

//
// Uninstall signal and exception handlers.
//
void UninstallCrashReporter();

} // end namespace Victor
} // end namespace Anki

#endif // __victorCrashReporter_h
