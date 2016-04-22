using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class FaceEnrollmentDataView : MonoBehaviour {

  public System.Action OnEnrollNewFace;

  [SerializeField]
  private Cozmo.UI.CozmoButton _EnrollNewFaceButton;

  void Start() {
    _EnrollNewFaceButton.Initialize(HandleEnrollNewFaceButton, "enroll_new_face_button", "face_enrollment_data_view");
  }

  private void HandleEnrollNewFaceButton() {
    OnEnrollNewFace();
  }
}
