using UnityEngine;
using System.Collections;

public class UpdateAppView : Cozmo.UI.BaseView {

  [SerializeField]
  private Cozmo.UI.CozmoButton _UpdateAppButton;

  private void Awake() {
    _UpdateAppButton.Initialize(HandleUpdateAppButton, "update_app_button", this.DASEventViewName);
  }

  private void HandleUpdateAppButton() {
#if UNITY_IOS
    Application.OpenURL("http://go.anki.com/cozmo-app-upgrade-ios");
#elif UNITY_ANDROID
    Application.OpenURL("http://go.anki.com/cozmo-app-upgrade-google");
    //http://go.anki.com/cozmo-app-upgrade-amazon
#endif
  }

  protected override void CleanUp() {
    base.CleanUp();
    // force a disconnect so we can reconnect to the robot.
    // we are doing this after the screen closes purely for QA to have a way to downgrade robots
    RobotEngineManager.Instance.StartIdleTimeout(0f, 0f);
  }
}
