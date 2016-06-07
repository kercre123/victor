using UnityEngine;
using System.Collections;

public class GoalCellHorizontalBar : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Image _HorizontalMarker;

  public void SetHorizontalMarker(bool enable) {
    _HorizontalMarker.enabled = enable;
  }
}
