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
    private NeedsMetersWidget _MetersWidget;

    // Use this for initialization
    void Start() {
      if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
        _ConnectButton.Initialize(HandleMockConnectButtonPressed, "connect_button", "needs_unconnected_view");
      }
      else {
        _ConnectButton.Initialize(HandleConnectButtonPressed, "connect_button", "needs_unconnected_view");
      }

      _MetersWidget.Initialize(enableButtonBasedOnNeeds: false, dasParentDialogName: DASEventDialogName, baseDialog: this);
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
    }

    protected override void CleanUp() {
      _MetersWidget.OnPlayPressed -= HandleMeterPressed;
      _MetersWidget.OnEnergyPressed -= HandleMeterPressed;
      _MetersWidget.OnRepairPressed -= HandleMeterPressed;
    }

    protected override void ConstructOpenAnimation(Sequence openAnimation) {
      ConstructDefaultFadeOpenAnimation(openAnimation);
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
      ConstructDefaultFadeCloseAnimation(closeAnimation);
    }

    private void HandleConnectButtonPressed() {
      if (OnConnectButtonPressed != null) {
        OnConnectButtonPressed();
      }
    }

    private void HandleMockConnectButtonPressed() {
      if (OnMockConnectButtonPressed != null) {
        OnMockConnectButtonPressed();
      }
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
  }
}