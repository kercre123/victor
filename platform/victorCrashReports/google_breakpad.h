/**
 * File: google_breakpad.h
 *
 * Author: chapados
 * Created: 10/08/2014
 *
 * Description: Google breakpad platform-specific methods
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __GOOGLE_BREAKPAD_H__
#define __GOOGLE_BREAKPAD_H__

namespace GoogleBreakpad {

void InstallGoogleBreakpad(const char* filenamePrefix);
void UnInstallGoogleBreakpad();

}

#endif // #ifndef __GOOGLE_BREAKPAD_H__



