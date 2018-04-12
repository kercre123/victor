/**
 * File: android-binding
 *
 * Author: baustin
 * Created: 07/14/16
 *
 * Description: Android-specific native hooks for UI
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

//
// This file is not used on victor.  Contents are preserved for reference.
//
#if !defined(VICOS_LA) && !defined(VICOS_LE)

#ifndef ANKI_COZMOAPI_ANDROID_BINDING_H
#define ANKI_COZMOAPI_ANDROID_BINDING_H

namespace Anki {
namespace Cozmo {
namespace AndroidBinding {

void InstallGoogleBreakpad(const char* path);
void UnInstallGoogleBreakpad();
int cozmo_shutdown();

}
}
}

#endif

#endif
