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
    private FaceEnrollmentEnterNameSlide _EnterNameSlidePrefab;
    private FaceEnrollmentEnterNameSlide _EnterNameSlideInstance;

    [SerializeField]
    private FaceEnrollmentInstructionsSlide _EnrollmentInstructionsSlidePrefab;

    private bool _AttemptedEnrollFace = false;

    private string _NameForFace;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      RobotEngineManager.Instance.RobotObservedNewFace += HandleObservedNewFace;
    }

    protected override void InitializeView(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.InitializeView(newView, data);

      // create enter name dialog
      _EnterNameSlideInstance = newView.ShowWideGameStateSlide(_EnterNameSlidePrefab.gameObject, "enter_name_slide").GetComponent<FaceEnrollmentEnterNameSlide>();
      _EnterNameSlideInstance.OnNameEntered += HandleNameEntered;
    }

    private void HandleNameEntered(string name) {
      _NameForFace = name;
      CurrentRobot.SetFaceEnrollmentMode(Anki.Vision.FaceEnrollmentMode.LookingStraight);
      SharedMinigameView.ShowWideGameStateSlide(_EnrollmentInstructionsSlidePrefab.gameObject, "enrollment_instructions_slide").GetComponent<FaceEnrollmentInstructionsSlide>();
    }

    private void HandleObservedNewFace(int id, Vector3 pos, Quaternion rot) {

      if (_AttemptedEnrollFace) {
        return;
      }

      SharedMinigameView.HideGameStateSlide();

      CurrentRobot.AssignNameToFace(id, _NameForFace);
      CurrentRobot.SetFaceEnrollmentMode(Anki.Vision.FaceEnrollmentMode.Disabled);
      PlayFaceReactionAnimation(id);
      _AttemptedEnrollFace = true;
    }

    private void PlayFaceReactionAnimation(int faceId) {
      DAS.Debug("FaceEnrollmentGame.PlayFaceReactionAnimation", "Attempt to Play Face Reaction Animation - FaceId: " + faceId);
      // TODO: Implement the Action that has been added to replace the PrepareFaceNameAnimation message which has been deprecated
      //CurrentRobot.PrepareFaceNameAnimation(faceId, _NameForFace);
      CurrentRobot.SendAnimation(_ReactionBank[Random.Range(0, _ReactionBank.Length)], HandleReactionDone);
    }

    private void HandleReactionDone(bool success) {
      base.RaiseMiniGameQuit();
    }

    protected override void CleanUpOnDestroy() {
      CurrentRobot.SetFaceEnrollmentMode(Anki.Vision.FaceEnrollmentMode.Disabled);
      SharedMinigameView.HideGameStateSlide();
      RobotEngineManager.Instance.RobotObservedNewFace -= HandleObservedNewFace;
    }

  }

}
