using UnityEngine;
using System.Collections;

public class InvalidPinModal : Cozmo.UI.BaseModal {

  public System.Action OnRetryPin;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _RetryButton;

  private void Awake() {
    _RetryButton.Initialize(() => { if (OnRetryPin != null) OnRetryPin(); }, "retry_button", this.DASEventDialogName);

  }

  protected override void CleanUp() {

  }
}
