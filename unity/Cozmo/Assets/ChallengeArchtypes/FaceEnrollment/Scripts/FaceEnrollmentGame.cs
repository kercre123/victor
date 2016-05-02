using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    private string[] _ReactionBank = {
      AnimationName.kFaceEnrollmentReaction_00,
      AnimationName.kFaceEnrollmentReaction_01,
      AnimationName.kFaceEnrollmentReaction_02,
      AnimationName.kFaceEnrollmentReaction_03,
      AnimationName.kFaceEnrollmentReaction_04
    };

    [SerializeField]
    private FaceEnrollmentEnterNameView _EnterNameViewPrefab;
    private FaceEnrollmentEnterNameView _EnterNameViewInstance;

    [SerializeField]
    private FaceEnrollmentInstructionsView _EnrollmentInstructionsViewPrefab;
    private FaceEnrollmentInstructionsView _EnrollmentInstructionsViewInstance;

    private string _NameForFace;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      RobotEngineManager.Instance.RobotObservedNewFace += HandleObservedNewFace;
    }

    protected override void InitializeView(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.InitializeView(newView, data);

      // create enter name dialog
      _EnterNameViewInstance = UIManager.OpenView(_EnterNameViewPrefab, verticalCanvas: true);
      _EnterNameViewInstance.OnNameEntered += HandleNameEntered;
    }

    private void HandleNameEntered(string name) {
      UIManager.CloseViewImmediately(_EnterNameViewInstance);
      _EnterNameViewInstance = null;

      _NameForFace = name;
      CurrentRobot.SetFaceEnrollmentMode(Anki.Vision.FaceEnrollmentMode.LookingStraight);
      _EnrollmentInstructionsViewInstance = UIManager.OpenView(_EnrollmentInstructionsViewPrefab, verticalCanvas: true);

    }

    private void HandleObservedNewFace(int id, Vector3 pos, Quaternion rot) {
      UIManager.CloseViewImmediately(_EnrollmentInstructionsViewInstance);
      _EnrollmentInstructionsViewInstance = null;

      CurrentRobot.AssignNameToFace(id, _NameForFace);
      CurrentRobot.SetFaceEnrollmentMode(Anki.Vision.FaceEnrollmentMode.Disabled);
      PlayFaceReactionAnimation(id);
    }

    private void PlayFaceReactionAnimation(int faceId) {
      DAS.Debug(this, "Attempt to Play Face Reaction Animation - FaceId: " + faceId);
      CurrentRobot.PrepareFaceNameAnimation(faceId, _NameForFace);
      CurrentRobot.SendAnimation(_ReactionBank[Random.Range(0, _ReactionBank.Length)], HandleReactionDone);
    }

    private void HandleReactionDone(bool success) {
      base.RaiseMiniGameQuit();
    }

    protected override void CleanUpOnDestroy() {
      CurrentRobot.SetFaceEnrollmentMode(Anki.Vision.FaceEnrollmentMode.Disabled);
      if (_EnterNameViewInstance != null) {
        UIManager.CloseViewImmediately(_EnterNameViewInstance);
      }
      if (_EnrollmentInstructionsViewInstance != null) {
        UIManager.CloseViewImmediately(_EnrollmentInstructionsViewInstance);
      }
      RobotEngineManager.Instance.RobotObservedNewFace -= HandleObservedNewFace;
    }

  }

}
