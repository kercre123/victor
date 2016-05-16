using UnityEngine;
using System.Collections;

public class Spinner : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Image _ImageToSpin;

  [Tooltip("in degrees per second")]
  [SerializeField]
  private float _SpinSpeed;

  void Update() {
    _ImageToSpin.transform.Rotate(new Vector3(0.0f, 0.0f, _SpinSpeed * Time.deltaTime));
  }
}
