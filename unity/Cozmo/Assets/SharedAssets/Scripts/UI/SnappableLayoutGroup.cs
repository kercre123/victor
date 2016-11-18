using UnityEngine;
using System.Collections.Generic;

public class SnappableLayoutGroup : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.HorizontalOrVerticalLayoutGroup _LayoutGroup;

  private List<UnityEngine.UI.LayoutElement> _LayoutElementList = new List<UnityEngine.UI.LayoutElement>();

  private float _TotalPanelSize = 0.0f;
  private bool _IsHorizontal;

  public void AddLayoutElement(UnityEngine.UI.LayoutElement element) {
    _LayoutElementList.Add(element);
    _IsHorizontal = _LayoutGroup is UnityEngine.UI.HorizontalLayoutGroup;

    if (_IsHorizontal) {
      _TotalPanelSize += element.minWidth;
    }
    else {
      _TotalPanelSize += element.minHeight;
    }

  }

  public float GetNormalizedSnapValue(int index) {
    if (index > _LayoutElementList.Count) {
      DAS.Error("HomeViewTab.GoToIndex out of bounds", gameObject.name);
      return 0.0f;
    }

    float indexedValue = 0.0f;
    for (int i = 0; i < _LayoutElementList.Count; ++i) {
      if (i < index) {
        if (_IsHorizontal) {
          indexedValue += _LayoutElementList[i].minWidth;
        }
        else {
          indexedValue += _LayoutElementList[i].minHeight;
        }
      }
      else {
        break;
      }
    }
    float elementSize;

    if (_IsHorizontal) {
      elementSize = _LayoutElementList[index].minWidth;
    }
    else {
      elementSize = _LayoutElementList[index].minHeight;
    }

    float scrollNormalizedPosition = (indexedValue + elementSize / 2.0f) / _TotalPanelSize;

    return scrollNormalizedPosition;
  }
}
