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
    private CozmoButton _HelpButton;

    [SerializeField]
    private CozmoButton _RepairButton;

    [SerializeField]
    private CozmoButton _FeedButton;

    [SerializeField]
    private StarBar _StarBar;

    [SerializeField]
    private SettingsModal _SettingsModalPrefab;
    private SettingsModal _SettingsModalInstance;

    [SerializeField]
    private BaseModal _HelpTipsModalPrefab;
    private BaseModal _HelpTipsModalInstance;

    [SerializeField]
    private RectTransform _MetersAnchor;

    [SerializeField]
    private NeedsMetersWidget _MetersWidgetPrefab;
    private NeedsMetersWidget _MetersWidget;

    [SerializeField]
    private NeedsRepairModal _NeedsRepairModalPrefab;
    private NeedsRepairModal _NeedsRepairModalInstance;

    [SerializeField]
    private NeedsEnergyModal _NeedsEnergyModalPrefab;
    private NeedsEnergyModal _NeedsEnergyModalInstance;

    [SerializeField]
    private CozmoImage _SettingsAlertImage;

    [SerializeField]
    private GameObject _StandardTitle;
    [SerializeField]
    private GameObject _LiquidMetalTitle;

    public void Start() {
      _ActivitiesButton.Initialize(HandleActivitiesButtonClicked, "open_activities_button", DASEventDialogName);
      _SparksButton.Initialize(HandleSparksButtonClicked, "open_sparks_button", DASEventDialogName);
      _SettingsButton.Initialize(HandleSettingsButton, "settings_button", DASEventDialogName);
      _HelpButton.Initialize(HandleHelpButton, "help_button", DASEventDialogName);

      _MetersWidget = UIManager.CreateUIElement(_MetersWidgetPrefab.gameObject, _MetersAnchor).GetComponent<NeedsMetersWidget>();
      _MetersWidget.Initialize(dasParentDialogName: DASEventDialogName, baseDialog: this);
      _MetersWidget.OnRepairPressed += HandleRepairButton;
      _MetersWidget.OnEnergyPressed += HandleEnergyButton;

      _RepairButton.Initialize(HandleRepairButton, "repair_need_meter_button", DASEventDialogName);
      _FeedButton.Initialize(HandleEnergyButton, "energy_need_meter_button", DASEventDialogName);

      NeedsStateManager nsm = NeedsStateManager.Instance;
      NeedsValue repairValue, energyValue;
      repairValue = nsm.GetCurrentDisplayValue(NeedId.Repair);
      energyValue = nsm.GetCurrentDisplayValue(NeedId.Energy);
      EnableButtonsBasedOnBrackets(repairValue.Bracket, energyValue.Bracket);

      bool liquidMetal = false;
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.OnNumBlocksConnectedChanged += HandleBlockConnectivityChanged;
        BodyColor robotBodyColor = RobotEngineManager.Instance.CurrentRobot.BodyColor;
        if (robotBodyColor == BodyColor.CE_LM_v15) {
          liquidMetal = true;
        }
      }

      _StandardTitle.SetActive(!liquidMetal);
      _LiquidMetalTitle.SetActive(liquidMetal);

      _SettingsAlertImage.gameObject.SetActive(!RobotEngineManager.Instance.AllCubesConnected());

      this.DialogOpenAnimationFinished += HandleDialogFinishedOpenAnimation;

      OnboardingManager.Instance.InitInitalOnboarding(this);
    }

    protected override void CleanUp() {
      _MetersWidget.OnRepairPressed -= HandleRepairButton;
      _MetersWidget.OnEnergyPressed -= HandleEnergyButton;
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

    private void HandleHelpButton() {
      UIManager.OpenModal(_HelpTipsModalPrefab, new ModalPriorityData(), HandleHelpModalCreated);
    }

    private void HandleHelpModalCreated(BaseModal newModal) {
      _HelpTipsModalInstance = newModal;
      _HelpTipsModalInstance.Initialize();
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
      // Onboarding overrides this stuff.
      if (OnboardingManager.Instance.IsOnboardingOverridingNavButtons()) {
        return;
      }
      // Sparks button is labelled play.
      if (repairBracket == NeedBracketId.Critical) {
        _FeedButton.Interactable = false;
        _SparksButton.Interactable = false;
      }
      else if (energyBracket == NeedBracketId.Critical) {
        _FeedButton.Interactable = true;
        _SparksButton.Interactable = false;
      }
      else {
        _FeedButton.Interactable = true;
        _SparksButton.Interactable = true;
      }
    }

    #region Onboarding
    public CozmoImage OnboardingBlockoutImage;
    public CozmoImage NavBackgroundImage;
    public NeedsMetersWidget MetersWidget {
      get { return _MetersWidget; }
    }
    public CozmoButton DiscoverButton {
      get { return _ActivitiesButton; }
    }
    public CozmoButton RepairButton {
      get { return _RepairButton; }
    }
    public CozmoButton FeedButton {
      get { return _FeedButton; }
    }
    public CozmoButton PlayButton {
      get { return _SparksButton; }
    }
    public StarBar RewardBar {
      get { return _StarBar; }
    }
    #endregion
  }
}