using UnityEngine;
using System.Collections;
using UnityEngine.EventSystems;
using System;

public class RotateHandle : MonoBehaviour, IDragHandler, IBeginDragHandler, IEndDragHandler {

  public event Action<float> OnRotationUpdated;

  private float _Radius;

  private float _Velocity;

  private float _LastAngle;
  private float _LastTime;

  private bool _IsDragging;

  private void Awake() {
    _Radius = transform.localPosition.magnitude;
  }

  #region IDragHandler implementation

  public void OnDrag(PointerEventData eventData) {
    
    var position = ((Vector3)eventData.position - transform.parent.position).normalized * _Radius;

    transform.localPosition = position;

    if (OnRotationUpdated != null) {
      var angle = Mathf.Atan2(position.y, position.x) * Mathf.Rad2Deg;

      var time = Time.time;

      var deltaTime = time - _LastTime;

      if (deltaTime > Mathf.Epsilon) {
        var angleDelta = (angle - _LastAngle);
        // make sure our angle delta doesn't wrap
        while (angleDelta > 360f) {
          angleDelta -= 360f;
        }
        while (angleDelta < -360f) {
          angleDelta += 360f;
        }
        _Velocity = angleDelta / deltaTime;
      }

      _LastTime = time;
      _LastAngle = angle;

      OnRotationUpdated(angle);
    }
  }

  #endregion

  #region IBeginDragHandler implementation

  public void OnBeginDrag(PointerEventData eventData) {
    _IsDragging = true;
  }

  #endregion

  #region IEndDragHandler implementation

  public void OnEndDrag(PointerEventData eventData) {
    _IsDragging = false;
  }

  #endregion

  private void Update() {
    if (!_IsDragging) {
      var position = transform.localPosition;
      float currentAngle = Mathf.Atan2(position.y, position.x) * Mathf.Rad2Deg;
      float closestAngle = Mathf.Round(currentAngle / 90f) * 90f;

      float newAngle = Mathf.SmoothDampAngle(currentAngle, closestAngle, ref _Velocity, Time.smoothDeltaTime);

      var newPosition = new Vector3(Mathf.Cos(newAngle * Mathf.Deg2Rad), Mathf.Sin(newAngle * Mathf.Deg2Rad), 0) * _Radius;

      _LastTime = Time.time;
      _LastAngle = newAngle;

      if (OnRotationUpdated != null) {
        OnRotationUpdated(newAngle);
      }

      transform.localPosition = newPosition;
    }
  }
}
