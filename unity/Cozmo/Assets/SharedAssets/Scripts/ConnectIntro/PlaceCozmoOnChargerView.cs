using UnityEngine;
using System.Collections;

public class PlaceCozmoOnChargerView : Cozmo.UI.BaseView {

  public System.Action OnConnectButton;

  [SerializeField]
  private SimpleConnectView _KeepCozmoOnChargerConnectViewPrefab;
  private SimpleConnectView _KeepCozmoOnChargerConnectViewInstance;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ContinueButton;

  private void Awake() {
    _ContinueButton.Initialize(() => {
      _KeepCozmoOnChargerConnectViewInstance = UIManager.OpenView(_KeepCozmoOnChargerConnectViewPrefab);
      _KeepCozmoOnChargerConnectViewInstance.OnConnectButton += HandleConnectButton;
    }, "continue_button", this.DASEventViewName);
  }

  private void HandleConnectButton() {
    if (OnConnectButton != null) {
      OnConnectButton();
    }
  }

  protected override void CleanUp() {
    base.CleanUp();
    if (_KeepCozmoOnChargerConnectViewInstance != null) {
      UIManager.CloseViewImmediately(_KeepCozmoOnChargerConnectViewInstance);
    }
  }
}
