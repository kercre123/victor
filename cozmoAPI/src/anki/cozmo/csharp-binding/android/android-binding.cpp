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

#include "anki/cozmo/csharp-binding/android/android-binding.h"
#include "anki/cozmo/csharp-binding/breakpad/google_breakpad.h"

namespace Anki {
namespace Cozmo {
namespace AndroidBinding {

void InstallGoogleBreakpad(const char* path)
{
  GoogleBreakpad::InstallGoogleBreakpad(path);
}

void UnInstallGoogleBreakpad()
{
  GoogleBreakpad::UnInstallGoogleBreakpad();
}

}
}
}
