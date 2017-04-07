using UnityEngine;
using UnityEngine.UI;
using Newtonsoft.Json;
using Anki.Cozmo.ExternalInterface;

public class DebugDisplayPane : MonoBehaviour {

  [SerializeField]
  private Button _ToggleDebugStringButton;

  [SerializeField]
  private Toggle _ToggleDebugStringType;

  [SerializeField]
  private GameObject _RobotStateTextFieldPrefab;

  // This attribute is static so the text string can be removed even when exiting and coming back to the debug menu
  private static GameObject _RobotStateTextFieldInstance;

  [SerializeField]
  private Toggle _ToggleShowLocDebug;

  [SerializeField]
  private InputField _InputRandSeed;
  [SerializeField]
  private Button _ButtonSetRandSeed;

  [SerializeField]
  private Text _DeviceID;

  [SerializeField]
  private Text _AppRunID;

  [SerializeField]
  private Text _LastAppRunID;

  [SerializeField]
  private Text _BuildVersion;

  [SerializeField]
  private Text _ActiveVariantText;

  private void Start() {

    _ToggleDebugStringButton.onClick.AddListener(HandleToggleDebugString);

    _ToggleDebugStringType.onValueChanged.AddListener(HandleToggleDebugStringType);

    _ToggleShowLocDebug.onValueChanged.AddListener(HandleToggleShowLocDebug);

    _ButtonSetRandSeed.onClick.AddListener(HandleSetRandSeed);

    _ToggleShowLocDebug.isOn = Localization.showDebugLocText;
    _ToggleDebugStringType.isOn = RobotStateTextField.IsAnimString();

    _ActiveVariantText.text = Screen.currentResolution + "\n" + Anki.Assets.AssetBundleManager.Instance.ActiveVariantsToString();
    FillKnownDeviceDataInformation();

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DeviceDataMessage>(HandleDeviceDataMessage);
    RobotEngineManager.Instance.SendRequestDeviceData();
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.DeviceDataMessage>(HandleDeviceDataMessage);
  }

  private void HandleToggleDebugStringType(bool check) {
    RobotStateTextField.UseAnimString(check);
  }

  private void HandleToggleDebugString() {
    Canvas debugCanvas = DebugMenuManager.Instance.DebugOverlayCanvas;
    if (debugCanvas != null) {
      if (_RobotStateTextFieldInstance == null) {
        _RobotStateTextFieldInstance = UIManager.CreateUIElement(_RobotStateTextFieldPrefab, debugCanvas.transform);
      }
      else {
        Destroy(_RobotStateTextFieldInstance.gameObject);
      }
    }
  }

  private void HandleToggleShowLocDebug(bool check) {
    Localization.showDebugLocText = check;
  }

  private void HandleSetRandSeed() {
    int seed = 0;
    try {
      seed = int.Parse(_InputRandSeed.text);
    }
    catch (System.Exception) {
      return;
    }
    Random.InitState(seed);
  }

  private void HandleDeviceDataMessage(Anki.Cozmo.ExternalInterface.DeviceDataMessage message) {
    for (int i = 0; i < message.dataList.Length; ++i) {
      Anki.Cozmo.DeviceDataPair currentPair = message.dataList[i];
      switch (currentPair.dataType) {
      case Anki.Cozmo.DeviceDataType.DeviceID: {
          _DeviceID.text = currentPair.dataValue;
          break;
        }
      case Anki.Cozmo.DeviceDataType.AppRunID: {
          _AppRunID.text = currentPair.dataValue;
          break;
        }
      case Anki.Cozmo.DeviceDataType.LastAppRunID: {
          _LastAppRunID.text = currentPair.dataValue;
          break;
        }
      case Anki.Cozmo.DeviceDataType.BuildVersion: {
          _BuildVersion.text = currentPair.dataValue;
          break;
        }
      default: {
          DAS.Debug("DebugDisplayPane.HandleDeviceDataMessage.UnhandledDataType", currentPair.dataType.ToString());
          break;
        }
      }
    }
  }

  private void FillKnownDeviceDataInformation() {
    // Cozmo Android doesn't have a main activity in java now so just get this the unity way.
    // iOS gets the build info from DeviceDataMessage
#if UNITY_ANDROID  && !UNITY_EDITOR
    _BuildVersion.text = GetVersionName();
#endif
  }

#if UNITY_ANDROID && !UNITY_EDITOR
  public static string GetVersionName() {
    string dasVersionName = "unknown";
    using (AndroidJavaClass contextCls = new AndroidJavaClass("com.unity3d.player.UnityPlayer"))
    using (AndroidJavaObject context = contextCls.GetStatic<AndroidJavaObject>("currentActivity"))
    using (AndroidJavaObject packageMngr = context.Call<AndroidJavaObject>("getPackageManager")) {
      string packageName = context.Call<string>("getPackageName");
      // Get arbitrary metadata.
      //  https://developer.android.com/reference/android/content/pm/PackageManager.html#GET_META_DATA
      using (AndroidJavaObject applicationInfo = packageMngr.Call<AndroidJavaObject>("getApplicationInfo", packageName, 128)) {
        using (AndroidJavaObject bundle = applicationInfo.Get<AndroidJavaObject>("metaData")) {
          dasVersionName = bundle.Call<string>("getString", "dasVersionName");
        }
      }
    }
    return dasVersionName;
  }
#endif

}
