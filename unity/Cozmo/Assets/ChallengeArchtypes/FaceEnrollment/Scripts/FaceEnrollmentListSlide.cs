using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class FaceEnrollmentListSlide : MonoBehaviour {

  [SerializeField]
  private FaceEnrollmentCell _FaceCellPrefab;

  private List<FaceEnrollmentCell> _FaceCellList = new List<FaceEnrollmentCell>();

  public void Initialize(List<string> initialNameList) {
    for (int i = 0; i < initialNameList.Count; ++i) {
      FaceEnrollmentCell newFaceCell = GameObject.Instantiate(_FaceCellPrefab.gameObject).GetComponent<FaceEnrollmentCell>();
      newFaceCell.transform.SetParent(transform, false);
      newFaceCell.Initialize(initialNameList[i]);
      _FaceCellList.Add(newFaceCell);
    }
  }
}
