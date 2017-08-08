
using Cozmo.Needs;
using Cozmo.Needs.UI;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Energy.UI {
  public class CubeHelpSevereHelper : MonoBehaviour {
    [SerializeField]
    private CozmoButton _FeedButton;

    private bool _QuickSnackAvailable;

    protected void Awake() {
      _FeedButton.Initialize(HandleFeedClicked, "quick_snack_button", "cube_help_view");
      // if no cubes are connected we want people to be able to keep playing.
      // this button simulates a feed when no cubes are connected.
      bool isFeedCritical = NeedsStateManager.Instance.GetCurrentDisplayValue(Anki.Cozmo.NeedId.Energy).Bracket ==
                                                      Anki.Cozmo.NeedBracketId.Critical;
      bool isInFeedOnboarding = OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.FeedIntro);

      _QuickSnackAvailable = isFeedCritical || isInFeedOnboarding;
      _FeedButton.ShowDisabledStateWhenInteractable = !_QuickSnackAvailable;
    }

    private void HandleFeedClicked() {
      string alertTitleKey = "";
      string alertBodyKey = "";

      if (_QuickSnackAvailable) {
        NeedsStateManager.Instance.RegisterNeedActionCompleted(Anki.Cozmo.NeedsActionId.Feed);
        _QuickSnackAvailable = false;
        _FeedButton.ShowDisabledStateWhenInteractable = !_QuickSnackAvailable;
        alertTitleKey = LocalizationKeys.kCubeHelpStringsInstructionsFeedPopupTitle;
        alertBodyKey = LocalizationKeys.kCubeHelpStringsInstructionsFeedPopupBody;
      }
      else {
        alertTitleKey = LocalizationKeys.kCubeHelpStringsInstructionsDontNeedToFeedPopupTitle;
        alertBodyKey = LocalizationKeys.kCubeHelpStringsInstructionsDontNeedToFeedPopupBody;
      }

      var quickSnackCompleteData = new AlertModalData("quick_snack_alert",
                                                      alertTitleKey,
                                                      alertBodyKey,
                                                      new AlertModalButtonData("okay_button",
                                                                               LocalizationKeys.kButtonOkay));

      var quickSnackCompletePriorityData = new ModalPriorityData(ModalPriorityLayer.High, 0,
                                                               LowPriorityModalAction.Queue,
                                                               HighPriorityModalAction.Stack);

      UIManager.OpenAlert(quickSnackCompleteData, quickSnackCompletePriorityData);
    }
  }
}