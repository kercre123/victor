using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.UI;

namespace SpeedTap {
  public class SpeedTapPlayerTapConfirmSlide : MonoBehaviour {

    [SerializeField]
    private GameObject _SPContainer;

    [SerializeField]
    private GameObject _MPContainer;

    [SerializeField]
    private GameObject[] _Player1OnlyContent;
    [SerializeField]
    private GameObject[] _Player2OnlyContent;

    [SerializeField]
    private GameObject[] _ConfirmContent;

    // This field changes iff we're using this for MP multiround selection
    [SerializeField]
    private AnkiTextLegacy _MPTapPlayer1Text;

    public void Init(int numPlayers, int playerIndex = 0, int roundsPlayed = 0) {
      _SPContainer.SetActive(numPlayers <= 2);
      _MPContainer.SetActive(numPlayers > 2);
      if (numPlayers > 2) {
        // If playerIndex is invalid ( -1 ) then show the final step...
        for (int i = 0; i < _ConfirmContent.Length; ++i) {
          _ConfirmContent[i].SetActive(playerIndex <= 0);
        }
        for (int i = 0; i < _Player2OnlyContent.Length; ++i) {
          _Player2OnlyContent[i].SetActive(playerIndex == 2);
        }
        for (int i = 0; i < _Player1OnlyContent.Length; ++i) {
          _Player1OnlyContent[i].SetActive(playerIndex == 1);
        }
        // Only player1's message in MP changes based on round everything else is same
        if (playerIndex == 1 && roundsPlayed > 0) {
          _MPTapPlayer1Text.text = Localization.Get(LocalizationKeys.kSpeedTapTapBlock2);
        }
      }
    }
  }
}