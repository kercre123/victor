using UnityEngine;
using System.Collections;

public class SpeedTapPanel : BaseDialog {

  [SerializeField]
  private UnityEngine.UI.Text cozmoScoreField;

  [SerializeField]
  private UnityEngine.UI.Text playerScoreField;

  public void SetScoreText(int cozmoScore, int playerScore) {
    if (cozmoScoreField != null) {
      cozmoScoreField.text = "cozmo: " + cozmoScore.ToString();
    }
    if (playerScoreField != null) {
      playerScoreField.text = "player: " + playerScore.ToString();
    }
  }

  protected override void CleanUp() {

  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

  }
}
