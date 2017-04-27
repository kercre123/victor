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

    // COZMO-10941 IVY TODO: Add NeedConnectModal prefab

    // Use this for initialization
    void Start() {
      UIManager.Instance.BackgroundColorController.SetBackgroundColor(BackgroundColorController.BackgroundColor.TintMe,
                                                                      Color.gray);
      if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
        _ConnectButton.Initialize(HandleMockConnectButtonPressed, "connect_button", "needs_unconnected_view");
      }
      else {
        _ConnectButton.Initialize(HandleConnectButtonPressed, "connect_button", "needs_unconnected_view");
      }

      // COZMO-10941 IVY TODO: Listen to button events from NeedsMeterWidget in order to open NeedConnect modal 
      _MetersWidget.Initialize(enableButtonBasedOnNeeds: false, dasParentDialogName: DASEventDialogName, baseDialog: this);
    }

    protected override void CleanUp() {

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
  }
}