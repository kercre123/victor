using UnityEngine;
using System.Collections;

namespace CozmoSays {
  public class CozmoSaysGame : GameBase {

    [SerializeField]
    private SayTextSlide _SayTextSlidePrefab;
    private SayTextSlide _SayTextSlideInstance;

    private const string idleAnimLock = "CozmoSaysGame";

    protected override void InitializeGame(ChallengeConfigBase challengeConfigData) {
      CurrentRobot.TurnTowardsLastFacePose(Mathf.PI, callback: (success) => {
        TurnTowardsLastFaceDone();
      });
      SharedMinigameView.HideShelf();
      _SayTextSlideInstance = SharedMinigameView.ShowFullScreenGameStateSlide(_SayTextSlidePrefab.gameObject, "say_text_slide", () => {
        if (!SharedMinigameView.IsQuitAlertViewOpen()) {
          _SayTextSlideInstance.RegisterInputFocus();
        }
      }).GetComponent<SayTextSlide>();
      _SayTextSlideInstance.Initialize(this);
    }

    private void TurnTowardsLastFaceDone() {
      if (CurrentRobot != null) {
        CurrentRobot.PushIdleAnimation(Anki.Cozmo.AnimationTrigger.CozmoSaysIdle, idleAnimLock);
        CurrentRobot.SetDefaultHeadAndLiftState(true, CurrentRobot.GetHeadAngleFactor(), 0.0f);
      }
    }

    protected override void CleanUpOnDestroy() {
      if (CurrentRobot != null) {
        CurrentRobot.SetDefaultHeadAndLiftState(false, 0.0f, 0.0f);
      }
    }
  }

}
