#include "app/localization.h"

#include "anki/embeddedCommon.h"

#include "cozmoBot.h"
extern CozmoBot gCozmoBot;

using namespace Anki;
using namespace Anki::Embedded;


namespace Localization
{
  namespace {
    // private members
    Pose2d currMatPose;
  }

  void InitLocalization() {

  }

  Pose2d GetCurrMatPose()
  {
    return currMatPose;
  }

  void UpdateLocalization() {
    float angle;
    gCozmoBot.GetGlobalPose(currMatPose.x(),currMatPose.y(),angle);
    currMatPose.angle = angle;
  }

}
