using UnityEngine;
using System.Collections.Generic;
using UnityEngine.UI;

public class SwipePageIndicator : MonoBehaviour {

  public System.Action OnNextButton;
  public System.Action OnBackButton;

  [SerializeField]
  private bool _UsePagePips;

  [SerializeField]
  private Image _PageIndicatorPrefab;

  [SerializeField]
  private RectTransform _PageIndicatorContainer;

  [SerializeField]
  private bool _UsePageText;

  [SerializeField]
  private CozmoText _PageIndicatorText;

  [SerializeField]
  private Sprite _ActivePipSprite;

  [SerializeField]
  private Sprite _InactivePipSprite;

  [SerializeField]
  private Cozmo.UI.CozmoButton _NextButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _BackButton;

  [SerializeField]
  private Image _BackArrowImage;

  [SerializeField]
  private Image _NextArrowImage;

  [SerializeField]
  private Sprite _InactiveArrowSprite;

  [SerializeField]
  private Sprite _ActiveArrowSprite;

  private List<Image> _PageIndicators = new List<Image>();
  private int _PageCount;

  private void Awake() {
    if (_NextButton != null) {
      _NextButton.Initialize(() => {
        if (OnNextButton != null) {
          OnNextButton();
        }
      }, "next_button", "swipe_page_indicator");
    }

    if (_BackButton != null) {
      _BackButton.Initialize(() => {
        if (OnBackButton != null) {
          OnBackButton();
        }
      }, "back_button", "swipe_page_indicator");
    }
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
        pageIndicator.GetComponent<Image>().overrideSprite = _InactivePipSprite;
        pageIndicator.transform.SetParent(_PageIndicatorContainer, false);
        _PageIndicators.Add(pageIndicator.GetComponent<Image>());
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

    if (_BackButton == null || _NextButton == null || _BackArrowImage == null || _NextArrowImage == null) {
      DAS.Error("SwipePageIndicator.SetCurrentPage", "Back or Next button was not properly set up in prefab!");
      return;
    }

    // Using overrideSprite because just sprite has problems updating on first run
    _BackButton.Interactable = true;
    _BackArrowImage.overrideSprite = _ActiveArrowSprite;
    _NextButton.Interactable = true;
    _NextArrowImage.sprite = _ActiveArrowSprite;

    if (currentPage == 0) {
      _BackButton.Interactable = false;
      _BackArrowImage.overrideSprite = _InactiveArrowSprite;
    }

    if (currentPage == _PageCount - 1) {
      _NextButton.Interactable = false;
      _NextArrowImage.sprite = _InactiveArrowSprite;
    }

    if (_UsePagePips) {
      for (int i = 0; i < _PageCount; ++i) {
        _PageIndicators[i].overrideSprite = _InactivePipSprite;
      }
      _PageIndicators[currentPage].overrideSprite = _ActivePipSprite;
    }
    if (_UsePageText) {
      _PageIndicatorText.FormattingArgs = new object[] { (currentPage + 1), _PageCount };
    }
  }
}
