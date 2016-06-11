using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class FaceEnrollmentListSlide : MonoBehaviour {

  public System.Action OnEnrollNewFaceRequested;
  public System.Action<int, string> OnEditNameRequested;

  [SerializeField]
  private FaceEnrollmentCell _FaceCellPrefab;

  [SerializeField]
  private Cozmo.UI.CozmoButton _EnrollNewFacePrefab;
  private Cozmo.UI.CozmoButton _EnrollNewFaceInstance;

  private List<FaceEnrollmentCell> _FaceCellList = new List<FaceEnrollmentCell>();

  public void Initialize(Dictionary<int, string> faceDatabase) {
    foreach (KeyValuePair<int, string> kvp in faceDatabase) {
      FaceEnrollmentCell newFaceCell = GameObject.Instantiate(_FaceCellPrefab.gameObject).GetComponent<FaceEnrollmentCell>();
      newFaceCell.transform.SetParent(transform, false);
      newFaceCell.Initialize(kvp.Key, kvp.Value);
      newFaceCell.OnEditNameRequested += HandleEditNameRequested;
      _FaceCellList.Add(newFaceCell);
    }
    _EnrollNewFaceInstance = GameObject.Instantiate(_EnrollNewFacePrefab.gameObject).GetComponent<Cozmo.UI.CozmoButton>();
    _EnrollNewFaceInstance.transform.SetParent(transform, false);
    _EnrollNewFaceInstance.Initialize(HandleNewEnrollmentRequested, "enroll_new_face", "face_enrollment_list_slide");
  }

  private void HandleEditNameRequested(int faceID, string faceName) {
    if (OnEditNameRequested != null) {
      OnEditNameRequested(faceID, faceName);
    }
  }

  private void HandleNewEnrollmentRequested() {
    if (OnEnrollNewFaceRequested != null) {
      OnEnrollNewFaceRequested();
    }
  }
}
