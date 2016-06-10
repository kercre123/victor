using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class FaceEnrollmentListSlide : MonoBehaviour {

  public System.Action OnEnrollNewFaceRequested;
  public System.Action<string> OnEditNameRequested;

  [SerializeField]
  private FaceEnrollmentCell _FaceCellPrefab;

  private List<FaceEnrollmentCell> _FaceCellList = new List<FaceEnrollmentCell>();

  public void Initialize(List<string> initialNameList) {
    for (int i = 0; i < initialNameList.Count; ++i) {
      FaceEnrollmentCell newFaceCell = GameObject.Instantiate(_FaceCellPrefab.gameObject).GetComponent<FaceEnrollmentCell>();
      newFaceCell.transform.SetParent(transform, false);
      newFaceCell.Initialize(initialNameList[i]);
      newFaceCell.OnEditNameRequested += HandleEditNameRequested;
      _FaceCellList.Add(newFaceCell);
    }
  }

  private void HandleEditNameRequested(string faceName) {
    if (OnEditNameRequested != null) {
      OnEditNameRequested(faceName);
    }
  }

  private void HandleNewEnrollmentRequested() {
    if (OnEnrollNewFaceRequested != null) {
      OnEnrollNewFaceRequested();
    }
  }
}
