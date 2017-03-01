using Anki.UI;
using System;
using UnityEngine;

// do not leave this screen until the user gives us external storage permission
public class AndroidPermissionBlocker : MonoBehaviour {

  public const string kPermission = "android.permission.READ_EXTERNAL_STORAGE";

  [SerializeField]
  private AnkiTextLegacy _Label;

  [SerializeField]
  private AnkiButtonLegacy _Button;

  private AndroidJavaObject _Activity;
  private AndroidJavaObject _PermissionUtil;
  private StartupManager _StartupManager;

  protected void Start() {
    _Activity = new AndroidJavaClass("com.unity3d.player.UnityPlayer").GetStatic<AndroidJavaObject>("currentActivity");
    _PermissionUtil = new AndroidJavaClass("com.anki.util.PermissionUtil");

    InitUI(true);
  }

  public void SetStartupManager(StartupManager instance) {
    _StartupManager = instance;
  }

  protected void InitUI(bool canGetPermission) {
    _Button.onClick.RemoveAllListeners();

    UnityEngine.Events.UnityAction action;
    if (canGetPermission) {
      action = () => _Activity.Call("unityRequestPermission", kPermission,
        this.gameObject.name, "OnPermissionResults");
      _Label.text = _StartupManager.GetBootString("boot.needStoragePermission");
      _Button.Text = _StartupManager.GetBootString("boot.continue");
    } else {
      action = () => _PermissionUtil.CallStatic("openAppSettings");
      _Label.text = _StartupManager.GetBootString("boot.appSettingsPermission");
      _Button.Text = _StartupManager.GetBootString("boot.settings");
    }
    _Button.Initialize(action, "button", "android_permission_blocker");
  }

  private bool CanAskForPermission() {
    return _PermissionUtil.CallStatic<bool>("shouldShowRationale", kPermission);
  }

  protected void OnPermissionResults(string resultString) {
    bool result = Boolean.Parse(resultString);
    if (result) {
      // cool, we're done here
      GameObject.Destroy(this.gameObject);
    } else {
      // did the user shoot themselves in the foot and make us direct them to app settings?
      InitUI(CanAskForPermission());
    }
  }
}
