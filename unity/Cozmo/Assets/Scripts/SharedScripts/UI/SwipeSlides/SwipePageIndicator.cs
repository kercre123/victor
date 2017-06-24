using UnityEngine;
using System.Collections.Generic;

public class SwipePageIndicator : MonoBehaviour {

  public System.Action OnNextButton;
  public System.Action OnBackButton;

  [SerializeField]
  private bool _UsePagePips;

  [SerializeField]
  private UnityEngine.UI.Image _PageIndicatorPrefab;

  [SerializeField]
  private RectTransform _PageIndicatorContainer;

  [SerializeField]
  private bool _UsePageText;

  [SerializeField]
  private CozmoText _PageIndicatorText;

  [SerializeField]
  private Sprite _ActiveSprite;

  [SerializeField]
  private Sprite _InactiveSprite;

  [SerializeField]
  private Cozmo.UI.CozmoButton _NextButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _BackButton;

  private List<UnityEngine.UI.Image> _PageIndicators = new List<UnityEngine.UI.Image>();
  private int _PageCount;
  private int _CurrentPage;

  private void Awake() {
    _NextButton.Initialize(() => {
      if (OnNextButton != null) {
        OnNextButton();
      }
    }, "next_button", "swipe_page_indicator");

    _BackButton.Initialize(() => {
      if (OnBackButton != null) {
        OnBackButton();
      }
    }, "back_button", "swipe_page_indicator");
  }

  public void SetPageCount(int pageCount) {
    _PageCount = pageCount;

    if (_UsePagePips) {
      for (int i = 0; i < _PageIndicators.Count; ++i) {
        GameObject.Destroy(_PageIndicators[i].gameObject);
      }
      _PageIndicators.Clear();

      for (int i = 0; i < pageCount; ++i) {
        GameObject pageIndicator = GameObject.Instantiate(_PageIndicatorPrefab.gameObject);
        pageIndicator.GetComponent<UnityEngine.UI.Image>().overrideSprite = _InactiveSprite;
        pageIndicator.transform.SetParent(_PageIndicatorContainer, false);
        _PageIndicators.Add(pageIndicator.GetComponent<UnityEngine.UI.Image>());
      }
    }
    if (_UsePageText) {
      _PageIndicatorText.FormattingArgs = new object[] { 1, _PageCount };
    }
  }

  public void SetCurrentPage(int currentPage) {
    if (currentPage < 0 || currentPage >= _PageCount) {
      DAS.Error("SwipePageIndicator.SetCurrentPage", "index out of range");
      return;
    }

    _BackButton.Interactable = true;
    _NextButton.Interactable = true;

    if (currentPage == 0) {
      _BackButton.Interactable = false;
    }

    if (currentPage == _PageCount - 1) {
      _NextButton.Interactable = false;
    }

    if (_UsePagePips) {
      for (int i = 0; i < _PageCount; ++i) {
        _PageIndicators[i].overrideSprite = _InactiveSprite;
      }
      _PageIndicators[currentPage].overrideSprite = _ActiveSprite;
    }
    if (_UsePageText) {
      _PageIndicatorText.FormattingArgs = new object[] { (currentPage + 1), _PageCount };
    }
  }
}
