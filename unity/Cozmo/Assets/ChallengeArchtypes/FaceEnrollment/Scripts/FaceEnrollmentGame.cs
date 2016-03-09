using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    public Dictionary<int, string> _FaceNameDictionary = new Dictionary<int, string>();
    public Dictionary<int, string> _FaceIDToReaction = new Dictionary<int, string>();

    private string[] _ReactionBank = {
      AnimationName.kFaceEnrollmentReaction_00,
      AnimationName.kFaceEnrollmentReaction_01,
      AnimationName.kFaceEnrollmentReaction_02,
      AnimationName.kFaceEnrollmentReaction_03
    };

    private int _ReactionIndex = 0;

    private float _LastPlayedReaction = 0.0f;
    private int _LastReactedID = 0;
    private bool _Reacting = false;

    [SerializeField]
    private FaceEnrollmentDataView _FaceEnrollmentDataViewPrefab;
    private FaceEnrollmentDataView _FaceEnrollmentDataView;

    [SerializeField]
    private FaceEnrollmentNameView _FaceEnrollmentViewPrefab;
    private FaceEnrollmentNameView _FaceEnrollmentView;

    private int _NewSeenFaceID = 0;
    private int _LastSeenFaceID = 0;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      ResetPose();
      RobotEngineManager.Instance.RobotObservedNewFace += HandleObservedNewFace;
      RobotEngineManager.Instance.RobotObservedFace += HandleOnAnyFaceSeen;
    }

    protected override void InitializeView(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.InitializeView(newView, data);

      _FaceEnrollmentDataView = 
        newView.ShowWideGameStateSlide(_FaceEnrollmentDataViewPrefab.gameObject, 
        "face_enrollment_data_view_panel").GetComponent<FaceEnrollmentDataView>();

      _FaceEnrollmentDataView.OnEnrollNewFace += LookForNewFaceToEnroll;
    }

    private void LookForNewFaceToEnroll() {
      ResetPose();
      CurrentRobot.EnableNewFaceEnrollment();
    }

    public void EnrollFace(string nameForFace) {
      _FaceNameDictionary.Add(_NewSeenFaceID, nameForFace);
      CurrentRobot.AssignNameToFace(_NewSeenFaceID, nameForFace);

      _FaceIDToReaction.Add(_NewSeenFaceID, _ReactionBank[_ReactionIndex]);
      _ReactionIndex = (_ReactionIndex + 1) % _ReactionBank.Length;
    }

    private void HandleObservedNewFace(int faceID, Vector3 facePos, Quaternion faceRot) {
      if (_FaceEnrollmentView != null) {
        return; // we already have a view open to ask for a name.
      }
      // I found a new face! let's get the name from the user.
      _FaceEnrollmentView = UIManager.OpenView<FaceEnrollmentNameView>(_FaceEnrollmentViewPrefab);
      _FaceEnrollmentView.OnSubmitButton += HandleSubmitButton;
      _NewSeenFaceID = faceID;
    }

    private void HandleSubmitButton(string name) {
      EnrollFace(name);
      CurrentRobot.SendAnimation(_FaceIDToReaction[_NewSeenFaceID], HandleReactionDone);
    }

    private void HandleReactionDone(bool success) {
      _NewSeenFaceID = 0;
      UIManager.CloseView(_FaceEnrollmentView);
      _FaceEnrollmentView = null;
      ResetPose();
    }

    private void ResetPose() {
      CurrentRobot.SetLiftHeight(0.0f);
      CurrentRobot.SetHeadAngle(0.5f);
    }

    private void HandleOnAnyFaceSeen(int faceID, string name, Vector3 pos, Quaternion rot) {
      // check if we have an active face enroll view open
      _LastSeenFaceID = faceID;
      if (_FaceEnrollmentView == null) {
        if (faceID > 0 && string.IsNullOrEmpty(name) == false && _FaceIDToReaction.ContainsKey(faceID)) {
          // this is a face we know...
          if (Time.time - _LastPlayedReaction > 10.0f || _LastReactedID != faceID && !_Reacting) {
            // been at least 10 seconds since we reacted or it's a new face.
            CurrentRobot.TurnTowardsFace(CurrentRobot.Faces.Find(x => x.ID == faceID), callback: FacePoseDone);
            _Reacting = true;
          }
        }
      }
    }

    private void FacePoseDone(bool success) {
      if (success) {
        CurrentRobot.SendAnimation(_FaceIDToReaction[_LastSeenFaceID], ReactToFaceAnimationDone);
        _LastReactedID = _LastSeenFaceID;
      }
      else {
        _Reacting = false;
      }
    }

    private void ReactToFaceAnimationDone(bool success) {
      ResetPose();
      _Reacting = false;
      _LastPlayedReaction = Time.time;
    }

    protected override void CleanUpOnDestroy() {
      RobotEngineManager.Instance.RobotObservedNewFace -= HandleObservedNewFace;
      RobotEngineManager.Instance.RobotObservedFace -= HandleOnAnyFaceSeen;
    }


  }

}
