using UnityEngine;
using System.Collections;

public class FaceEnrollmentNewCell : MonoBehaviour {

  public System.Action OnCreateNewButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _CreateNewButton;

  private void Awake() {
    _CreateNewButton.Initialize(() => {
      if (OnCreateNewButton != null) {
        OnCreateNewButton();
      }
    }, "create_new_button", "face_enrollment_new_cell");
  }

}
