using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    public Dictionary<int, string> _FaceNameDictionary = new Dictionary<int, string>();

    private string[] _ReactionBank = {
      AnimationName.kFaceEnrollmentReaction_00,
      AnimationName.kFaceEnrollmentReaction_01,
      AnimationName.kFaceEnrollmentReaction_02,
      AnimationName.kFaceEnrollmentReaction_03,
      AnimationName.kFaceEnrollmentReaction_04
    };
    private int _LastAnimationPlayedIdx = 0;

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
    private int _LastTurnedToAttemptID = 0;

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
      CurrentRobot.SetFaceEnrollmentMode(Anki.Vision.FaceEnrollmentMode.LookingStraight);
    }

    public void EnrollFace(string nameForFace) {
      _FaceNameDictionary.Add(_NewSeenFaceID, nameForFace);
      CurrentRobot.AssignNameToFace(_NewSeenFaceID, nameForFace);
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

      // Perform Name for the first time
      PlayFaceReactionAnimation(_NewSeenFaceID);
    }

    private void HandleReactionDone(bool success) {
      _NewSeenFaceID = 0;
      _FaceEnrollmentView.OnSubmitButton -= HandleSubmitButton;
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
      if (_FaceEnrollmentView == null) {
        if (faceID > 0 && string.IsNullOrEmpty(name) == false && _FaceNameDictionary.ContainsKey(faceID)) {
          // this is a face we know...
          if (Time.time - _LastPlayedReaction > 6.0f || _LastReactedID != faceID && !_Reacting) {
            // been at least 10 seconds since we reacted or it's a new face.
            _LastTurnedToAttemptID = faceID;
            _Reacting = true;
            CurrentRobot.TurnTowardsFacePose(CurrentRobot.Faces.Find(x => x.ID == faceID), callback: FacePoseDone);
          }
        }
      }
    }

    private void FacePoseDone(bool success) {
      if (success) {
        // Send Message to play react to face 
        PlayFaceReactionAnimation(_LastTurnedToAttemptID);

        _LastReactedID = _LastTurnedToAttemptID;
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

      if (_FaceEnrollmentView != null) {
        _FaceEnrollmentView.OnSubmitButton -= HandleSubmitButton;
        UIManager.CloseViewImmediately(_FaceEnrollmentView);
        _FaceEnrollmentView = null;
      }
    }

    private void PlayFaceReactionAnimation(int faceId) {
      DAS.Debug(this, "Attempt to Play Face Reaction Animation - FaceId: " + faceId);
      CurrentRobot.PrepareFaceNameAnimation(faceId, _FaceNameDictionary[faceId]);
      CurrentRobot.SendAnimation(_ReactionBank[_LastAnimationPlayedIdx], HandleReactionDone);
      _LastAnimationPlayedIdx = (_LastAnimationPlayedIdx + 1) % _ReactionBank.Length;
    }

  }

}
