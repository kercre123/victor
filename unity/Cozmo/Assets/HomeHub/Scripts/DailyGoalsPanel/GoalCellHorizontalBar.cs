using UnityEngine;
using System.Collections;

public class GoalCellHorizontalBar : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Image _HorizontalMarker;

  public void SetHorizontalMarker(bool enable) {
    if (_HorizontalMarker != null) {
      _HorizontalMarker.enabled = enable;
    }
  }
}
