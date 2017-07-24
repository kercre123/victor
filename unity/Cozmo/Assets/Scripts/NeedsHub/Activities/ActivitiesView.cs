using Cozmo.Challenge;
using Cozmo.UI;
using DG.Tweening;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Needs.Activities.UI {
  public class ActivitiesView : BaseView {
    public delegate void BackButtonPressedHandler();
    public event BackButtonPressedHandler OnBackButtonPressed;

    public delegate void ActivityButtonPressedHandler(string challengeId);
    public event ActivityButtonPressedHandler OnActivityButtonPressed;

    [SerializeField]
    private CozmoButton _BackButton;
    [SerializeField]
    private HorizontalLayoutGroup _ActivityCardContainer;

    [SerializeField]
    private ActivityCard _ActivityCardPrefab;

    [SerializeField]
    private int _MaxNumCardsOnScreen = 3;

    [SerializeField]
    private ScrollRect _ScrollRect;

    private ChallengeStartEdgeCaseAlertController _EdgeCaseAlertController;

    public void InitializeActivitiesView(List<ChallengeManager.ChallengeStatePacket> activityData) {
      _BackButton.Initialize(HandleBackButtonPressed, "back_button", DASEventDialogName);

      int numCards = 0;
      foreach (var data in activityData) {
        // IVY TODO: Add check to see if activity is unlocked? Requires loading a different "defaultUnlocks" config
        if (!data.Data.ActivityData.HideInActivityView) {
          GameObject newCard = UIManager.CreateUIElement(_ActivityCardPrefab, _ActivityCardContainer.transform);
          ActivityCard cardScript = newCard.GetComponent<ActivityCard>();
          cardScript.Initialize(data.Data, DASEventDialogName);
          cardScript.OnActivityButtonPressed += HandleActivityButtonPressed;
          numCards++;
        }
      }

      if (numCards <= _MaxNumCardsOnScreen) {
        _ScrollRect.enabled = false;
      }
      else {
        _ScrollRect.horizontalNormalizedPosition = 0f;
      }

      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.DiscoverIntro)) {
        OnboardingManager.Instance.StartPhase(OnboardingManager.OnboardingPhases.DiscoverIntro);
      }

      ChallengeEdgeCases edgeCases = ChallengeEdgeCases.CheckForDizzy | ChallengeEdgeCases.CheckForHiccups
                                                       | ChallengeEdgeCases.CheckForDriveOffCharger
                                                       | ChallengeEdgeCases.CheckForOnTreads
                                                       | ChallengeEdgeCases.CheckForOS;
      _EdgeCaseAlertController = new ChallengeStartEdgeCaseAlertController(new ModalPriorityData(), edgeCases);
    }

    protected override void CleanUp() {
      if (_EdgeCaseAlertController != null) {
        _EdgeCaseAlertController.CleanUp();
      }
    }

    private void HandleBackButtonPressed() {
      if (OnBackButtonPressed != null) {
        OnBackButtonPressed();
      }
    }

    private void HandleActivityButtonPressed(ChallengeData challengeData) {
      UnlockableInfo unlockInfo = UnlockablesManager.Instance.GetUnlockableInfo(challengeData.UnlockId.Value);
      bool isOsSupported = UnlockablesManager.Instance.IsOSSupported(unlockInfo);
      bool edgeCaseFound = _EdgeCaseAlertController.ShowEdgeCaseAlertIfNeeded(challengeData.ChallengeTitleLocKey,
                                                                              unlockInfo.CubesRequired,
                                                                              isOsSupported,
                                                                              unlockInfo.AndroidReleaseVersion);
      if (!edgeCaseFound) {
        if (OnActivityButtonPressed != null) {
          OnActivityButtonPressed(challengeData.ChallengeID);
        }
      }
    }
  }
}