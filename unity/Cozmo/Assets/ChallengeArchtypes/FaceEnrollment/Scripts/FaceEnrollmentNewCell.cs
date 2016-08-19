using UnityEngine;
using System.Collections;

public class FaceEnrollmentNewCell : MonoBehaviour {

  public System.Action OnCreateNewButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _CreateNewButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _CreateNewButtonFace;

  private void Awake() {
    _CreateNewButtonFace.Initialize(HandleEnrollNewFaceButton, "create_new_face_button", "face_enrollment_new_cell");
    _CreateNewButton.Initialize(HandleEnrollNewFaceButton, "create_new_button", "face_enrollment_new_cell");
  }

  private void HandleEnrollNewFaceButton() {
    if (OnCreateNewButton != null) {
      OnCreateNewButton();
    }
  }

}
