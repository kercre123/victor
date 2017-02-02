using UnityEngine;
using System.Collections;

public class SimpleConnectModal : Cozmo.UI.BaseModal {

  public System.Action OnConnectButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ConnectButton;

  private void Awake() {
    _ConnectButton.Initialize(HandleConnectButton, "connect_button", "simple_connect_view");
  }

  private void Start() {
    _ConnectButton.Text = Localization.Get(LocalizationKeys.kLabelConnect);
  }

  private void HandleConnectButton() {
    if (OnConnectButton != null) {
      OnConnectButton();
    }
  }

  protected override void CleanUp() {

  }
}
