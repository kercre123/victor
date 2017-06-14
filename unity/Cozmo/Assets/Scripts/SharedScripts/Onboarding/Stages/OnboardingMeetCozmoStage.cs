using Cozmo.MinigameWidgets;

namespace Onboarding {

  public class OnboardingMeetCozmoStage : OnboardingBaseStage {

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
