/**
 * File: google_breakpad.h
 *
 * Author: chapados
 * Created: 10/08/2014
 *
 * Description: Android-specific methods used by RushHour instance
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __GOOGLE_BREAKPAD_H__
#define __GOOGLE_BREAKPAD_H__

namespace GoogleBreakpad {

void InstallGoogleBreakpad(const char *path);
void UnInstallGoogleBreakpad();

}

#endif // #ifndef __GOOGLE_BREAKPAD_H__



