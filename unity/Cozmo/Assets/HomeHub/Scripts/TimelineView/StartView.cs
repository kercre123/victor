using UnityEngine;
using System.Collections;
using Anki.UI;
using UnityEngine.UI;
using Cozmo.UI;

public class StartView : BaseView {

  [SerializeField]
  private AnkiButton _ConnectButton;

  [SerializeField]
  private Color _DisconnectedColor;

  [SerializeField]
  private Image _WifiIndicator;

  [SerializeField]
  private Image _BluetoothIndicator;

  public event System.Action OnConnectClicked;

  private void Awake() {
    _ConnectButton.onClick.AddListener(HandleConnectClicked);
  }

  private bool IsWifiConnected() {
    return Application.internetReachability == NetworkReachability.ReachableViaLocalAreaNetwork;
  }

  private bool IsBluetoothConnected() {
    // TODO: Figure this one out.
    return true;
  }

  private void HandleConnectClicked() {
    if (OnConnectClicked != null) {
      OnConnectClicked();
    }
  }

  #region implemented abstract members of BaseView

  protected override void CleanUp() {
    OnConnectClicked = null;
  }

  #endregion
}
