using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Anki.Cozmo.Viz {
  [RequireComponent(typeof(RawImage))]
  public class VizCameraControls : MonoBehaviour {

    private RectTransform _RectTransform;

    private Dictionary<int, FingerState> _FingerStates = new Dictionary<int, FingerState>();

    private Vector2 _LastTouchCenter;
    private int _LastTouchCount;

    private Camera _myCamera;

    [SerializeField]
    private Button _ResetButton;

    private const float _kDragScale = 0.001f;

    private const float _kRotateScale = 0.2f;

    #if UNITY_IOS && !UNITY_EDITOR
    private const float _kZoomScale = 1f;
    

#else
    private const float _kZoomScale = 0.1f;
    #endif

    private void Awake() {
      _RectTransform = GetComponent<RectTransform>();
      if (_ResetButton != null) {
        _ResetButton.onClick.AddListener(Reset);
      }
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

    private void OnEnable() {
      VizManager.Instance.Camera.enabled = true;
    }

    private void OnDisable() {
      VizManager.Instance.Camera.enabled = false;
    }

    private void Reset() {
      VizManager.Instance.ResetCamera();
    }

    private class FingerState {
      public bool StartedInside;
      public Vector2 CurrentPosition;
      public Vector2 LastPosition;
      public Vector2 StartPosition;
    }

    // Update is called once per frame
    private void Update() {

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

          if (right) {
            UpdatePan(cameraTransform, delta);
          }
          else {
            UpdateRotation(cameraTransform, delta);
          }
        }
        else if (_LastTouchCount == 0) {
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

      UpdateZoom(cameraTransform, scrollWheel);
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

        if (touch.phase == TouchPhase.Began) {
          state.StartPosition = localPoint;
          state.CurrentPosition = localPoint;

          state.StartedInside = _RectTransform.rect.Contains(localPoint);
        }

        if ((touch.phase == TouchPhase.Ended) || (state.StartedInside == false)) {
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
        _LastTouchCount = 0;
        return;
      }

      touchCenter /= touchCount;

      if (touchCount == 2) {
        var currentSize = first.CurrentPosition - second.CurrentPosition;
        var lastSize = first.LastPosition - second.LastPosition;

        if (!lastSize.Approximately(Vector2.zero)) {

          UpdateZoom(cameraTransform, (currentSize.magnitude / lastSize.magnitude - 1));

        }
      }

      if (touchCount == _LastTouchCount) {      
        var delta = (_LastTouchCenter - touchCenter);

        if (touchCount > 2) {
          UpdateRotation(cameraTransform, delta);
        }
        else {
          UpdatePan(cameraTransform, delta);
        }
      }

      _LastTouchCount = touchCount;
      _LastTouchCenter = touchCenter;
    }

    private void UpdatePan(Transform cameraTransform, Vector2 delta) {
      var scaledDelta = delta * _kDragScale * Mathf.Max(10, cameraTransform.localPosition.y);

      var cRight = cameraTransform.right;
      var cUp = cameraTransform.up;

      cameraTransform.position += cRight * scaledDelta.x + cUp * scaledDelta.y;
    }

    private void UpdateRotation(Transform cameraTransform, Vector2 delta) {
      var scaledDelta = (Vector3)(delta * _kRotateScale);

      if (!cameraTransform.forward.xz().Approximately(Vector2.zero)) {
        cameraTransform.forward = Quaternion.Euler(0, -scaledDelta.x, 0) * cameraTransform.forward;
      }
      else {
        cameraTransform.localRotation *= Quaternion.Euler(0, 0, -scaledDelta.x);
      }

      var lastRotation = cameraTransform.localRotation;
      var lastUp = cameraTransform.up;

      cameraTransform.localRotation *= Quaternion.Euler(scaledDelta.y, 0, 0);

      // don't allow going past the bottom, because we always want up to be positive
      if ((lastUp.y > 0) != (cameraTransform.up.y > 0)) {
        cameraTransform.localRotation = lastRotation;
      }
    }

    private void UpdateZoom(Transform cameraTransform, float zoomFactor) {
      var zoom = _kZoomScale * zoomFactor * Mathf.Max(10, cameraTransform.localPosition.y);

      var cForward = cameraTransform.forward;
      cameraTransform.position += cForward * zoom;

    }
  }
}