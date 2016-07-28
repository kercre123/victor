using UnityEngine;
using System.Collections;

public class InvalidPinView : Cozmo.UI.BaseView {

  public System.Action OnRetryPin;

  [SerializeField]
  private Cozmo.UI.CozmoButton _RetryButton;

  private void Start() {
    _RetryButton.Initialize(() => { if (OnRetryPin != null) OnRetryPin(); }, "retry_button", this.DASEventViewName);

  }

  protected override void CleanUp() {

  }
}
