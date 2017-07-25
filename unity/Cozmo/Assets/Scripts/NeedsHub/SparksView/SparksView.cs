using Anki.Cozmo;
using Anki.Cozmo.VoiceCommand;
using Cozmo.Challenge;
using Cozmo.RequestGame;
using Cozmo.UI;
using DataPersistence;
using UnityEngine;
using System.Collections.Generic;
using UnityEngine.UI;
using DG.Tweening;

namespace Cozmo.Needs.Sparks.UI {
  public class SparksView : BaseView {
    public delegate void BackButtonPressedHandler();
    public event BackButtonPressedHandler OnBackButtonPressed;

    private const string _DisableTouchKey = "SparksViewAskForGameInProgress";
    private const float _ReenableTouchTimeout_s = 5f;

    [SerializeField]
    private CozmoButton _BackButton;

    [SerializeField]
    private CozmoButton _AskForTrickButton;

    [SerializeField]
    private CozmoText _AskForTrickCostText;

    [SerializeField]
    private CozmoImage _AskForTrickMicIcon;

    [SerializeField]
    private CozmoButton _AskForGameButton;

    [SerializeField]
    private CozmoText _AskForGameCostText;

    [SerializeField]
    private CozmoImage _AskForGameMicIcon;

    [SerializeField]
    private CozmoButton _ListAbilitiesButton;

    [SerializeField]
    private GameObject[] _OnboardingDimmers;

    [SerializeField]
    private SparksListModal _SparksListModalPrefab;
    private SparksListModal _SparksListModalInstance;

    [SerializeField]
    private ScrollRect _ScrollRect;

    [SerializeField]
    private FreeplayCard _FreeplayCardInstance;

    [SerializeField]
    private float _FreeplayPanelShowRightArrowWidth = 0.2f;

    [SerializeField]
    private float _FreeplayPanelShowLeftArrowWidth = 0.01f;

    [SerializeField]
    private float _FreeplayPanelShowHideInterpolateTime = 0.3f;

    private float _FreeplayPanelNormalizedWidth;

    private List<UnlockableInfo> _AllUnlockData;
    private List<ChallengeManager.ChallengeStatePacket> _MinigameData;

    private bool _IsDisablingTouches = false;
    private ChallengeStartEdgeCaseAlertController _EdgeCaseAlertController;
    private NeedSevereAlertController _NeedSevereAlertController;

    public void InitializeSparksView(List<ChallengeManager.ChallengeStatePacket> minigameData) {
      _BackButton.Initialize(HandleBackButtonPressed, "back_button", DASEventDialogName);
      _AskForTrickButton.Initialize(HandleAskForTrickButtonPressed, "ask_for_trick", DASEventDialogName);
      _AskForGameButton.Initialize(HandleAskForGameButtonPressed, "ask_for_game", DASEventDialogName);
      _ListAbilitiesButton.Initialize(HandleListAbilitiesButtonPressed, "list_abilities_button", DASEventDialogName);

      _AskForTrickCostText.text = Localization.GetNumber((int)EnumConcept.GetSparkCosts(SparkableThings.DoATrick, 1));
      _AskForGameCostText.text = Localization.GetNumber((int)EnumConcept.GetSparkCosts(SparkableThings.PlayAGame, 1));
      UpdateButtonState();

      _MinigameData = minigameData;

      _AllUnlockData = UnlockablesManager.Instance.GetUnlockablesByType(UnlockableType.Action);
      _AllUnlockData.Sort();

      Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded += HandleItemValueChanged;
      playerInventory.ItemRemoved += HandleItemValueChanged;
      playerInventory.ItemCountSet += HandleItemValueChanged;
      playerInventory.ItemCountUpdated += HandleItemValueChanged;

      VoiceCommandManager.Instance.StateDataCallback += UpdateStateData;
      VoiceCommandManager.RequestCurrentStateData();
      _AskForGameMicIcon.gameObject.SetActive(false);
      _AskForTrickMicIcon.gameObject.SetActive(false);

      RequestGameManager.Instance.OnRequestGameAlertCreated += ReenableTouches;

      bool hideFreeplayCard = DataPersistenceManager.Instance.Data.DefaultProfile.HideFreeplayCard;

      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.PlayIntro)) {
        for (int i = 0; i < _OnboardingDimmers.Length; i++) {
          _OnboardingDimmers[i].SetActive(true);
        }
        _BackButton.gameObject.SetActive(false);
        hideFreeplayCard = true;
        OnboardingManager.Instance.OnOnboardingPhaseCompleted += HandleOnboardingPhaseComplete;
      }

      // calculate freeplay panel width
      LayoutRebuilder.ForceRebuildLayoutImmediate(_ScrollRect.content);
      float width = (_ScrollRect.content.rect.width - _ScrollRect.viewport.rect.width);
      float spacing = _ScrollRect.content.GetComponent<HorizontalLayoutGroup>().spacing;
      _FreeplayPanelNormalizedWidth = (_FreeplayCardInstance.GetComponent<RectTransform>().rect.width + spacing) / width;
      if (_FreeplayPanelNormalizedWidth > 1.0f) {
        _FreeplayPanelNormalizedWidth = 1.0f;
      }

      _FreeplayCardInstance.Initialize(this);
      if (hideFreeplayCard) {
        HideFreeplayCard(instant: true);
        _FreeplayCardInstance.ShowSlideLeftArrow();
      }
      else {
        ShowFreeplayCard(instant: true);
        _FreeplayCardInstance.ShowSlideRightArrow();
      }

      ChallengeEdgeCases challengeEdgeCases = ChallengeEdgeCases.CheckForDizzy
                                      | ChallengeEdgeCases.CheckForHiccups
                                      | ChallengeEdgeCases.CheckForDriveOffCharger
                                      | ChallengeEdgeCases.CheckForOnTreads;
      _EdgeCaseAlertController = new ChallengeStartEdgeCaseAlertController(new ModalPriorityData(), challengeEdgeCases);

      _NeedSevereAlertController = new NeedSevereAlertController(new ModalPriorityData(ModalPriorityLayer.High,
                                                                                       1,
                                                                                       LowPriorityModalAction.CancelSelf,
                                                                                       HighPriorityModalAction.Stack));
      _NeedSevereAlertController.AllowAlert = true;
      _NeedSevereAlertController.OnNeedSevereAlertClosed += HandleNeedSevereAlertClosed;
      SparksDetailModal.OnSparkTrickStarted += HandleSparkTrickStarted;
      SparksDetailModal.OnSparkTrickEnded += HandleSparkTrickEnded;
      SparksDetailModal.OnSparkTrickQuit += HandleSparkTrickEnded;
    }

    protected override void CleanUp() {
      Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded -= HandleItemValueChanged;
      playerInventory.ItemRemoved -= HandleItemValueChanged;
      playerInventory.ItemCountSet -= HandleItemValueChanged;
      playerInventory.ItemCountUpdated -= HandleItemValueChanged;

      VoiceCommandManager.Instance.StateDataCallback -= UpdateStateData;

      RequestGameManager.Instance.OnRequestGameAlertCreated -= ReenableTouches;

      OnboardingManager.Instance.OnOnboardingPhaseCompleted -= HandleOnboardingPhaseComplete;

      _NeedSevereAlertController.OnNeedSevereAlertClosed -= HandleNeedSevereAlertClosed;
      SparksDetailModal.OnSparkTrickStarted -= HandleSparkTrickStarted;
      SparksDetailModal.OnSparkTrickEnded -= HandleSparkTrickEnded;
      SparksDetailModal.OnSparkTrickQuit -= HandleSparkTrickEnded;
      _NeedSevereAlertController.CleanUp();
      _NeedSevereAlertController = null;

      if (_IsDisablingTouches) {
        CancelInvoke();
        ReenableTouches();
      }
    }

    private void HandleNeedSevereAlertClosed() {
      // Emulate a back button press to return back to NeedsHubView
      HandleBackButtonPressed();
    }

    private void HandleBackButtonPressed() {
      if (OnBackButtonPressed != null) {
        OnBackButtonPressed();
      }
      // slide scrollview all the way left so it goes offscreen
      ShowFreeplayCard();
    }

    private void HandleOnboardingPhaseComplete(OnboardingManager.OnboardingPhases phase) {
      if (phase == OnboardingManager.OnboardingPhases.PlayIntro) {
        HandleBackButtonPressed();
      }
    }

    private void HandleAskForTrickButtonPressed() {
      if (ShowEdgeCaseAlertIfNeeded()) {
        return;
      }

      // Decrementing spark cost will be handled by engine
      // Showing details modal is done by NeedsHub to account for VC
      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.PlayIntro)) {
        OnboardingManager.Instance.GoToNextStage();
      }
      else if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.DoRandomSpark();
      }
    }

    private void HandleAskForGameButtonPressed() {
      if (ShowEdgeCaseAlertIfNeeded()) {
        return;
      }

      // Decrementing spark cost will be handled by engine
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.RequestRandomGame();

        // Prevent the player from doing other things
        _IsDisablingTouches = true;
        UIManager.DisableTouchEvents(_DisableTouchKey);
        ContextManager.Instance.ShowForeground();

        // Make sure that we re-enable touches in case something goes wrong with robot comms
        Invoke("ReenableTouches", _ReenableTouchTimeout_s);
      }
    }

    private bool ShowEdgeCaseAlertIfNeeded() {
      // We can send in default values because we are not checking cubes or os
      return _EdgeCaseAlertController.ShowEdgeCaseAlertIfNeeded(null, 0, true, null);
    }

    private void ReenableTouches() {
      if (_IsDisablingTouches) {
        _IsDisablingTouches = false;
        UIManager.EnableTouchEvents(_DisableTouchKey);
        ContextManager.Instance.HideForeground();
      }
    }

    private void HandleListAbilitiesButtonPressed() {
      ModalPriorityData sparksListPriority = new ModalPriorityData(ModalPriorityLayer.VeryLow,
                                                                   0,
                                                                   LowPriorityModalAction.CancelSelf,
                                                                   HighPriorityModalAction.Stack);
      UIManager.OpenModal(_SparksListModalPrefab, sparksListPriority, (obj) => {
        _SparksListModalInstance = (SparksListModal)obj;
        _SparksListModalInstance.InitializeSparksListModal(_MinigameData, _AllUnlockData);
      });
    }

    private void HandleItemValueChanged(string itemId, int delta, int newCount) {
      if (itemId == RewardedActionManager.Instance.SparkID) {
        UpdateButtonState();
      }
    }

    private void UpdateButtonState() {
      Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      string sparkId = RewardedActionManager.Instance.SparkID;
      int randomSparkCost = (int)EnumConcept.GetSparkCosts(SparkableThings.DoATrick, 1);
      bool canAffordRandomSparks = playerInventory.CanRemoveItemAmount(RewardedActionManager.Instance.SparkID, randomSparkCost);
      _AskForTrickButton.Interactable = canAffordRandomSparks;
      _AskForTrickCostText.color = canAffordRandomSparks ? UIColorPalette.GeneralSparkTintColor.CanAffordColor
        : UIColorPalette.GeneralSparkTintColor.CannotAffordColor;

      int randomGameCost = (int)EnumConcept.GetSparkCosts(SparkableThings.PlayAGame, 1);
      bool canAffordRandomGame = playerInventory.CanRemoveItemAmount(RewardedActionManager.Instance.SparkID, randomGameCost);
      _AskForGameButton.Interactable = canAffordRandomGame;
      _AskForGameCostText.color = canAffordRandomGame ? UIColorPalette.GeneralSparkTintColor.CanAffordColor
        : UIColorPalette.GeneralSparkTintColor.CannotAffordColor;
    }

    private void UpdateStateData(StateData stateData) {
      _AskForTrickMicIcon.gameObject.SetActive(VoiceCommandManager.IsVoiceCommandsEnabled(stateData));
      _AskForGameMicIcon.gameObject.SetActive(VoiceCommandManager.IsVoiceCommandsEnabled(stateData));
    }

    internal void HideFreeplayCard(bool instant = false) {
      if (instant) {
        _ScrollRect.horizontalNormalizedPosition = _FreeplayPanelNormalizedWidth;
      }
      else {
        _ScrollRect.DOHorizontalNormalizedPos(_FreeplayPanelNormalizedWidth, _FreeplayPanelShowHideInterpolateTime);
      }
    }

    internal void ShowFreeplayCard(bool instant = false) {
      float normalizedPos = 0;
      if (instant) {
        _ScrollRect.horizontalNormalizedPosition = normalizedPos;
      }
      else {
        _ScrollRect.DOHorizontalNormalizedPos(normalizedPos, _FreeplayPanelShowHideInterpolateTime);
      }
    }

    private void HandleSparkTrickStarted() {
      _NeedSevereAlertController.AllowAlert = false;
    }

    private void HandleSparkTrickEnded() {
      _NeedSevereAlertController.AllowAlert = true;
    }

    protected void Update() {
      float showRightWidth = _FreeplayPanelNormalizedWidth * (1.0f - _FreeplayPanelShowRightArrowWidth);
      if (_ScrollRect.horizontalNormalizedPosition < showRightWidth) {
        _FreeplayCardInstance.ShowSlideRightArrow();
      }
      else {
        float showLeftWidth = _FreeplayPanelNormalizedWidth * (1.0f - _FreeplayPanelShowLeftArrowWidth);
        if (_ScrollRect.horizontalNormalizedPosition >= showLeftWidth) {
          _FreeplayCardInstance.ShowSlideLeftArrow();
        }
      }
    }
  }
}