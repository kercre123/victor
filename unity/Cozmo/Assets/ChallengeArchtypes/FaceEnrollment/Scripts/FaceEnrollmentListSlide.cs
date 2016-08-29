using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class FaceEnrollmentListSlide : MonoBehaviour {

  public System.Action<string> OnEnrollNewFaceRequested;
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

  [SerializeField]
  private FaceEnrollmentNewCell _LockedNewFaceSlotPrefab;
  private FaceEnrollmentNewCell _LockedNewFaceSlotInstance;

  [SerializeField]
  private FaceEnrollmentUnlockView _UnlockFaceCellViewPrefab;
  private FaceEnrollmentUnlockView _UnlockFaceCellViewInstance;

  private List<FaceEnrollmentCell> _FaceCellList = new List<FaceEnrollmentCell>();
  private List<FaceEnrollmentNewCell> _FaceNewCellList = new List<FaceEnrollmentNewCell>();

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

    for (int i = _FaceCellList.Count; i < UnlockablesManager.Instance.FaceSlotsSize(); ++i) {
      if (UnlockablesManager.Instance.IsFaceSlotUnlocked(i)) {
        // show enroll cell
        _EnrollNewFaceInstance = GameObject.Instantiate(_EnrollNewFacePrefab.gameObject).GetComponent<FaceEnrollmentNewCell>();
        _EnrollNewFaceInstance.transform.SetParent(_ContentContainer, false);
        _EnrollNewFaceInstance.OnCreateNewButton += HandleNewEnrollmentRequested;
        _FaceNewCellList.Add(_EnrollNewFaceInstance);
      }
      else {
        int faceSlot = i;
        // show locked cell, need to find next available locked face cell to unlock
        _LockedNewFaceSlotInstance = GameObject.Instantiate(_LockedNewFaceSlotPrefab.gameObject).GetComponent<FaceEnrollmentNewCell>();
        _LockedNewFaceSlotInstance.transform.SetParent(_ContentContainer, false);
        _LockedNewFaceSlotInstance.OnCreateNewButton += () => {
          _UnlockFaceCellViewInstance = UIManager.OpenView(_UnlockFaceCellViewPrefab);
          KeyValuePair<string, int> unlockCostInfo = UnlockablesManager.Instance.FaceUnlockCost(faceSlot);
          _UnlockFaceCellViewInstance.Initialize(unlockCostInfo.Key, unlockCostInfo.Value);
        };
        _FaceNewCellList.Add(_LockedNewFaceSlotInstance);
      }
    }
  }

  private void HandleUnlockResults(Anki.Cozmo.ExternalInterface.RequestSetUnlockResult message) {
    RefreshList(RobotEngineManager.Instance.CurrentRobot.EnrolledFaces);
  }

  private void Start() {
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleUnlockResults);
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleUnlockResults);
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

  private void ClearList() {
    for (int i = 0; i < _FaceNewCellList.Count; ++i) {
      GameObject.Destroy(_FaceNewCellList[i].gameObject);
    }

    for (int i = 0; i < _FaceCellList.Count; ++i) {
      GameObject.Destroy(_FaceCellList[i].gameObject);
    }
    _FaceNewCellList.Clear();
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

      // if this the first name then we should pre-populate the name with the profile name.
      if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
        OnEnrollNewFaceRequested(DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName);
      }
      else {
        OnEnrollNewFaceRequested("");
      }

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
