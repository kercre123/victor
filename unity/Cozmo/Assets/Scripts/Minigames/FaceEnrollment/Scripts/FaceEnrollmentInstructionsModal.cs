using UnityEngine;
using Cozmo.MinigameWidgets;

namespace FaceEnrollment {
  public class FaceEnrollmentInstructionsModal : Cozmo.UI.BaseModal {

    [SerializeField]
    private CozmoText _NameForFaceLabel;

    [SerializeField]
    private QuitMinigameButton _QuitMinigameButton;

    public QuitMinigameButton QuitMinigameButton {
      get {
        return _QuitMinigameButton;
      }
    }

    protected override void RaiseDialogOpened() {
      base.RaiseDialogOpened();
      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.MeetCozmo)) {
        if (RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count == 0) {
          SetQuitButtonVisibility(false);
        }
      }
    }
    public void SetNameForFace(string nameForFace) {
      _NameForFaceLabel.text = nameForFace + ":";
    }

    protected override void CleanUp() {
      base.CleanUp();
    }

    public void SetQuitButtonVisibility(bool visable) {
      if (_OptionalCloseDialogButton != null) {
        _OptionalCloseDialogButton.gameObject.SetActive(visable);
      }
      if (_OptionalCloseDialogCozmoButton != null) {
        _OptionalCloseDialogCozmoButton.gameObject.SetActive(visable);
      }
    }
  }
}