using UnityEngine;
using UnityEngine.UI;
using Newtonsoft.Json;

public class ScratchRequest {
  public string command { get; set; }
  public int argument { get; set; }
}

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

  [SerializeField]
  private Button _LoadWebViewButton;
  private GameObject _WebViewObject;

  private void Start() {

    _ToggleDebugStringButton.onClick.AddListener(HandleToggleDebugString);

    _ToggleDebugStringType.onValueChanged.AddListener(HandleToggleDebugStringType);

    _ToggleShowLocDebug.onValueChanged.AddListener(HandleToggleShowLocDebug);

    _ButtonSetRandSeed.onClick.AddListener(HandleSetRandSeed);

    _ToggleShowLocDebug.isOn = Localization.showDebugLocText;
    _ToggleDebugStringType.isOn = RobotStateTextField.IsAnimString();

    _ActiveVariantText.text = Screen.currentResolution + "\n" + Anki.Assets.AssetBundleManager.Instance.ActiveVariantsToString();
    FillKnownDeviceDataInformation();

    _LoadWebViewButton.onClick.AddListener(HandleLoadWebView);

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DeviceDataMessage>(HandleDeviceDataMessage);
    RobotEngineManager.Instance.SendRequestDeviceData();
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.DeviceDataMessage>(HandleDeviceDataMessage);

    if (_WebViewObject != null) {
      GameObject.Destroy(_WebViewObject);
      _WebViewObject = null;
    }
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
    UnityEngine.Random.seed = seed;
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

  private void HandleLoadWebView() {
    // Turn off music
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
    // Send EnterSDKMode to engine as we enter this view
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.EnterSDKMode(false);
    }

    if (_WebViewObject == null) {
      _WebViewObject = new GameObject("WebView", typeof(WebViewObject));
      WebViewObject webViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
      webViewObjectComponent.Init(WebViewCallback, false, @"Mozilla/5.0 (iPhone; CPU iPhone OS 7_1_2 like Mac OS X) AppleWebKit/537.51.2 (KHTML, like Gecko) Version/7.0 Mobile/11D257 Safari/9537.53", WebViewError, WebViewLoaded, true);

#if UNITY_EDITOR
      string indexFile = Application.streamingAssetsPath + "/Scratch/index.html";
#else
      string indexFile = "file://" + PlatformUtil.GetResourcesBaseFolder() + "/Scratch/index.html";
#endif

      Debug.Log("Index file = " + indexFile);
      webViewObjectComponent.LoadURL(indexFile);
    }

    /*
    // TODO Consider using when close webview
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Freeplay);
    RobotEngineManager.Instance.CurrentRobot.ExitSDKMode();
    */
  }

  private void WebViewCallback(string text) {
    string jsonStringFromJS = string.Format("{0}", text);
    Debug.Log("JSON from JavaScript: " + jsonStringFromJS);
    ScratchRequest scratchRequest = JsonConvert.DeserializeObject<ScratchRequest>(jsonStringFromJS, GlobalSerializerSettings.JsonSettings);

    if (scratchRequest.command == "cozmoDriveForward") {
      const float speed_mmps = 30.0f;
      float dist_mm = 30.0f;

      float distMultiplier = scratchRequest.argument;
      if (distMultiplier < 1.0f) {
        distMultiplier = 1.0f;
      }
      dist_mm *= distMultiplier;

      RobotEngineManager.Instance.CurrentRobot.DriveStraightAction(speed_mmps, dist_mm, false);
    }
    else {
      Debug.LogError("Scratch: no match for command");
    }

    return;
  }

  private void WebViewError(string text) {
    Debug.LogError(string.Format("CallOnError[{0}]", text));
  }

  private void WebViewLoaded(string text) {
    Debug.Log(string.Format("CallOnLoaded[{0}]", text));
    WebViewObject webViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
    webViewObjectComponent.SetVisibility(true);

#if !UNITY_ANDROID
    webViewObjectComponent.EvaluateJS(@"
              window.Unity = {
                call: function(msg) {
                  var iframe = document.createElement('IFRAME');
                  iframe.setAttribute('src', 'unity:' + msg);
                  document.documentElement.appendChild(iframe);
                  iframe.parentNode.removeChild(iframe);
                  iframe = null;
                }
              }
            ");
#endif

  }
}
