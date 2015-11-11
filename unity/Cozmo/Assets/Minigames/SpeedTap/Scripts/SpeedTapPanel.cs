using UnityEngine;
using System.Collections;

namespace SpeedTap {

  public class SpeedTapPanel : BaseDialog {

    [SerializeField]
    private UnityEngine.UI.Text _CozmoScoreField;

    [SerializeField]
    private UnityEngine.UI.Text _PlayerScoreField;

    [SerializeField]
    private UnityEngine.UI.Button _TapButton;

    public delegate void TapButtonPressedHandler();

    public TapButtonPressedHandler TapButtonPressed;

    void Start() {
      _TapButton.onClick.AddListener(OnTapped);
    }

    private void OnTapped() {
      if (TapButtonPressed != null) {
        TapButtonPressed();
      }
    }

    public void SetScoreText(int cozmoScore, int playerScore) {
      if (_CozmoScoreField != null) {
        _CozmoScoreField.text = "cozmo: " + cozmoScore.ToString();
      }
      if (_PlayerScoreField != null) {
        _PlayerScoreField.text = "player: " + playerScore.ToString();
      }
    }

    protected override void CleanUp() {

    }

    protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

    }
  }

}
