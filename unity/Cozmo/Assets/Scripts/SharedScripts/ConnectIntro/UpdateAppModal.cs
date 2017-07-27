using UnityEngine;
using System.Collections;

public class UpdateAppModal : Cozmo.UI.BaseModal {

  [SerializeField]
  private Cozmo.UI.CozmoButton _UpdateAppButton;

  [SerializeField]
  private UnityEngine.UI.Image _ButtonImage;

  [SerializeField]
  private Sprite _KindleStoreSprite;

  private void Awake() {
    _UpdateAppButton.Initialize(HandleUpdateAppButton, "update_app_button", this.DASEventDialogName);

#if UNITY_ANDROID && !UNITY_EDITOR
    AndroidJavaClass dasClass = new AndroidJavaClass("com.anki.daslib.DAS");
    bool isKindle = dasClass.CallStatic<bool>("IsOnKindle");

    if (isKindle) {
      _ButtonImage.overrideSprite = _KindleStoreSprite;
    }
#endif

  }

  private void HandleUpdateAppButton() {
#if UNITY_IOS
    Application.OpenURL("http://go.anki.com/cozmo-app-upgrade-ios");
#elif UNITY_ANDROID && !UNITY_EDITOR

    AndroidJavaClass dasClass = new AndroidJavaClass("com.anki.daslib.DAS");
    bool isKindle = dasClass.CallStatic<bool>("IsOnKindle");

    if (isKindle) {
      Application.OpenURL("http://go.anki.com/cozmo-app-upgrade-amazon");
    }
    else {
      Application.OpenURL("http://go.anki.com/cozmo-app-upgrade-google");
    }
#endif

#if UNITY_ANDROID && UNITY_EDITOR
    Application.OpenURL("http://go.anki.com/cozmo-app-upgrade-google");
#endif

  }

  protected override void CleanUp() {
    base.CleanUp();
    // force a disconnect so we can reconnect to the robot.
    // we are doing this after the screen closes purely for QA to have a way to downgrade robots
    RobotEngineManager.Instance.StartIdleTimeout(0f, 0f);
  }
}
