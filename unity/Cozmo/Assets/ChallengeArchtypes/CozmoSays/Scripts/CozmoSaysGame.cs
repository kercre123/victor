using UnityEngine;
using System.Collections;

namespace CozmoSays {
  public class CozmoSaysGame : GameBase {

    [SerializeField]
    private SayTextSlide _SayTextSlidePrefab;
    private SayTextSlide _SayTextSlideInstance;

    protected override void InitializeGame(MinigameConfigBase minigameConfig) {
      CurrentRobot.TurnTowardsLastFacePose(Mathf.PI);
      SharedMinigameView.HideShelf();
      _SayTextSlideInstance = SharedMinigameView.ShowWideGameStateSlide(_SayTextSlidePrefab.gameObject, "say_text_slide", () => {
        _SayTextSlideInstance.RegisterInputFocus();
      }).GetComponent<SayTextSlide>();
    }

    protected override void CleanUpOnDestroy() {

    }
  }

}
