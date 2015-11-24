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

  public Camera WorldCamera {
    get { return _WorldCamera; }
    private set { _WorldCamera = value; }
  }

  private void Start() {
    _HubWorldCameraInstance = this;
  }

  public Tweener CenterCameraOnTarget(Vector3 targetWorldPos) {
    return CenterCameraOnTarget(targetWorldPos, new Vector3(0.5f, 0.5f, 0));
  }

  public Tweener CenterCameraOnTarget(Vector3 targetWorldPos, Vector3 cameraCenterViewportPos) {
    return null;
  }

  public Tweener ReturnCameraToDefault() {
    return null;
  }
}
