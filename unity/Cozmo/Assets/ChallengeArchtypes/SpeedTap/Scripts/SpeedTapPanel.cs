using UnityEngine;
using System.Collections;
using Cozmo.UI;

namespace SpeedTap {

  public class SpeedTapPanel : BaseView {

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
        string localizedText = Localization.Get(LocalizationKeys.kSpeedTapScoreCozmo);
        _CozmoScoreField.text = string.Format(Localization.GetCultureInfo(), localizedText, cozmoScore);
      }
      if (_PlayerScoreField != null) {
        string localizedText = Localization.Get(LocalizationKeys.kSpeedTapScorePlayer);
        _PlayerScoreField.text = string.Format(Localization.GetCultureInfo(), localizedText, playerScore);
      }
    }

    protected override void CleanUp() {

    }
  }

}
