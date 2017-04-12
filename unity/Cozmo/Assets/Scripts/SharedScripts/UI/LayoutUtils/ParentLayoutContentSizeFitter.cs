using UnityEngine;
using System.Collections;
using UnityEngine.UI;

// Similar to Unity's default ContentSizefitter but modifies
// the LayoutElement instead of modifying the RectTransform
// directly. This is useful if you want a child to expand to
// its content but still have it be under a parent with a layout group.
public class ParentLayoutContentSizeFitter : MonoBehaviour {

  public event System.Action OnResizedParent;

  private RectTransform _RectTransform;

  private void Start() {
    _RectTransform = GetComponent<RectTransform>();
  }

  private void OnRectTransformDimensionsChange() {
    if (transform.parent != null && gameObject.activeInHierarchy) {
      StartCoroutine(ResizeParent());
    }
  }

  IEnumerator ResizeParent() {
    yield return null;
    LayoutElement layoutElement = transform.parent.GetComponent<LayoutElement>();
    if (layoutElement != null) {
      layoutElement.minWidth = _RectTransform.rect.width;
      layoutElement.minHeight = _RectTransform.rect.height;
    }
    if (OnResizedParent != null) {
      OnResizedParent();
    }
  }
}
