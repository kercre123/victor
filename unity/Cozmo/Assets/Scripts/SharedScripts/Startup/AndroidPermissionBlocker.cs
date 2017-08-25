using TMPro;
using System;
using UnityEngine;
using UnityEngine.UI;

// do not leave this screen until the user gives us external storage permission
public class AndroidPermissionBlocker : MonoBehaviour {

  public const string kPermission = "android.permission.READ_EXTERNAL_STORAGE";

  // These are regular unity UI / Anki objects and not Cozmo objects because this screen is used before the ThemeSystem is initialized
  [SerializeField]
  private TextMeshProUGUI _LatinInstructionLabel;

  [SerializeField]
  private TextMeshProUGUI _LatinButtonLabel;

  [SerializeField]
  private TextMeshProUGUI _JPInstructionLabel;

  [SerializeField]
  private TextMeshProUGUI _JPButtonLabel;

  [SerializeField]
  private Anki.UI.AnkiButtonLegacy _Button;

  private AndroidJavaObject _Activity;
  private AndroidJavaObject _PermissionUtil;
  private StartupManager _StartupManager;

  private TextMeshProUGUI _InstructionLabel;
  private TextMeshProUGUI _ButtonLabel;

  public void SetStartupManager(StartupManager instance) {
    _StartupManager = instance;
    _Activity = new AndroidJavaClass("com.unity3d.player.UnityPlayer").GetStatic<AndroidJavaObject>("currentActivity");
    _PermissionUtil = new AndroidJavaClass("com.anki.util.PermissionUtil");

    bool useJPLanguage = Localization.UseJapaneseFont();
    _LatinInstructionLabel.gameObject.SetActive(!useJPLanguage);
    _LatinButtonLabel.gameObject.SetActive(!useJPLanguage);
    _JPInstructionLabel.gameObject.SetActive(useJPLanguage);
    _JPButtonLabel.gameObject.SetActive(useJPLanguage);
    _InstructionLabel = useJPLanguage ? _JPInstructionLabel : _LatinInstructionLabel;
    _ButtonLabel = useJPLanguage ? _JPButtonLabel : _LatinButtonLabel;

    InitUI(true);
  }

  protected void InitUI(bool canGetPermission) {
    _Button.onClick.RemoveAllListeners();

    UnityEngine.Events.UnityAction action;
    if (canGetPermission) {
      action = () => _Activity.Call("unityRequestPermission", kPermission,
        this.gameObject.name, "OnPermissionResults");
      _InstructionLabel.text = _StartupManager.GetBootString(LocalizationKeys.kBootNeedStoragePermission);
      _ButtonLabel.text = _StartupManager.GetBootString(LocalizationKeys.kBootContinue);
    }
    else {
      action = () => _PermissionUtil.CallStatic("openAppSettings");
      _InstructionLabel.text = _StartupManager.GetBootString(LocalizationKeys.kBootAppSettingsPermission);
      _ButtonLabel.text = _StartupManager.GetBootString(LocalizationKeys.kBootSettings);
    }
    _Button.Initialize(action, "button", "android_permission_blocker");
    //despite the call to removeAllListeners, there is no need for a repairListeners call.
    //repairListeners is only necessary for CozmoButton.
  }

  private bool CanAskForPermission() {
    return _PermissionUtil.CallStatic<bool>("shouldShowRationale", kPermission);
  }

  protected void OnPermissionResults(string resultString) {
    bool result = Boolean.Parse(resultString);
    if (result) {
      // cool, we're done here
      GameObject.Destroy(this.gameObject);
    }
    else {
      // did the user shoot themselves in the foot and make us direct them to app settings?
      InitUI(CanAskForPermission());
    }
  }
}
