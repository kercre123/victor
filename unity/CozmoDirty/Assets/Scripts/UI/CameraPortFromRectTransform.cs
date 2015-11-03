using UnityEngine;
using UnityEngine.UI;
using System.Collections;

[ExecuteInEditMode]
public class CameraPortFromRectTransform : MonoBehaviour {

  [SerializeField] RectTransform rectTransform;

  Camera cam;
  Canvas canvas;

  bool initialized = false;

  void OnEnable() {
    initialized = false;
  }

  void Update() {
    if (initialized)
      return;
    if (rectTransform == null)
      return;

    if (cam == null) {
      cam = gameObject.GetComponent<Camera>();
      if (cam == null)
        return;
    }

    if (canvas == null) {
      canvas = rectTransform.gameObject.GetComponentInParent<Canvas>();
      if (canvas == null)
        return;
    }

    if (!rectTransform.gameObject.activeInHierarchy)
      return;

    initialized = true;

    Vector3[] corners = new Vector3[4];
    rectTransform.GetWorldCorners(corners);

    //get corners in viewport space
    for (int i = 0; i < 4; i++) {
      corners[i] = RectTransformUtility.WorldToScreenPoint(canvas.renderMode != RenderMode.ScreenSpaceOverlay ? canvas.worldCamera : null, corners[i]);
      corners[i].x /= Screen.width;
      corners[i].y /= Screen.height;
    }

    float xMin = float.MaxValue;
    float xMax = 0f;
    float yMin = float.MaxValue;
    float yMax = 0f;

    for (int i = 0; i < 4; i++) {
      xMin = Mathf.Min(corners[i].x, xMin);
      xMax = Mathf.Max(corners[i].x, xMax);
      yMin = Mathf.Min(corners[i].y, yMin);
      yMax = Mathf.Max(corners[i].y, yMax);
    }

    Rect rect = new Rect(xMin, yMin, xMax - xMin, yMax - yMin);

    cam.rect = rect; 

  }
}
