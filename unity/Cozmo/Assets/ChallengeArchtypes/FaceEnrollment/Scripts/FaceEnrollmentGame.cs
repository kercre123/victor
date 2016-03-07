using UnityEngine;
using System.Collections;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    //private float _LastKnownFaceSeenPlayed = 0.0f;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      LookForNewFaceToEnroll();
      //_LastKnownFaceSeenPlayed = Time.time;
    }

    private void LookForNewFaceToEnroll() {
      CurrentRobot.SetLiftHeight(0.0f);
      CurrentRobot.SetHeadAngle(0.5f);
      CurrentRobot.EnableNewFaceEnrollment();
    }

    protected override void CleanUpOnDestroy() {
    }

  }

}
