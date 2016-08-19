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

  private long _UpdateSeenThresholdSeconds;
  private long _updateEnrolledThresholdSeconds;

  public void Initialize(Dictionary<int, string> faceDatabase, long seenThreshold, long enrolledThreshold) {
    _UpdateSeenThresholdSeconds = seenThreshold;
    _updateEnrolledThresholdSeconds = enrolledThreshold;

    foreach (KeyValuePair<int, string> kvp in faceDatabase) {
      FaceEnrollmentCell faceCellInstance = GameObject.Instantiate(_FaceCellPrefab.gameObject).GetComponent<FaceEnrollmentCell>();
      faceCellInstance.transform.SetParent(_ContentContainer, false);
      faceCellInstance.Initialize(kvp.Key, kvp.Value, NeedsUpdate(kvp.Key, seenThreshold, enrolledThreshold));
      faceCellInstance.OnEditNameRequested += HandleEditNameRequested;
      faceCellInstance.OnDeleteNameRequested += HandleDeleteFace;
      faceCellInstance.OnReEnrollFaceRequested += HandleReEnrollFaceRequested;
      _FaceCellList.Add(faceCellInstance);
    }
    _EnrollNewFaceInstance = GameObject.Instantiate(_EnrollNewFacePrefab.gameObject).GetComponent<FaceEnrollmentNewCell>();
    _EnrollNewFaceInstance.transform.SetParent(_ContentContainer, false);
    _EnrollNewFaceInstance.OnCreateNewButton += HandleNewEnrollmentRequested;
  }

  private bool NeedsUpdate(int faceID, long seenThreshold, long enrolledThreshold) {
    if (RobotEngineManager.Instance.CurrentRobot.EnrolledFacesLastSeenTime.ContainsKey(faceID) == false) {
      DAS.Error("FaceEnrollmentSlide.NeedsUpdate", "EnrolledFacesSecondsSinceSeen Dictionary does not contain ID " + faceID);
      return false;
    }

    if (RobotEngineManager.Instance.CurrentRobot.EnrolledFacesLastEnrolledTime.ContainsKey(faceID) == false) {
      DAS.Error("FaceEnrollmentSlide.NeedsUpdate", "EnrolledFacesSecondsSinceEnrolled Dictionary does not contain ID " + faceID);
      return false;
    }

    if (Time.time - RobotEngineManager.Instance.CurrentRobot.EnrolledFacesLastSeenTime[faceID] > seenThreshold) {
      return true;
    }

    if (Time.time - RobotEngineManager.Instance.CurrentRobot.EnrolledFacesLastEnrolledTime[faceID] > enrolledThreshold) {
      return true;
    }

    return false;
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
    Initialize(faceDatabase, _UpdateSeenThresholdSeconds, _updateEnrolledThresholdSeconds);
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
