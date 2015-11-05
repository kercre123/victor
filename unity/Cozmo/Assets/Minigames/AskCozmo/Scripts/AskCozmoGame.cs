using UnityEngine;
using System.Collections;

public class AskCozmoGame : GameBase {

  private bool animationPlaying_ = false;

  [SerializeField]
  private AskCozmoPanel gamePanelPrefab_;

  private AskCozmoPanel gamePanel_;

  void Start() {
    gamePanel_ = UIManager.OpenDialog(gamePanelPrefab_).GetComponent<AskCozmoPanel>();
    gamePanel_.OnAskButtonPressed += OnAnswerRequested;
  }

  public override void CleanUp() {
    if (gamePanel_ != null) {
      UIManager.CloseDialogImmediately(gamePanel_);
    }
  }

  // user just asked the question and pressed the
  // give me answer button.
  public void OnAnswerRequested() {
    if (animationPlaying_) {
      return;
    }

    animationPlaying_ = true;
    if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.5f) {
      robot.SendAnimation("majorWin", AnimationDone);
    }
    else {
      robot.SendAnimation("shocked", AnimationDone);
    }
  }

  void AnimationDone(bool success) {
    animationPlaying_ = false;
  }
}
