using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.UI;

namespace SpeedTap {
  public class SpeedTapRoundBeginSlide : MonoBehaviour {

    [SerializeField]
    private AnkiTextLabel _RoundHeaderLabel;

    [SerializeField]
    private AnkiTextLabel _RoundSubtitleLabel;

    public void SetText(int currentRound, int totalRounds) {
      _RoundHeaderLabel.text = Localization.GetWithArgs(LocalizationKeys.kSpeedTapRoundsText, currentRound);
      _RoundSubtitleLabel.text = Localization.GetWithArgs(LocalizationKeys.kSpeedTapTextBestOf, totalRounds);
    }
  }
}