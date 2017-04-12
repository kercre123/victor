using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace MemoryMatch {
  public class MemoryMatchLivesWidget : MonoBehaviour {

    [SerializeField]
    private Image _PortraitImage;

    [SerializeField]
    private LayoutElement _LivesContainer;

    [SerializeField]
    private SegmentedBar _LivesCountBar;

    [SerializeField]
    private Image _Background;

    [SerializeField]
    private Sprite _ActiveBackground;

    [SerializeField]
    private GameObject _LblMyTurn;

    private CanvasGroup _AlphaController;

    public void UpdateStatus(int lives, int maxLives) {
      _LivesCountBar.SetMaximumSegments(maxLives);
      _LivesCountBar.SetCurrentNumSegments(lives);
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
        if (_Background != null && _ActiveBackground != null) {
          _Background.overrideSprite = _ActiveBackground;
        }
      }
      else {
        transform.localScale = new Vector3(0.7f, 0.7f, 1);
        _AlphaController.alpha = 0.5f;
        if (_Background != null) {
          _Background.overrideSprite = null;
        }
      }
    }
  }
}