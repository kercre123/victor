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

} // Anki::Util namespace
} // Anki namespace


bool NativeAnkiUtilAreDevFeaturesEnabled()
{
  return ((ANKI_DEV_CHEATS != 0) && !Anki::Util::kForceDisableAnkiDevFeatures);
}
