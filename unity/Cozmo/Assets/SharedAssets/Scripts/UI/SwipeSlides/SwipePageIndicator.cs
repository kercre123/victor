using UnityEngine;
using System.Collections.Generic;

public class SwipePageIndicator : MonoBehaviour {

  public System.Action OnNextButton;
  public System.Action OnBackButton;

  [SerializeField]
  private UnityEngine.UI.Image _PageIndicatorPrefab;

  [SerializeField]
  private RectTransform _PageIndicatorContainer;

  [SerializeField]
  private Sprite _ActiveSprite;

  [SerializeField]
  private Sprite _InactiveSprite;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _NextButton;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _BackButton;

  private List<UnityEngine.UI.Image> _PageIndicators = new List<UnityEngine.UI.Image>();

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

  public void SetCurrentPage(int currentPage) {
    if (currentPage < 0 || currentPage >= _PageIndicators.Count) {
      DAS.Error("SwipePageIndicator.SetCurrentPage", "index out of range");
      return;
    }

    _BackButton.Interactable = true;
    _NextButton.Interactable = true;

    if (currentPage == 0) {
      _BackButton.Interactable = false;
    }

    if (currentPage == _PageIndicators.Count - 1) {
      _NextButton.Interactable = false;
    }

    for (int i = 0; i < _PageIndicators.Count; ++i) {
      _PageIndicators[i].overrideSprite = _InactiveSprite;
    }
    _PageIndicators[currentPage].overrideSprite = _ActiveSprite;
  }
}
