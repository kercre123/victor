#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include "anki/cozmo/robot/cozmoTypes.h"


namespace Localization
{


  void InitLocalization();
  Anki::Embedded::Pose2d GetCurrMatPose();
  void UpdateLocalization();


}
#endif

