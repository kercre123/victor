using UnityEngine;
using System.Collections.Generic;
using Cozmo.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentListSlide : MonoBehaviour {

    [SerializeField]
    private FaceEnrollmentCell _FaceCellPrefab;

    [SerializeField]
    private GameObject _FaceCellEmptyPrefab;

    [SerializeField]
    private RectTransform _ContentContainer;

    private List<FaceEnrollmentCell> _FaceCellList = new List<FaceEnrollmentCell>();
    private List<GameObject> _EmptyCellList = new List<GameObject>();
    private FaceEnrollmentGame _FaceEnrollmentGame;

    public void Initialize(Dictionary<int, string> faceDatabase, FaceEnrollmentGame faceEnrollmentGame) {

      _FaceEnrollmentGame = faceEnrollmentGame;

      foreach (KeyValuePair<int, string> kvp in faceDatabase) {
        FaceEnrollmentCell faceCellInstance = GameObject.Instantiate(_FaceCellPrefab.gameObject).GetComponent<FaceEnrollmentCell>();
        faceCellInstance.transform.SetParent(_ContentContainer, false);
        faceCellInstance.Initialize(kvp.Key, kvp.Value);
        _FaceCellList.Add(faceCellInstance);
        faceCellInstance.OnDetailsViewRequested += _FaceEnrollmentGame.HandleDetailsViewRequested;
      }

      for (int i = _FaceCellList.Count; i < UnlockablesManager.Instance.FaceSlotsSize(); ++i) {
        GameObject faceCellEmptyInstance = GameObject.Instantiate(_FaceCellEmptyPrefab);
        faceCellEmptyInstance.transform.SetParent(_ContentContainer, false);
        _EmptyCellList.Add(faceCellEmptyInstance);
      }

    }

    private void HandleEnrolledFace(Anki.Cozmo.ExternalInterface.FaceEnrollmentCompleted message) {
      RefreshList(RobotEngineManager.Instance.CurrentRobot.EnrolledFaces);
    }

    private void Start() {
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.OnEnrolledFaceComplete += HandleEnrolledFace;
      }
    }

    private void OnDestroy() {
      IRobot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.OnEnrolledFaceComplete -= HandleEnrolledFace;
      }
    }

    private void ClearList() {
      for (int i = 0; i < _EmptyCellList.Count; ++i) {
        GameObject.Destroy(_EmptyCellList[i].gameObject);
      }

      for (int i = 0; i < _FaceCellList.Count; ++i) {
        _FaceCellList[i].OnDetailsViewRequested -= _FaceEnrollmentGame.HandleDetailsViewRequested;
        GameObject.Destroy(_FaceCellList[i].gameObject);
      }
      _EmptyCellList.Clear();
      _FaceCellList.Clear();
    }

    public void RefreshList(Dictionary<int, string> faceDatabase) {
      ClearList();
      Initialize(faceDatabase, _FaceEnrollmentGame);
    }


  }
}