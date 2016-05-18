using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentGame : GameBase {

    [SerializeField]
    private FaceEnrollmentEnterNameSlide _EnterNameSlidePrefab;
    private FaceEnrollmentEnterNameSlide _EnterNameSlideInstance;

    [SerializeField]
    private GameObject _FaceEnrollmentDiagramPrefab;

    private bool _AttemptedEnrollFace = false;

    // used by press demo to skip saving to actual robot.
    private bool _SaveToRobot = true;

    private string _NameForFace;

    private int _FixedFaceID = -1;

    private bool _UseFixedFaceID = false;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      // make cozmo look up
      CurrentRobot.SetHeadAngle(0.5f);
      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnWiggle, HandleWiggleAnimEnd);
    }

    protected override void InitializeView(Cozmo.MinigameWidgets.SharedMinigameView newView, ChallengeData data) {
      base.InitializeView(newView, data);

      // create enter name dialog
      _EnterNameSlideInstance = newView.ShowWideGameStateSlide(_EnterNameSlidePrefab.gameObject, "enter_name_slide").GetComponent<FaceEnrollmentEnterNameSlide>();
      _EnterNameSlideInstance.OnNameEntered += HandleNameEntered;
    }

    public void SetSaveToRobot(bool saveToRobot) {
      _SaveToRobot = saveToRobot;
    }

    public void SetFixedFaceID(int id) {
      _FixedFaceID = id;
      _UseFixedFaceID = true;
    }

    private void HandleNameEntered(string name) {
      _NameForFace = name;
      SharedMinigameView.ShowWideAnimationSlide("faceEnrollment.instructions", "face_enrollment_wait_instructions", _FaceEnrollmentDiagramPrefab, HandleInstructionsSlideEntered);
      SharedMinigameView.ShowShelf();
      SharedMinigameView.ShowSpinnerWidget();
    }

    private void HandleInstructionsSlideEntered() {
      if (_UseFixedFaceID) {
        CurrentRobot.EnrollNamedFace(_FixedFaceID, _NameForFace, saveToRobot: _SaveToRobot, callback: HandleEnrolledFace);
        _AttemptedEnrollFace = true;
      }
      else {
        RobotEngineManager.Instance.RobotObservedNewFace += HandleObservedNewFace;
      }
    }

    private void HandleObservedNewFace(int id, Vector3 pos, Quaternion rot) {

      if (_AttemptedEnrollFace) {
        return;
      }

      CurrentRobot.EnrollNamedFace(id, _NameForFace, saveToRobot: _SaveToRobot, callback: HandleEnrolledFace);
      _AttemptedEnrollFace = true;
    }

    private void HandleEnrolledFace(bool success) {

      // hides the instructions slide
      SharedMinigameView.HideGameStateSlide();
      SharedMinigameView.HideShelf();
      SharedMinigameView.HideSpinnerWidget();

      if (success) {
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedBlockConnect);
        PlayFaceReactionAnimation(_NameForFace);
      }
      else {
        // TODO: Retry or notify failure or something?
        base.RaiseMiniGameQuit();
      }
    }

    private void PlayFaceReactionAnimation(string faceName) {
      DAS.Debug("FaceEnrollmentGame.PlayFaceReactionAnimation", "Attempt to Play Face Reaction Animation - FaceId: " + faceName);
      // Chains the wiggle to the Long face name in HandleWiggleEnd
      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnWiggle);
    }

    private void HandleReactionDone(bool success) {
      base.RaiseMiniGameQuit();
    }

    private void HandleWiggleAnimEnd(bool success) {
      RobotEngineManager.Instance.CurrentRobot.SayTextWithEvent(_NameForFace, Anki.Cozmo.GameEvent.OnLearnedPlayerName, Anki.Cozmo.SayTextStyle.Name_FirstIntroduction, HandleReactionDone);
    }

    protected override void CleanUpOnDestroy() {
      SharedMinigameView.HideGameStateSlide();
      RobotEngineManager.Instance.RobotObservedNewFace -= HandleObservedNewFace;
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnWiggle, HandleWiggleAnimEnd);
    }

  }

}
