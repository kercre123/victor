using UnityEngine;
using System.Collections.Generic;

public class SwipePageIndicator : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.Image _PageIndicatorPrefab;

  [SerializeField]
  private RectTransform _PageIndicatorContainer;

  [SerializeField]
  private Sprite _ActiveSprite;

  [SerializeField]
  private Sprite _InactiveSprite;

  private List<UnityEngine.UI.Image> _PageIndicators = new List<UnityEngine.UI.Image>();

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
    for (int i = 0; i < _PageIndicators.Count; ++i) {
      _PageIndicators[i].overrideSprite = _InactiveSprite;
    }
    _PageIndicators[currentPage].overrideSprite = _ActiveSprite;
  }
}
