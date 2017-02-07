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

    public void Init(int numPlayers, int playerIndex = 0) {
      _SPContainer.SetActive(numPlayers <= 2);
      _MPContainer.SetActive(numPlayers > 2);
      if (numPlayers > 2) {
        for (int i = 0; i < _Player2OnlyContent.Length; ++i) {
          _Player2OnlyContent[i].SetActive(playerIndex > 1);
        }
        for (int i = 0; i < _Player1OnlyContent.Length; ++i) {
          _Player1OnlyContent[i].SetActive(playerIndex <= 1);
        }
      }
    }
  }
}