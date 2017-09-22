using Cozmo.Needs.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;

namespace Cozmo.ConnectionFlow.UI {
  public class NeedsUnconnectedView : BaseView {

    public delegate void NeedsUnconnectedButtonHandler();
    public event NeedsUnconnectedButtonHandler OnConnectButtonPressed;
    public event NeedsUnconnectedButtonHandler OnMockConnectButtonPressed;

    [SerializeField]
    private CozmoButton _ConnectButton;

    [SerializeField]
    private RectTransform _MetersAnchor;

    [SerializeField]
    private NeedsMetersWidget _MetersWidgetPrefab;
    private NeedsMetersWidget _MetersWidget;

    [SerializeField]
    private CozmoButton _SettingsButton;

    [SerializeField]
    private CozmoButton _HelpButton;

    [SerializeField]
    private SettingsModal _SettingsModalPrefab;
    private SettingsModal _SettingsModalInstance;

    [SerializeField]
    private BaseModal _HelpTipsModalPrefab;
    private BaseModal _HelpTipsModalInstance;

    // Use this for initialization
    void Start() {
      if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
        _ConnectButton.Initialize(HandleMockConnectButtonPressed, "connect_button", "needs_unconnected_view");
      }
      else {
        _ConnectButton.Initialize(HandleConnectButtonPressed, "connect_button", "needs_unconnected_view");
      }

      _SettingsButton.Initialize(HandleSettingsButton, "settings_button", "needs_unconnected_view");
      _HelpButton.Initialize(HandleHelpButton, "help_button", "needs_unconnected_view");

      _MetersWidget = UIManager.CreateUIElement(_MetersWidgetPrefab.gameObject, _MetersAnchor).GetComponent<NeedsMetersWidget>();
      _MetersWidget.Initialize(dasParentDialogName: DASEventDialogName, baseDialog: this);
      _MetersWidget.OnPlayPressed += HandleMeterPressed;
      _MetersWidget.OnEnergyPressed += HandleMeterPressed;
      _MetersWidget.OnRepairPressed += HandleMeterPressed;

      // Request Locale gives us ability to get real platform dependent locale
      // whereas unity just gives us the language
      bool dataCollectionEnabled = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.DataCollectionEnabled;
      if (!dataCollectionEnabled) {
        // by default on so only needs to get set if false.
        RobotEngineManager.Instance.Message.RequestDataCollectionOption =
                      Singleton<Anki.Cozmo.ExternalInterface.RequestDataCollectionOption>.Instance.Initialize(dataCollectionEnabled);
        RobotEngineManager.Instance.SendMessage();
      }

      WhatsNew.WhatsNewModalManager.OnCodeLabConnectPressed += HandleCodeLabConnectPressed;
    }

    protected override void CleanUp() {
      _MetersWidget.OnPlayPressed -= HandleMeterPressed;
      _MetersWidget.OnEnergyPressed -= HandleMeterPressed;
      _MetersWidget.OnRepairPressed -= HandleMeterPressed;
      WhatsNew.WhatsNewModalManager.OnCodeLabConnectPressed -= HandleCodeLabConnectPressed;
      Destroy(_MetersWidget.gameObject);
    }

    protected override void RaiseDialogClosed() {
      if (_MetersWidget != null) {
        _MetersWidget.gameObject.SetActive(false);
      }
      base.RaiseDialogClosed();
    }

    private void HandleConnectButtonPressed() {
      if (OnConnectButtonPressed != null) {
        OnConnectButtonPressed();
        Invoke("HideSelf", 1.0f);
      }
    }

    private void HandleMockConnectButtonPressed() {
      if (OnMockConnectButtonPressed != null) {
        OnMockConnectButtonPressed();
        Invoke("HideSelf", 1.0f);
      }
    }

    // hide this screen, it was still visible during the gaps between future screens (COZMO-12283)
    private void HideSelf() {
      gameObject.SetActive(false);
    }

    private void HandleSettingsButton() {
      UIManager.OpenModal(_SettingsModalPrefab, new ModalPriorityData(), HandleSettingsModalCreated);
    }

    private void HandleSettingsModalCreated(BaseModal newModal) {
      _SettingsModalInstance = (SettingsModal)newModal;
      _SettingsModalInstance.Initialize(this);
    }

    private void HandleHelpButton() {
      UIManager.OpenModal(_HelpTipsModalPrefab, new ModalPriorityData(), HandleHelpModalCreated);
    }

    private void HandleHelpModalCreated(BaseModal newModal) {
      _HelpTipsModalInstance = newModal;
      _HelpTipsModalInstance.Initialize();
    }

    private void HandleMeterPressed() {
      AlertModalButtonData okayButtonData = new AlertModalButtonData("okay_button",
                                                                     LocalizationKeys.kButtonOkay);

      AlertModalData needToConnectData = new AlertModalData("need_to_connect_alert",
                                                            LocalizationKeys.kNeedsUnconnectedNeedToConnectTitle,
                                                            LocalizationKeys.kNeedsUnconnectedNeedToConnectDescription,
                                                            okayButtonData,
                                                            showCloseButton: true);

      UIManager.OpenAlert(needToConnectData, new ModalPriorityData());
    }

    private void HandleCodeLabConnectPressed() {
      if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
        HandleMockConnectButtonPressed();
      }
      else {
        HandleConnectButtonPressed();
      }
    }
  }
}