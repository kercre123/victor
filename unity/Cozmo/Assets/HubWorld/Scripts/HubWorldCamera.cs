using UnityEngine;
using System.Collections;
using DG.Tweening;

public class HubWorldCamera : MonoBehaviour {

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
