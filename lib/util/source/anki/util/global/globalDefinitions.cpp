/**
 * File: globalDefinitions.cpp
 *
 * Author: mwesley
 * Created: 06/10/2015
 * 
 * Description: Implementations for any global definitions
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "util/global/globalDefinitions.h"
#include "util/console/consoleInterface.h"


namespace Anki {
namespace Util {
  
CONSOLE_VAR(bool, kForceDisableAnkiDevFeatures, "Dev", false);

const char * HidePersonallyIdentifiableInfo(const char* str)
{
  static const char * const kPrivacyString = "<PII>";
  if(ANKI_PRIVACY_GUARD)
  {
    return kPrivacyString;
  }
  else
  {
    return str;
  }
}

} // Anki::Util namespace
} // Anki namespace


bool NativeAnkiUtilAreDevFeaturesEnabled()
{
  return ((ANKI_DEV_CHEATS != 0) && !Anki::Util::kForceDisableAnkiDevFeatures);
}
