using UnityEngine;
using System.Collections;

public class UpdateAppView : Cozmo.UI.BaseView {

  [SerializeField]
  private Cozmo.UI.CozmoButton _UpdateAppButton;

  private void Start() {
    _UpdateAppButton.Initialize(HandleUpdateAppButton, "update_app_button", this.DASEventViewName);
  }

  private void HandleUpdateAppButton() {
#if UNITY_IOS
    Application.OpenURL("https://itunes.apple.com/app/cozmo/id1154282030");
#elif UNITY_ANDROID
    Application.OpenURL("https://play.google.com/store/apps/details?id=com.anki.cozmo");
#endif
  }

  protected override void CleanUp() {
    base.CleanUp();
    // force a disconnect so we can reconnect to the robot.
    // we are doing this after the screen closes purely for QA to have a way to downgrade robots
    RobotEngineManager.Instance.StartIdleTimeout(0f, 0f);
  }
}
