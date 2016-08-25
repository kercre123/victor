using UnityEngine;
using System.Collections;

public class FaceEnrollmentUnlockView : Cozmo.UI.BaseView {

  public System.Action OnUnlockButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _UnlockButton;

  private void Awake() {
    _UnlockButton.Initialize(HandleOnUnlockButton, "unlock_button", this.DASEventViewName);
  }

  private void HandleOnUnlockButton() {
    if (OnUnlockButton != null) {
      OnUnlockButton();
    }
    CloseView();
  }

  protected override void CleanUp() {
    base.CleanUp();
  }
}
