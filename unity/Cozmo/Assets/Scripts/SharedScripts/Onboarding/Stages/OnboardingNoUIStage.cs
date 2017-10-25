using Cozmo.MinigameWidgets;

namespace Onboarding {

  public class OnboardingNoUIStage : OnboardingBaseStage {

    // Since this is really the meet cozmo class. Don't allow quitting or other ways of escape from onboarding.
    public override void Start() {
      base.Start();
      Cozmo.PauseManager.Instance.IsIdleTimeOutEnabled = false;
      Cozmo.PauseManager.Instance.ExitChallengeOnPause = false;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      Cozmo.PauseManager.Instance.IsIdleTimeOutEnabled = true;
      Cozmo.PauseManager.Instance.ExitChallengeOnPause = true;
    }
    public override void SkipPressed() {

      FaceEnrollment.FaceEnrollmentInstructionsModal faceEnrollmentModal = FindObjectOfType<FaceEnrollment.FaceEnrollmentInstructionsModal>();
      if (faceEnrollmentModal != null) {
        faceEnrollmentModal.SetQuitButtonVisibility(true);
      }
      // The power just to quit yourself, the only custom behavior this onboarding phse turned off...
      SharedMinigameView challengeView = FindObjectOfType<SharedMinigameView>();
      if (challengeView != null) {
        challengeView.ShowQuitButton();
      }
      OnboardingManager.Instance.GoToNextStage();
    }

  }

}
