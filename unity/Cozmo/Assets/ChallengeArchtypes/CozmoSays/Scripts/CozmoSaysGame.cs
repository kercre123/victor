using UnityEngine;
using System.Collections;

namespace CozmoSays {
  public class CozmoSaysGame : GameBase {

    [SerializeField]
    private SayTextSlide _SayTextSlidePrefab;
    private SayTextSlide _SayTextSlideInstance;

    protected override void InitializeGame(MinigameConfigBase minigameConfig) {
      CurrentRobot.TurnTowardsLastFacePose(Mathf.PI, callback: (success) => {
        TurnTowardsLastFaceDone();
      });
      SharedMinigameView.HideShelf();
      _SayTextSlideInstance = SharedMinigameView.ShowWideGameStateSlide(_SayTextSlidePrefab.gameObject, "say_text_slide", () => {
        _SayTextSlideInstance.RegisterInputFocus();
      }).GetComponent<SayTextSlide>();
    }

    private void TurnTowardsLastFaceDone() {
      CurrentRobot.PushIdleAnimation(Anki.Cozmo.AnimationTrigger.CozmoSaysIdle);
    }

    protected override void CleanUpOnDestroy() {
      CurrentRobot.PopIdleAnimation();
    }
  }

}
