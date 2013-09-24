#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "cozmoTypes.h"

using namespace Anki;
using namespace Anki::Embedded;


namespace Localization
{


  void InitLocalization();
  Pose2d GetCurrMatPose();
  void UpdateLocalization();


}
#endif

