using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class MinesweeperCornerCube : MonoBehaviour {
  [SerializeField]
  private Image _Image;

  public enum Corner {
    TopLeft,
    BottomRight,
    TopRight,
    BottomLeft
  }

  public void SetCorner(Corner corner, Color color) {
    var rectTransform = GetComponent<RectTransform>();

    switch (corner) {
    case Corner.TopLeft:
      rectTransform.anchorMin = rectTransform.anchorMax = Vector2.up;
      break;
    case Corner.BottomRight:
      rectTransform.anchorMin = rectTransform.anchorMax = Vector2.one;
      break;
    case Corner.TopRight:
      rectTransform.anchorMin = rectTransform.anchorMax = Vector2.right;
      break;
    case Corner.BottomLeft:
      rectTransform.anchorMin = rectTransform.anchorMax = Vector2.zero;
      break;
    }
    rectTransform.anchoredPosition = Vector2.zero;

    _Image.color = color;
  }
}
