using Anki.Cozmo;
using Anki.Cozmo.VoiceCommand;
using Cozmo.Challenge;
using Cozmo.RequestGame;
using Cozmo.UI;
using DataPersistence;
using UnityEngine;
using System.Collections.Generic;

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
    private GameObject _OnboardingDimmer;

    [SerializeField]
    private SparksListModal _SparksListModalPrefab;
    private SparksListModal _SparksListModalInstance;

    private List<UnlockableInfo> _AllUnlockData;
    private List<ChallengeManager.ChallengeStatePacket> _MinigameData;

    private bool _IsDisablingTouches = false;
    private ChallengeStartEdgeCaseAlertController _EdgeCaseAlertController;

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
      VoiceCommandManager.SendVoiceCommandEvent<RequestStatusUpdate>(Singleton<RequestStatusUpdate>.Instance);

      RequestGameManager.Instance.OnRequestGameAlertCreated += ReenableTouches;

      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.PlayIntro)) {
        _OnboardingDimmer.SetActive(true);
        _BackButton.gameObject.SetActive(false);
        OnboardingManager.Instance.OnOnboardingPhaseCompleted += HandleOnboardingPhaseComplete;
      }

      ChallengeEdgeCases challengeEdgeCases = ChallengeEdgeCases.CheckForDizzy
                                      | ChallengeEdgeCases.CheckForHiccups
                                      | ChallengeEdgeCases.CheckForDriveOffCharger
                                      | ChallengeEdgeCases.CheckForOnTreads;
      _EdgeCaseAlertController = new ChallengeStartEdgeCaseAlertController(new ModalPriorityData(), challengeEdgeCases);
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

      if (_IsDisablingTouches) {
        CancelInvoke();
        ReenableTouches();
      }
    }

    private void HandleBackButtonPressed() {
      if (OnBackButtonPressed != null) {
        OnBackButtonPressed();
      }
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
      _AskForTrickMicIcon.gameObject.SetActive(stateData.isVCEnabled);
      _AskForGameMicIcon.gameObject.SetActive(stateData.isVCEnabled);
    }
  }
}
