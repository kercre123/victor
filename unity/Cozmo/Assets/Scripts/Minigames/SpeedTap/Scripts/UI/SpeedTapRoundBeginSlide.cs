using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.UI;

namespace SpeedTap {
  public class SpeedTapRoundBeginSlide : MonoBehaviour {

    [SerializeField]
    private AnkiTextLegacy _RoundHeaderLabel;

    [SerializeField]
    private AnkiTextLegacy _RoundSubtitleLabel;

    public void SetText(int currentRound, int totalRounds, int playerCount) {
      _RoundHeaderLabel.text = Localization.GetWithArgs(LocalizationKeys.kSpeedTapRoundsText, currentRound);
      if (playerCount <= 2) {
        _RoundSubtitleLabel.text = Localization.GetWithArgs(LocalizationKeys.kSpeedTapTextBestOf, totalRounds);
      }
      else {
        _RoundSubtitleLabel.text = Localization.Get(LocalizationKeys.kSpeedTapTextMPBestOf);
      }
    }
  }
}