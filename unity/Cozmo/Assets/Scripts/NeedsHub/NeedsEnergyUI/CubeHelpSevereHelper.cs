
using Cozmo.Needs;
using Cozmo.Needs.UI;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Energy.UI {
  public class CubeHelpSevereHelper : MonoBehaviour {
    [SerializeField]
    private CozmoButton _FeedButton;

    protected void Awake() {
      _FeedButton.Initialize(HandleFeedClicked, "quick_snack_button", "cube_help_view");
      // if no cubes are connected we want people to be able to keep playing.
      // this button simulates a feed when no cubes are connected.
      bool isFeedCritical = NeedsStateManager.Instance.GetCurrentDisplayValue(Anki.Cozmo.NeedId.Energy).Bracket ==
                                                      Anki.Cozmo.NeedBracketId.Critical;
      bool isInFeedOnboarding = OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.FeedIntro);
      _FeedButton.Interactable = (isFeedCritical || isInFeedOnboarding);
    }

    private void HandleFeedClicked() {
      NeedsStateManager.Instance.RegisterNeedActionCompleted(Anki.Cozmo.NeedsActionId.Feed);
      _FeedButton.Interactable = false;

      var quickSnackCompleteData = new AlertModalData("quick_snack_alert",
                                                      LocalizationKeys.kCubeHelpStringsInstructionsFeedPopupTitle,
                                                    LocalizationKeys.kCubeHelpStringsInstructionsFeedPopupBody,
                                                      new AlertModalButtonData("okay_button",
                                                                               LocalizationKeys.kButtonOkay, false));

      var quickSnackCompletePriorityData = new ModalPriorityData(ModalPriorityLayer.High, 0,
                                                               LowPriorityModalAction.Queue,
                                                               HighPriorityModalAction.Stack);

      UIManager.OpenAlert(quickSnackCompleteData, quickSnackCompletePriorityData);
    }
  }
}