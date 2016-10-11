using UnityEngine;
using System.Collections;

namespace CozmoSays {
  public class CozmoSaysGame : GameBase {

    [SerializeField]
    private SayTextSlide _SayTextSlidePrefab;

    protected override void InitializeGame(MinigameConfigBase minigameConfig) {
      SharedMinigameView.ShowFullScreenGameStateSlide(_SayTextSlidePrefab.gameObject, "say_text_slide");
    }

    protected override void CleanUpOnDestroy() {

    }
  }

}
