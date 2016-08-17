using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class FaceEnrollmentListSlide : MonoBehaviour {

  public System.Action OnEnrollNewFaceRequested;
  public System.Action<int, string> OnEditNameRequested;
  public System.Action<int> OnDeleteEnrolledFace;
  public System.Action<int, string> OnReEnrollFaceRequested;

  [SerializeField]
  private FaceEnrollmentCell _FaceCellPrefab;

  [SerializeField]
  private RectTransform _ContentContainer;

  [SerializeField]
  private FaceEnrollmentNewCell _EnrollNewFacePrefab;
  private FaceEnrollmentNewCell _EnrollNewFaceInstance;

  private List<FaceEnrollmentCell> _FaceCellList = new List<FaceEnrollmentCell>();

  public void Initialize(Dictionary<int, string> faceDatabase) {
    foreach (KeyValuePair<int, string> kvp in faceDatabase) {
      FaceEnrollmentCell newFaceCell = GameObject.Instantiate(_FaceCellPrefab.gameObject).GetComponent<FaceEnrollmentCell>();
      newFaceCell.transform.SetParent(_ContentContainer, false);
      newFaceCell.Initialize(kvp.Key, kvp.Value);
      newFaceCell.OnEditNameRequested += HandleEditNameRequested;
      newFaceCell.OnDeleteNameRequested += HandleDeleteFace;
      newFaceCell.OnReEnrollFaceRequested += HandleReEnrollFaceRequested;
      _FaceCellList.Add(newFaceCell);
    }
    _EnrollNewFaceInstance = GameObject.Instantiate(_EnrollNewFacePrefab.gameObject).GetComponent<FaceEnrollmentNewCell>();
    _EnrollNewFaceInstance.transform.SetParent(_ContentContainer, false);
    _EnrollNewFaceInstance.OnCreateNewButton += HandleNewEnrollmentRequested;
  }

  public void ClearList() {
    if (_EnrollNewFaceInstance != null) {
      GameObject.Destroy(_EnrollNewFaceInstance.gameObject);
    }

    for (int i = 0; i < _FaceCellList.Count; ++i) {
      GameObject.Destroy(_FaceCellList[i].gameObject);
    }
    _FaceCellList.Clear();
  }

  public void RefreshList(Dictionary<int, string> faceDatabase) {
    ClearList();
    Initialize(faceDatabase);
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

  private void HandleReEnrollFaceRequested(int faceID, string faceName) {
    if (OnReEnrollFaceRequested != null) {
      OnReEnrollFaceRequested(faceID, faceName);
    }
  }

  private void HandleDeleteFace(int faceID) {
    if (OnDeleteEnrolledFace != null) {
      OnDeleteEnrolledFace(faceID);
    }
  }
}
