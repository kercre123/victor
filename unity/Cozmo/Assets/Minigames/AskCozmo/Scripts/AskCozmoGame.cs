using UnityEngine;
using System.Collections;

public class AskCozmoGame : GameBase {

  private bool _AnimationPlaying = false;

  [SerializeField]
  private AskCozmoPanel _GamePanelPrefab;

  private AskCozmoPanel _GamePanel;

  void Start() {
    _GamePanel = UIManager.OpenDialog(_GamePanelPrefab).GetComponent<AskCozmoPanel>();
    _GamePanel.OnAskButtonPressed += OnAnswerRequested;
    CreateDefaultQuitButton();
  }

  public override void CleanUp() {
    if (_GamePanel != null) {
      UIManager.CloseDialogImmediately(_GamePanel);
    }
    DestroyDefaultQuitButton();
  }

  // user just asked the question and pressed the
  // give me answer button.
  public void OnAnswerRequested() {
    if (_AnimationPlaying) {
      return;
    }

    _AnimationPlaying = true;
    if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.5f) {
      robot.SendAnimation("majorWin", AnimationDone);
    }
    else {
      robot.SendAnimation("shocked", AnimationDone);
    }
  }

  void AnimationDone(bool success) {
    _AnimationPlaying = false;
  }
}
