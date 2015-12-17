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
    private UnityEngine.UI.Text _PlayerRoundsWon;

    [SerializeField]
    private UnityEngine.UI.Text _CozmoRoundsWon;

    [SerializeField]
    private UnityEngine.UI.Text _RoundsText;

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

    public void SetScoreText(int cozmoScore, int playerScore, int cozmoRoundsWon, int playerRoundsWon, int totalRounds) {
      _CozmoScoreField.text = string.Format(Localization.GetCultureInfo(), Localization.Get(LocalizationKeys.kSpeedTapScoreCozmo), cozmoScore);
      _PlayerScoreField.text = string.Format(Localization.GetCultureInfo(), Localization.Get(LocalizationKeys.kSpeedTapScorePlayer), playerScore);
      _CozmoRoundsWon.text = string.Format(Localization.GetCultureInfo(), Localization.Get(LocalizationKeys.kSpeedTapRoundsWonCozmo), cozmoRoundsWon);
      _PlayerRoundsWon.text = string.Format(Localization.GetCultureInfo(), Localization.Get(LocalizationKeys.kSpeedTapRoundsWonPlayer), playerRoundsWon);
      _RoundsText.text = string.Format(Localization.GetCultureInfo(), Localization.Get(LocalizationKeys.kSpeedTapRoundsText), cozmoRoundsWon + playerRoundsWon, totalRounds);
    }

    protected override void CleanUp() {

    }
  }

}
