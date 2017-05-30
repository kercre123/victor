using UnityEngine;

public class Spinner : MonoBehaviour {
  [Tooltip("in degrees per second")]
  [SerializeField]
  private float _SpinSpeed;

  void Update() {
    gameObject.transform.Rotate(new Vector3(0.0f, 0.0f, _SpinSpeed * Time.deltaTime));
  }
}
