#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "cozmoTypes.h"


namespace Localization
{


  void InitLocalization();
  Anki::Embedded::Pose2d GetCurrMatPose();
  void UpdateLocalization();


}
#endif

