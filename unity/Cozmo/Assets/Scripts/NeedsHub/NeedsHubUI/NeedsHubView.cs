using Anki.Cozmo;
using Cozmo.Energy.UI;
using Cozmo.Play.UI;
using Cozmo.Repair.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;

namespace Cozmo.Needs.UI {
  public class NeedsHubView : BaseView {
    private const int kCubesCount = 3;

    public delegate void ActivitiesClickedHandler();
    public event ActivitiesClickedHandler OnActivitiesButtonClicked;

    public delegate void StartChallengeClickedHandler();
    public event StartChallengeClickedHandler OnStartChallengeClicked;

    [SerializeField]
    private CozmoButton _PlayRandomChallengeButton;

    [SerializeField]
    private CozmoButton _ActivitiesButton;

    [SerializeField]
    private CozmoButton _SettingsButton;

    [SerializeField]
    private NeedsSettingsWidget _SettingsWidget;

    private bool _SettingsIsOpen = false;

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

    public void Start() {
      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe,
                                                                      Color.gray);
      _PlayRandomChallengeButton.Initialize(HandlePlayChallengeButtonClicked, "play_random_Challenge_button", DASEventDialogName);
      _ActivitiesButton.Initialize(HandleActivitiesButtonClicked, "open_activities_button", DASEventDialogName);
      _SettingsButton.Initialize(HandleSettingsButton, "settings_button", DASEventDialogName);

      _MetersWidget.Initialize(enableButtonBasedOnNeeds: true, dasParentDialogName: DASEventDialogName, baseDialog: this);
      _MetersWidget.OnRepairPressed += HandleRepairButton;
      _MetersWidget.OnEnergyPressed += HandleEnergyButton;
      _MetersWidget.OnPlayPressed += HandlePlayButton;

      NeedsStateManager nsm = NeedsStateManager.Instance;
      NeedBracketId repairBracket = nsm.GetCurrentDisplayBracket(NeedId.Repair);
      NeedBracketId energyBracket = nsm.GetCurrentDisplayBracket(NeedId.Energy);
      EnableButtonsBasedOnBrackets(repairBracket, energyBracket);

      this.DialogOpenAnimationFinished += HandleDialogFinishedOpenAnimation;
    }

    protected override void CleanUp() {
      _MetersWidget.OnRepairPressed -= HandleRepairButton;
      _MetersWidget.OnEnergyPressed -= HandleEnergyButton;
      _MetersWidget.OnPlayPressed -= HandlePlayButton;
      NeedsStateManager.Instance.OnNeedsBracketChanged -= HandleLatestNeedsBracketChanged;
    }

    protected override void ConstructOpenAnimation(Sequence openAnimation) {
      ConstructDefaultFadeOpenAnimation(openAnimation);
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
      ConstructDefaultFadeCloseAnimation(closeAnimation);
    }

    private void HandlePlayChallengeButtonClicked() {
      if (OnStartChallengeClicked != null) {
        OnStartChallengeClicked();
      }
    }

    private void HandleActivitiesButtonClicked() {
      if (OnActivitiesButtonClicked != null) {
        OnActivitiesButtonClicked();
      }
    }

    private void HandleSettingsButton() {
      if (!_SettingsIsOpen) {
        _MetersWidget.gameObject.SetActive(false);
        _PlayRandomChallengeButton.gameObject.SetActive(false);
        _ActivitiesButton.gameObject.SetActive(false);
        ShowSettings();
        _SettingsIsOpen = true;
      }
      else {
        _MetersWidget.gameObject.SetActive(true);
        _PlayRandomChallengeButton.gameObject.SetActive(true);
        _ActivitiesButton.gameObject.SetActive(true);
        _SettingsWidget.HideSettings();
        _SettingsIsOpen = false;
      }
    }

    private void ShowSettings() {
      _SettingsWidget.ShowSettings();

      // auto scroll to the cubes setting panel if we don't have three cubes connected.
      if (RobotEngineManager.Instance.CurrentRobot != null && RobotEngineManager.Instance.CurrentRobot.LightCubes.Count != kCubesCount) {
        _SettingsWidget.ScrollToCubeSettings();
      }
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
      NeedsStateManager nsm = NeedsStateManager.Instance;
      NeedBracketId repairBracket = nsm.PopLatestEngineBracket(NeedId.Repair);
      NeedBracketId energyBracket = nsm.PopLatestEngineBracket(NeedId.Energy);
      EnableButtonsBasedOnBrackets(repairBracket, energyBracket);
      NeedsStateManager.Instance.OnNeedsBracketChanged += HandleLatestNeedsBracketChanged;
    }

    private void HandleLatestNeedsBracketChanged(NeedsActionId actionId, NeedId needId) {
      if (needId == NeedId.Repair || needId == NeedId.Energy) {
        NeedsStateManager nsm = NeedsStateManager.Instance;
        NeedBracketId repairBracket = nsm.PopLatestEngineBracket(NeedId.Repair);
        NeedBracketId energyBracket = nsm.PopLatestEngineBracket(NeedId.Energy);
        EnableButtonsBasedOnBrackets(repairBracket, energyBracket);
      }
    }

    private void EnableButtonsBasedOnBrackets(NeedBracketId repairBracket, NeedBracketId energyBracket) {
      if (repairBracket == NeedBracketId.Critical || repairBracket == NeedBracketId.Warning
          || energyBracket == NeedBracketId.Critical || energyBracket == NeedBracketId.Warning) {
        _PlayRandomChallengeButton.Interactable = false;
        _ActivitiesButton.Interactable = false;
      }
      else {
        _PlayRandomChallengeButton.Interactable = true;
        _ActivitiesButton.Interactable = true;
      }
    }
  }
}