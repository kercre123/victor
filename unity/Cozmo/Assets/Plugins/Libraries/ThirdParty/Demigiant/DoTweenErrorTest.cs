using UnityEngine;
using DG.Tweening;

public class DoTweenErrorTest : MonoBehaviour {
  private void Start() {
    transform.DOMove(Vector3.one, 2.0f);
    GameObject.Destroy(this.gameObject);
  }
}
