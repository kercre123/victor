using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Simon {
  public class SimonLivesWidget : MonoBehaviour {

    [SerializeField]
    private Image _PortraitImage;

    [SerializeField]
    private LayoutElement _TextContainer;

    [SerializeField]
    private AnkiTextLabel _BigTextLabel;

    [SerializeField]
    private LayoutElement _LivesContainer;

    [SerializeField]
    private SegmentedBar _LivesCountBar;

    [SerializeField]
    private Image _Background;

    [SerializeField]
    private GameObject _LblMyTurn;

    private CanvasGroup _AlphaController;

    public void UpdateStatus(int lives, int maxLives, string statusLocKey) {
      _LivesCountBar.SetMaximumSegments(maxLives);
      _LivesCountBar.SetCurrentNumSegments(lives);
      _BigTextLabel.text = statusLocKey == "" ? "" : Localization.Get(statusLocKey);
    }
    // Just fade out the previous stuff.
    public void SetTurn(bool isMyTurn) {
      _LblMyTurn.SetActive(isMyTurn);
      if (_AlphaController == null) {
        _AlphaController = GetComponent<CanvasGroup>();
      }
      if (isMyTurn) {
        transform.localScale = new Vector3(1, 1, 1);
        _AlphaController.alpha = 1.0f;
      }
      else {
        transform.localScale = new Vector3(0.5f, 0.5f, 1);
        _AlphaController.alpha = 0.5f;
      }
    }
  }
}