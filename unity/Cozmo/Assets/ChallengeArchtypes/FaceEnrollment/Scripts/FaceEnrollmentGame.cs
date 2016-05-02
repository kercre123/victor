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
      UIManager.CloseView(_EnterNameViewInstance);

      _NameForFace = name;
      CurrentRobot.SetFaceEnrollmentMode(Anki.Vision.FaceEnrollmentMode.LookingStraight);
      _EnrollmentInstructionsViewInstance = UIManager.OpenView(_EnrollmentInstructionsViewPrefab, verticalCanvas: true);

    }

    private void HandleObservedNewFace(int id, Vector3 pos, Quaternion rot) {
      UIManager.CloseView(_EnrollmentInstructionsViewInstance);

      // we've successfully enrolled a new face, lets assign the inputted name to it.
      CurrentRobot.AssignNameToFace(id, _NameForFace);
      PlayFaceReactionAnimation(id);
    }

    private void PlayFaceReactionAnimation(int faceId) {
      DAS.Debug(this, "Attempt to Play Face Reaction Animation - FaceId: " + faceId);
      CurrentRobot.PrepareFaceNameAnimation(faceId, _FaceNameDictionary[faceId]);
      CurrentRobot.SendAnimation(_ReactionBank[Random.Range(0, _ReactionBank.Length)], HandleReactionDone);
    }

    private void HandleReactionDone(bool success) {
      base.RaiseMiniGameWin();
    }

    protected override void CleanUpOnDestroy() {
      if (_EnterNameViewInstance != null) {
        UIManager.CloseView(_EnterNameViewInstance);
      }
      if (_EnrollmentInstructionsViewInstance != null) {
        UIManager.CloseView(_EnrollmentInstructionsViewInstance);
      }
      RobotEngineManager.Instance.RobotObservedNewFace -= HandleObservedNewFace;
    }

  }

}
