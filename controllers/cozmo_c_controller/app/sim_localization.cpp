#include "app/localization.h"

#include "anki/embeddedCommon.h"

#include "cozmoBot.h"
extern CozmoBot gCozmoBot;

namespace Localization
{
  namespace {
    // private members
    Anki::Embedded::Pose2d currMatPose;
  }

  void InitLocalization() {

  }

  Anki::Embedded::Pose2d GetCurrMatPose()
  {
    return currMatPose;
  }

  void UpdateLocalization() {
    float angle;
    gCozmoBot.GetGlobalPose(currMatPose.x(),currMatPose.y(),angle);
    currMatPose.angle = angle;
  }

}
