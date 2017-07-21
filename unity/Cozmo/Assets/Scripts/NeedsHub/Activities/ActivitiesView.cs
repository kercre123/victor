using Cozmo.Challenge;
using Cozmo.UI;
using DG.Tweening;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using DataPersistence;
using System;

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
    private FreeplayCard _FreeplayCardPrefab;
    private FreeplayCard _FreeplayCardInstance;

    [SerializeField]
    private ScrollRect _ScrollRect;

    private ChallengeStartEdgeCaseAlertController _EdgeCaseAlertController;

    [SerializeField]
    private float _FreeplayPanelArrowToggleWidth;
    private float _FreeplayPanelNormalizedWidth;

    public void InitializeActivitiesView(List<ChallengeManager.ChallengeStatePacket> activityData) {
      _BackButton.Initialize(HandleBackButtonPressed, "back_button", DASEventDialogName);

      // add the freeplay card
      GameObject newCard = UIManager.CreateUIElement(_FreeplayCardPrefab, _ActivityCardContainer.transform);
      _FreeplayCardInstance = newCard.GetComponent<FreeplayCard>();
      _FreeplayCardInstance.Initialize(this);

      int numCards = 0;
      foreach (var data in activityData) {
        // IVY TODO: Add check to see if activity is unlocked? Requires loading a different "defaultUnlocks" config
        if (!data.Data.ActivityData.HideInActivityView) {
          newCard = UIManager.CreateUIElement(_ActivityCardPrefab, _ActivityCardContainer.transform);
          ActivityCard cardScript = newCard.GetComponent<ActivityCard>();
          cardScript.Initialize(data.Data, DASEventDialogName);
          cardScript.OnActivityButtonPressed += HandleActivityButtonPressed;
          numCards++;
        }
      }

      LayoutRebuilder.ForceRebuildLayoutImmediate(_ScrollRect.content);

      // calculate panel width
      float width = (_ScrollRect.content.rect.width - _ScrollRect.viewport.rect.width);
      float spacing = _ScrollRect.content.GetComponent<HorizontalLayoutGroup>().spacing;
      _FreeplayPanelNormalizedWidth = (_FreeplayCardInstance.GetComponent<RectTransform>().rect.width + spacing) / width;
      if (_FreeplayPanelNormalizedWidth > 1.0f) {
        _FreeplayPanelNormalizedWidth = 1.0f;
      }

      if (DataPersistenceManager.Instance.Data.DefaultProfile.HideFreeplayCard) {
        HideFreeplayCard(true);
        _FreeplayCardInstance.SlideArrowFaceLeft(true);
      }
      else {
        ShowFreeplayCard(true);
        _FreeplayCardInstance.SlideArrowFaceLeft(false);
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

    internal void HideFreeplayCard(bool instant = false) {
      if (instant) {
        _ScrollRect.horizontalNormalizedPosition = _FreeplayPanelNormalizedWidth;
      }
      else {
        _ScrollRect.DOHorizontalNormalizedPos(_FreeplayPanelNormalizedWidth, 0.3f);
      }
    }

    internal void ShowFreeplayCard(bool instant = false) {
      float normalizedPos = 0;
      if (instant) {
        _ScrollRect.horizontalNormalizedPosition = normalizedPos;
      }
      else {
        _ScrollRect.DOHorizontalNormalizedPos(normalizedPos, 0.3f);
      }
    }

    protected void Update() {
      if (_ScrollRect.horizontalNormalizedPosition < _FreeplayPanelNormalizedWidth - _FreeplayPanelArrowToggleWidth) {
        _FreeplayCardInstance.SlideArrowFaceLeft(false);
      }
      else {
        // a tiny bit of slop for the left arrow
        float faceLeftPosition = Mathf.Min(_FreeplayPanelNormalizedWidth - 0.01f, 0.99f);
        if (_ScrollRect.horizontalNormalizedPosition > faceLeftPosition) {
          _FreeplayCardInstance.SlideArrowFaceLeft(true);
        }
      }
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
      // makes the slide off right look better
      _ScrollRect.DOHorizontalNormalizedPos(0, 0.3f);
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