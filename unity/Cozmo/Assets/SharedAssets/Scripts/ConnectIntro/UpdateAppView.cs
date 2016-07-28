using UnityEngine;
using System.Collections;

public class UpdateAppView : Cozmo.UI.BaseView {

  [SerializeField]
  private Cozmo.UI.CozmoButton _UpdateAppButton;

  private void Start() {
    _UpdateAppButton.Initialize(HandleUpdateAppButton, "update_app_button", this.DASEventViewName);
  }

  private void HandleUpdateAppButton() {
    // TODO: replace with App Store / Google Play URL
    Application.OpenURL("https://www.anki.com/");
  }

  protected override void CleanUp() {

  }
}
