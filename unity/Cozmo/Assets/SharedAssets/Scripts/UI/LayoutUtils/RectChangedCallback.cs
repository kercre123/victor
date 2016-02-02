using UnityEngine;
using System.Collections;
using System;

public class RectChangedCallback : MonoBehaviour {

  public Action OnRectChanged;

  private void OnRectTransformDimensionsChange() {
    if (OnRectChanged != null) {
      OnRectChanged();
    }
  }
}
