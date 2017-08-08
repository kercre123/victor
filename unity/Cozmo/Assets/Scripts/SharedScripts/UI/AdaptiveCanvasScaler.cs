using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class AdaptiveCanvasScaler : CanvasScaler {
  private const float _AspectAboveWhichWeScaleToHeight = 1.7778f; //greater than 16:9 aspect

  protected override void OnEnable() {
    RefreshScaler();
    base.OnEnable();
  }

  private void RefreshScaler() {
    float aspect = (float)Screen.width / (float)Screen.height;
    if (aspect > _AspectAboveWhichWeScaleToHeight) {
      matchWidthOrHeight = 1f;
    }
    else {
      matchWidthOrHeight = 0f;
    }
  }
}
