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

    public void Start() {
      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe,
                                                                      UIColorPalette.GeneralBackgroundColor);
      _ActivitiesButton.Initialize(HandleActivitiesButtonClicked, "open_activities_button", DASEventDialogName);
      _SparksButton.Initialize(HandleSparksButtonClicked, "open_sparks_button", DASEventDialogName);
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
        _ActivitiesButton.Interactable = false;
        _SparksButton.Interactable = false;
      }
      else {
        _ActivitiesButton.Interactable = true;
        _SparksButton.Interactable = true;
      }
    }
  }
}