using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class FaceEnrollmentDataView : MonoBehaviour {

  public System.Action OnEnrollNewFace;

  [SerializeField]
  private Cozmo.UI.CozmoButton _EnrollNewFaceButton;

  void Start() {
    _EnrollNewFaceButton.onClick.AddListener(HandleEnrollNewFaceButton);
    _EnrollNewFaceButton.DASEventButtonName = "enroll_new_face_button";
    _EnrollNewFaceButton.DASEventViewController = "enroll_data_view";
  }

  private void HandleEnrollNewFaceButton() {
    OnEnrollNewFace();
  }

  void Update() {
	  
  }
}
