#include "anki/cozmo/robot/cozmoBot.h"
#include "anki/cozmo/robot/localization.h"


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
    Robot::GetGlobalPose(currMatPose.x(),currMatPose.y(),angle);
    currMatPose.angle = angle;
  }

}
