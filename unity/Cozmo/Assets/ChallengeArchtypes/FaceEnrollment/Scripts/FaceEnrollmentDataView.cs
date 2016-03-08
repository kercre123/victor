using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class FaceEnrollmentDataView : MonoBehaviour {

  public System.Action OnEnrollNewFace;

  [SerializeField]
  private Button _EnrollNewFaceButton;

  void Start() {
    _EnrollNewFaceButton.onClick.AddListener(HandleEnrollNewFaceButton);
  }

  private void HandleEnrollNewFaceButton() {
    OnEnrollNewFace();
  }

  void Update() {
	  
  }
}
