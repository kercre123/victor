using UnityEngine;
using System.Collections;

public class ConnectionRejectedScreen : MonoBehaviour {

  public System.Action OnCancelButton;
  public System.Action OnRetryButton;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _CancelButton;

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _RetryButton;

  private void Awake() {

    _CancelButton.Initialize(() => {
      if (OnCancelButton != null) {
        OnCancelButton();
      }
    }, "cancel_button", "connection_rejected_screen");

    _RetryButton.Initialize(() => {
      if (OnRetryButton != null) {
        OnRetryButton();
      }
    }, "retry_button", "connection_rejected_screen");

  }
}
