using UnityEngine;
using System.Collections;

public class FaceEnrollmentCell : MonoBehaviour {

  public System.Action<string> OnEditNameRequested;

  private string _FaceName;

  public void Initialize(string faceName) {
    _FaceName = faceName;
  }

  private void HandleEditNameClicked() {
    if (OnEditNameRequested != null) {
      OnEditNameRequested(_FaceName);
    }
  }
}
