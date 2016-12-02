using UnityEngine;
using System.Collections;

public class InvalidPinView : Cozmo.UI.BaseModal {

  public System.Action OnRetryPin;

  [SerializeField]
  private Cozmo.UI.CozmoButton _RetryButton;

  private void Awake() {
    _RetryButton.Initialize(() => { if (OnRetryPin != null) OnRetryPin(); }, "retry_button", this.DASEventViewName);

  }

  protected override void CleanUp() {

  }
}
