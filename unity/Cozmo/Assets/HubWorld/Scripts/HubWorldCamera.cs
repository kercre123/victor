using UnityEngine;
using System.Collections;
using DG.Tweening;

public class HubWorldCamera : MonoBehaviour {

  private static HubWorldCamera _HubWorldCameraInstance;

  public static HubWorldCamera Instance {
    get { return _HubWorldCameraInstance; }
    private set { _HubWorldCameraInstance = value; }
  }

  [SerializeField]
  private Camera _WorldCamera;

  private Vector3 _DefaultLocalPosition;

  public Camera WorldCamera {
    get { return _WorldCamera; }
    private set { _WorldCamera = value; }
  }

  private void Start() {
    _HubWorldCameraInstance = this;
    _DefaultLocalPosition = _WorldCamera.transform.localPosition;
  }

  public Tweener CenterCameraOnTarget(Vector3 targetWorldPos) {
    return CenterCameraOnTarget(targetWorldPos, new Vector3(0.5f, 0.5f, 400));
  }

  public Tweener CenterCameraOnTarget(Vector3 focusTargetWorldPos, Vector3 cameraFocusViewportPos) {
    Vector3 cameraFocusWorldPos = _WorldCamera.ViewportToWorldPoint(cameraFocusViewportPos);
    Vector3 distanceToMove = focusTargetWorldPos - cameraFocusWorldPos;
    Vector3 targetWorldPosition = _WorldCamera.transform.position + distanceToMove;
    return _WorldCamera.transform.DOMove(targetWorldPosition, 0.5f).SetEase(Ease.OutQuad);
  }

  public Tweener ReturnCameraToDefault() {
    return _WorldCamera.transform.DOLocalMove(_DefaultLocalPosition, 0.5f).SetEase(Ease.OutQuad);
  }
}
