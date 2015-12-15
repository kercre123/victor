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

  [SerializeField]
  private float _FrustrumWidth = 2000;

  private Vector3 _DefaultLocalPosition;

  public Camera WorldCamera {
    get { return _WorldCamera; }
    private set { _WorldCamera = value; }
  }

  private void Start() {
    _HubWorldCameraInstance = this;

    var ray = _WorldCamera.ViewportPointToRay(new Vector3(1, 1, 0));

    var zplane = new Plane(Vector3.back, Vector3.zero);

    float len;
    zplane.Raycast(ray, out len);

    var planeIntersection = ray.GetPoint(len);

    Debug.Log("Plane Intersection: " + planeIntersection);

    float xDelta = _FrustrumWidth / 2 - planeIntersection.x;

    float zDelta = xDelta * (_WorldCamera.transform.localPosition.z / planeIntersection.x);

    _DefaultLocalPosition = _WorldCamera.transform.localPosition;
    _DefaultLocalPosition.z += zDelta;
    _WorldCamera.transform.localPosition = _DefaultLocalPosition;
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
