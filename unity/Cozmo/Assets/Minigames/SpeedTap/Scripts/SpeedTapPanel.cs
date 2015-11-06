using UnityEngine;
using System.Collections;

public class SpeedTapPanel : BaseDialog {

  [SerializeField]
  private UnityEngine.UI.Text cozmoScoreField_;

  [SerializeField]
  private UnityEngine.UI.Text playerScoreField_;

  [SerializeField]
  private UnityEngine.UI.Button tapButton_;

  public delegate void TapButtonPressedHandler();

  public TapButtonPressedHandler TapButtonPressed;

  void Start() {
    tapButton_.onClick.AddListener(OnTapped);
  }

  private void OnTapped() {
    if (TapButtonPressed != null) {
      TapButtonPressed();
    }
  }

  public void SetScoreText(int cozmoScore, int playerScore) {
    if (cozmoScoreField_ != null) {
      cozmoScoreField_.text = "cozmo: " + cozmoScore.ToString();
    }
    if (playerScoreField_ != null) {
      playerScoreField_.text = "player: " + playerScore.ToString();
    }
  }

  protected override void CleanUp() {

  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

  }
}
