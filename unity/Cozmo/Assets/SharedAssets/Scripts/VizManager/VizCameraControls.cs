using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.Cozmo;

[RequireComponent(typeof(RawImage))]
public class VizCameraControls : MonoBehaviour {

  private RectTransform _RectTransform;

  private Dictionary<int, FingerState> _FingerStates = new Dictionary<int, FingerState>();

  private Vector2 _LastTouchCenter;
  private int _LastTouchCount;

  private Camera _myCamera;

  private const float _kDragScale = 0.001f;

  private const float _kRotateScale = 0.2f;

  private const float _kZoomScale = 0.1f;

  private void Awake() {
    _RectTransform = GetComponent<RectTransform>();
  }

  private void Start() {
    Canvas highestCanvas = null;

    Transform t = transform;
    while (t.parent != null) {
      t = t.parent;

      var canvas = t.GetComponent<Canvas>();
      if (canvas != null) {
        highestCanvas = canvas;
      }
    }

    if (highestCanvas != null) {
      _myCamera = highestCanvas.worldCamera;
    }
  }
	
  private class FingerState {
    public bool StartedInside;
    public Vector2 CurrentPosition;
    public Vector2 LastPosition;
    public Vector2 StartPosition;
  }

	// Update is called once per frame
	private void Update () {

    #if UNITY_IOS && !UNITY_EDITOR
    ProcessTouches();
    #else
    ProcessMouseInput();
    #endif
  }

  private void ProcessMouseInput() {
    var cameraTransform = VizManager.Instance.Camera.transform;

    bool left = Input.GetMouseButton(0);

    bool right = Input.GetMouseButton(1);

    var scrollWheel = Input.GetAxis("Mouse ScrollWheel");

    Vector2 localPoint;
    RectTransformUtility.ScreenPointToLocalPointInRectangle(_RectTransform, Input.mousePosition, _myCamera, out localPoint);

    if (left || right) {
      if (_LastTouchCount == 1) {
        var delta = _LastTouchCenter - localPoint;

        if (left) {
          var scaledDelta = delta * _kDragScale * cameraTransform.localPosition.magnitude;

          var cRight = cameraTransform.InverseTransformVector(cameraTransform.right).normalized;
          var cUp = cameraTransform.InverseTransformVector(cameraTransform.up).normalized;

          cameraTransform.localPosition += cRight * scaledDelta.x + cUp * scaledDelta.y;
        }
        else {
          var scaledDelta = (Vector3)(delta * _kRotateScale);
          var rotation = Quaternion.Euler(scaledDelta.y, -scaledDelta.x, 0);
          cameraTransform.localRotation *= rotation;
        }
      }
      else if(_LastTouchCount == 0) {
        if (_RectTransform.rect.Contains(localPoint)) {
          _LastTouchCount = 1;
        }
        else {
          _LastTouchCount = -1;
        }
      }
    }
    else {
      _LastTouchCount = 0;
    }

    _LastTouchCenter = localPoint;

    float zoom = scrollWheel * _kZoomScale;
    var cForward = cameraTransform.InverseTransformVector(cameraTransform.forward).normalized;
    cameraTransform.localPosition += cForward * zoom;
  }

  private void ProcessTouches() {
    var cameraTransform = VizManager.Instance.Camera.transform;
    var touches = UnityEngine.Input.touches;

    int touchCount = 0;
    Vector2 touchCenter = Vector2.zero;

    FingerState first = null;
    FingerState second = null;

    for (int i = 0; i < touches.Length; i++) {  
      var touch = touches[i];
      FingerState state;
      if (!_FingerStates.TryGetValue(touch.fingerId, out state)) {
        state = new FingerState();
        _FingerStates.Add(touch.fingerId, state);
      }

      Vector2 localPoint;
      RectTransformUtility.ScreenPointToLocalPointInRectangle(_RectTransform, touch.position, _myCamera, out localPoint);

      Debug.Log(localPoint);

      if (touch.phase == TouchPhase.Began) {
        state.StartPosition = localPoint;
        state.CurrentPosition = localPoint;

        state.StartedInside = _RectTransform.rect.Contains(localPoint);
      }

      if (touch.phase == TouchPhase.Ended) {
        _FingerStates.Remove(touch.fingerId);
        continue;
      }

      state.LastPosition = state.CurrentPosition;
      state.CurrentPosition = localPoint;

      if (touchCount == 0) {
        first = state;
      }
      if (touchCount == 1) {
        second = state;
      }
      touchCount++;
      touchCenter += localPoint;
    }

    if (touchCount == 0) {
      return;
    }

    if (touchCount == 1) {
      var scaledDelta = ((first.LastPosition - first.CurrentPosition) * _kDragScale) * cameraTransform.localPosition.magnitude;

      var cRight = cameraTransform.InverseTransformVector(cameraTransform.right).normalized;
      var cUp = cameraTransform.InverseTransformVector(cameraTransform.up).normalized;

      cameraTransform.localPosition += cRight * scaledDelta.x + cUp * scaledDelta.y;
      return;
    }

    if (touchCount == 2) {
      var currentSize = first.CurrentPosition - second.CurrentPosition;
      var lastSize = first.LastPosition - second.LastPosition;

      if (!lastSize.Approximately(Vector2.zero)) {

        var zoom = _kZoomScale * (currentSize.magnitude / lastSize.magnitude - 1);

        var cForward = cameraTransform.InverseTransformVector(cameraTransform.forward).normalized;
        cameraTransform.localPosition += cForward * zoom;
      }
    }
    else {
      touchCenter /= touchCount;

      if (touchCount == _LastTouchCount) {
        var delta = (_LastTouchCenter - touchCenter) * _kRotateScale;

        var rotation = Quaternion.Euler(delta.y, -delta.x, 0);
        VizManager.Instance.Camera.transform.localRotation *= rotation;
      }

      _LastTouchCount = touchCount;
      _LastTouchCenter = touchCenter;
    }


	}


}
