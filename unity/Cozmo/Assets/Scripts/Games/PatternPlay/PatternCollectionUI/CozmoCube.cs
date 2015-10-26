using UnityEngine;
using System.Collections;

public class CozmoCube : MonoBehaviour {

  public SolidColor leftColor;
  public SolidColor rightColor;
  public SolidColor frontColor;
  public SolidColor backColor;

  public void SetOrientation(bool facingCozmo) {
    Vector3 newRotation = new Vector3 (facingCozmo ? -140 : -50,
                                       0,
                                       0);
    transform.localRotation = Quaternion.Euler(newRotation);
  }
}
