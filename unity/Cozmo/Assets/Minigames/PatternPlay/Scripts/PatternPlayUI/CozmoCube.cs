using UnityEngine;
using System.Collections;

public class CozmoCube : MonoBehaviour {

  [SerializeField]
  private float _FacingCozmoAngle = -140f;

  [SerializeField]
  private float _AwayCozmoAngle = -50f;

  public SolidColor leftColor;
  public SolidColor rightColor;
  public SolidColor frontColor;
  public SolidColor backColor;

  public void SetOrientation(bool facingCozmo) {
    Vector3 newRotation = new Vector3(facingCozmo ? _FacingCozmoAngle : _AwayCozmoAngle,
                            0,
                            0);
    transform.localRotation = Quaternion.Euler(newRotation);
  }
}
