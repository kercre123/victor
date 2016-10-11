using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.UI;
using Cozmo.UI;

namespace Simon {
  public class SimonTurnSlide : MonoBehaviour {

    [SerializeField]
    private Image _CenterImage;

    [SerializeField]
    private AnkiTextLabel _StatusTextLabel;

    [SerializeField]
    private Sprite _CorrectSprite;
    [SerializeField]
    private Sprite _WrongSprite;

    [SerializeField]
    private SimonLivesWidget _PlayerWidget;
    [SerializeField]
    private SimonLivesWidget _CozmoWidget;

    [SerializeField]
    private CozmoButton _ButtonPlayPattern;

    public void Awake() {
      _PlayerWidget.gameObject.SetActive(false);
      _CozmoWidget.gameObject.SetActive(false);
      _CenterImage.gameObject.SetActive(false);
      _StatusTextLabel.text = "";
    }

    public void ShowEndGame(Sprite portraitSprite, string status = null) {
      _PlayerWidget.gameObject.SetActive(false);
      _CozmoWidget.gameObject.SetActive(false);
      if (portraitSprite) {
        _CenterImage.gameObject.SetActive(true);
        _CenterImage.sprite = portraitSprite;
      }
      else {
        _CenterImage.gameObject.SetActive(false);
      }
      if (status != null) {
        _StatusTextLabel.text = status;
      }
      else {
        _StatusTextLabel.text = "";
      }
    }

    public void ShowHumanLives(int lives, int maxLives, string headerLocKey, string statusLocKey) {
      _PlayerWidget.gameObject.SetActive(true);
      _CozmoWidget.gameObject.SetActive(false);

      _PlayerWidget.UpdateStatus(lives, maxLives, headerLocKey, statusLocKey);
    }
    public void ShowCozmoLives(int lives, int maxLives, string headerLocKey, string statusLocKey) {
      _PlayerWidget.gameObject.SetActive(false);
      _CozmoWidget.gameObject.SetActive(true);
      _CozmoWidget.UpdateStatus(lives, maxLives, headerLocKey, statusLocKey);
    }

    public void ShowCenterImage(bool enabled, bool showCorrect) {
      _CenterImage.gameObject.SetActive(enabled);
      _CenterImage.sprite = showCorrect ? _CorrectSprite : _WrongSprite;
    }

    public void ShowPlayPatternButton(UnityEngine.Events.UnityAction ClickHandler) {
      _ButtonPlayPattern.onClick.RemoveAllListeners();
      _ButtonPlayPattern.gameObject.SetActive(true);
      _ButtonPlayPattern.Initialize(ClickHandler, "simon.PlayPattern", "next_round_of_play_continue_button");
    }

    public void HidePlayPatternButton() {
      _ButtonPlayPattern.gameObject.SetActive(false);
      _ButtonPlayPattern.onClick.RemoveAllListeners();
    }
  }
}