using Anki.Cozmo;
using Anki.Cozmo.VoiceCommand;
using Cozmo.UI;
using Cozmo.Challenge;
using DataPersistence;
using UnityEngine;
using System.Collections.Generic;

namespace Cozmo.Needs.Sparks.UI {
  public class SparksView : BaseView {
    public delegate void BackButtonPressedHandler();
    public event BackButtonPressedHandler OnBackButtonPressed;

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

      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.PlayIntro)) {
        _OnboardingDimmer.SetActive(true);
        _BackButton.gameObject.SetActive(false);
        OnboardingManager.Instance.OnOnboardingPhaseCompleted += HandleOnboardingPhaseComplete;
      }
    }

    protected override void CleanUp() {
      Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded -= HandleItemValueChanged;
      playerInventory.ItemRemoved -= HandleItemValueChanged;
      playerInventory.ItemCountSet -= HandleItemValueChanged;
      playerInventory.ItemCountUpdated -= HandleItemValueChanged;

      VoiceCommandManager.Instance.StateDataCallback -= UpdateStateData;
      OnboardingManager.Instance.OnOnboardingPhaseCompleted -= HandleOnboardingPhaseComplete;
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
      // TODO: Will be implemented as part of COZMO-11581 Random Game flow
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

      // TODO: Will be implemented as part of COZMO-11581 Random Game flow
      _AskForGameMicIcon.gameObject.SetActive(false);
    }
  }
}
