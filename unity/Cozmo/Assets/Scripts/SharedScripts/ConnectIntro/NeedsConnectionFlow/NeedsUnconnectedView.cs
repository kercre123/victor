using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Cozmo.UI;

namespace Cozmo.ConnectionFlow.UI {
  public class NeedsUnconnectedView : BaseView {

    public delegate void NeedsUnconnectedButtonHandler();
    public event NeedsUnconnectedButtonHandler OnConnectButtonPressed;
    public event NeedsUnconnectedButtonHandler OnMockConnectButtonPressed;

    [SerializeField]
    private CozmoButton _ConnectButton;

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