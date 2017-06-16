using Anki.Cozmo;
using Cozmo.Energy.UI;
using Cozmo.Play.UI;
using Cozmo.Repair.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;

namespace Cozmo.Needs.UI {
  public class NeedsHubView : BaseView {
    public delegate void ActivitiesClickedHandler();
    public event ActivitiesClickedHandler OnActivitiesButtonClicked;

    public delegate void SparksClickedHandler();
    public event SparksClickedHandler OnSparksButtonClicked;

    [SerializeField]
    private CozmoButton _ActivitiesButton;

    [SerializeField]
    private CozmoButton _SettingsButton;

    [SerializeField]
    private CozmoButton _SparksButton;

    [SerializeField]
    private StarBar _StarBar;

    [SerializeField]
    private SettingsModal _SettingsModalPrefab;
    private SettingsModal _SettingsModalInstance;

    [SerializeField]
    private NeedsMetersWidget _MetersWidget;

    [SerializeField]
    private NeedsRepairModal _NeedsRepairModalPrefab;
    private NeedsRepairModal _NeedsRepairModalInstance;

    [SerializeField]
    private NeedsEnergyModal _NeedsEnergyModalPrefab;
    private NeedsEnergyModal _NeedsEnergyModalInstance;

    [SerializeField]
    private NeedsPlayModal _NeedsPlayModalPrefab;
    private NeedsPlayModal _NeedsPlayModalInstance;

    [SerializeField]
    private CozmoImage _SettingsAlertImage;

    public void Start() {
      _ActivitiesButton.Initialize(HandleActivitiesButtonClicked, "open_activities_button", DASEventDialogName);
      _SparksButton.Initialize(HandleSparksButtonClicked, "open_sparks_button", DASEventDialogName);
      _SettingsButton.Initialize(HandleSettingsButton, "settings_button", DASEventDialogName);

      _MetersWidget.Initialize(enableButtonBasedOnNeeds: true, dasParentDialogName: DASEventDialogName, baseDialog: this);
      _MetersWidget.OnRepairPressed += HandleRepairButton;
      _MetersWidget.OnEnergyPressed += HandleEnergyButton;
      _MetersWidget.OnPlayPressed += HandlePlayButton;

      NeedsStateManager nsm = NeedsStateManager.Instance;
      NeedsValue repairValue, energyValue;
      repairValue = nsm.GetCurrentDisplayValue(NeedId.Repair);
      energyValue = nsm.GetCurrentDisplayValue(NeedId.Energy);
      EnableButtonsBasedOnBrackets(repairValue.Bracket, energyValue.Bracket);

      RobotEngineManager.Instance.CurrentRobot.OnNumBlocksConnectedChanged += HandleBlockConnectivityChanged;
      _SettingsAlertImage.gameObject.SetActive(!RobotEngineManager.Instance.AllCubesConnected());

      this.DialogOpenAnimationFinished += HandleDialogFinishedOpenAnimation;

      OnboardingManager.Instance.InitInitalOnboarding(this);
    }

    protected override void CleanUp() {
      _MetersWidget.OnRepairPressed -= HandleRepairButton;
      _MetersWidget.OnEnergyPressed -= HandleEnergyButton;
      _MetersWidget.OnPlayPressed -= HandlePlayButton;
      NeedsStateManager.Instance.OnNeedsBracketChanged -= HandleLatestNeedsBracketChanged;

      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.OnNumBlocksConnectedChanged -= HandleBlockConnectivityChanged;
      }
    }

    private void HandleBlockConnectivityChanged(int blocksConnected) {
      _SettingsAlertImage.gameObject.SetActive(!RobotEngineManager.Instance.AllCubesConnected());
    }

    private void HandleActivitiesButtonClicked() {
      if (OnActivitiesButtonClicked != null) {
        OnActivitiesButtonClicked();
      }
    }

    private void HandleSparksButtonClicked() {
      if (OnSparksButtonClicked != null) {
        OnSparksButtonClicked();
      }
    }

    private void HandleSettingsButton() {
      UIManager.OpenModal(_SettingsModalPrefab, new ModalPriorityData(), HandleSettingsModalCreated);
    }

    private void HandleSettingsModalCreated(BaseModal newModal) {
      _SettingsModalInstance = (SettingsModal)newModal;
      _SettingsModalInstance.Initialize(this);
    }

    private void HandleRepairButton() {
      UIManager.OpenModal(_NeedsRepairModalPrefab, new ModalPriorityData(), HandleRepairModalCreated);
    }

    private void HandleRepairModalCreated(BaseModal newModal) {
      _NeedsRepairModalInstance = (NeedsRepairModal)newModal;
      _NeedsRepairModalInstance.InitializeRepairModal();
    }

    private void HandleEnergyButton() {
      UIManager.OpenModal(_NeedsEnergyModalPrefab, new ModalPriorityData(), HandleEnergyModalCreated);
    }

    private void HandleEnergyModalCreated(BaseModal newModal) {
      _NeedsEnergyModalInstance = (NeedsEnergyModal)newModal;
      _NeedsEnergyModalInstance.InitializeEnergyModal();
    }

    private void HandlePlayButton() {
      UIManager.OpenModal(_NeedsPlayModalPrefab, new ModalPriorityData(), HandlePlayModalCreated);
    }

    private void HandlePlayModalCreated(BaseModal newModal) {
      _NeedsPlayModalInstance = (NeedsPlayModal)newModal;
      _NeedsPlayModalInstance.InitializePlayModal();
    }

    private void HandleDialogFinishedOpenAnimation() {
      PopLatestBracketAndUpdateButtons();
      NeedsStateManager.Instance.OnNeedsBracketChanged += HandleLatestNeedsBracketChanged;

      OnboardingManager.Instance.StartAnyPhaseIfNeeded();
    }

    private void HandleLatestNeedsBracketChanged(NeedsActionId actionId, NeedId needId) {
      if (needId == NeedId.Repair || needId == NeedId.Energy) {
        PopLatestBracketAndUpdateButtons();
      }
    }

    private void PopLatestBracketAndUpdateButtons() {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      NeedsValue repairValue, energyValue;
      repairValue = nsm.PopLatestEngineValue(NeedId.Repair);
      energyValue = nsm.PopLatestEngineValue(NeedId.Energy);
      EnableButtonsBasedOnBrackets(repairValue.Bracket, energyValue.Bracket);
    }

    private void EnableButtonsBasedOnBrackets(NeedBracketId repairBracket, NeedBracketId energyBracket) {
      if (repairBracket == NeedBracketId.Critical || energyBracket == NeedBracketId.Critical) {
        _SparksButton.Interactable = false;
      }
      else {
        _SparksButton.Interactable = true;
      }
    }

    #region Onboarding
    public CozmoButton DiscoverButton {
      get { return _ActivitiesButton; }
    }
    public CozmoButton RepairButton {
      get { return _MetersWidget.RepairButton; }
    }
    public CozmoButton FeedButton {
      get { return _MetersWidget.FeedButton; }
    }
    public CozmoButton PlayButton {
      get { return _SparksButton; }
    }
    #endregion
  }
}