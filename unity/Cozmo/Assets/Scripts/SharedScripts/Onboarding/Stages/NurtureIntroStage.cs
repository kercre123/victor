using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;

namespace Onboarding {

  // This screen is either initial
  public class NurtureIntroStage : ShowContinueStage {
    [SerializeField]
    private CozmoText _TextFieldInstance;

    protected override void Awake() {
      base.Awake();

      if (OnboardingManager.Instance.IsReturningUser()) {
        _TextFieldInstance.text = Localization.Get(LocalizationKeys.kOnboardingNeedsIntroReturning);
      }

      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Appear);

      // set up for the upcoming repair bit
      ForceSetDamagedParts msg = new ForceSetDamagedParts();
      msg.partIsDamaged = new bool[] { false, true, false };
      RobotEngineManager.Instance.Message.ForceSetDamagedParts = msg;
      RobotEngineManager.Instance.SendMessage();
    }

    public override void Start() {
      base.Start();
      // Usually not having completed onboarding prevents freeplay.
      // this stage is the exception, and we still dont want freeplay music, etc.
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        if (robot.Faces.Count > 0) {
          robot.TurnTowardsLastFacePose(Mathf.PI);
        }
        else {
          robot.ActivateHighLevelActivity(Anki.Cozmo.HighLevelActivity.Selection);
          robot.ExecuteBehaviorByID(Anki.Cozmo.BehaviorID.FindFaces_socialize);
        }
      }
    }
    public override void OnDestroy() {
      base.OnDestroy();
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.ExecuteBehaviorByID(Anki.Cozmo.BehaviorID.Wait);
      }
    }

  }

}
