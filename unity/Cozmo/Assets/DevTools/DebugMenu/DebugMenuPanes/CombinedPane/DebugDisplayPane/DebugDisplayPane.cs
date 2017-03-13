using UnityEngine;
using UnityEngine.UI;
using Newtonsoft.Json;
using Anki.Cozmo.ExternalInterface;
using System.Collections.Generic;

public class ScratchRequest {
  public int requestId { get; set; }
  public string command { get; set; }
  public string argString { get; set; }
  public int argInt { get; set; }
  public uint argUInt { get; set; }
  public float argFloat { get; set; }
}

public class InProgressScratchBlock {
  private int _RequestId;
  private WebViewObject _WebViewObjectComponent;

  public void Init(int requestId = -1, WebViewObject webViewObjectComponent = null) {
    _RequestId = requestId;
    _WebViewObjectComponent = webViewObjectComponent;
  }

  public void AdvanceToNextBlock(bool success) {
    // Calls the JavaScript function resolving the Promise on the block
    _WebViewObjectComponent.EvaluateJS(@"window.resolveCommands[" + this._RequestId + "]();");
  }
}

public static class InProgressScratchBlockPool {
  private static List<InProgressScratchBlock> _available = new List<InProgressScratchBlock>();
  private static List<InProgressScratchBlock> _inUse = new List<InProgressScratchBlock>();

  public static InProgressScratchBlock GetInProgressScratchBlock() {
    // TODO Verify the lock is required and if we can assert on threadId here
    // to make sure we're not opening ourselves to threading issues.
    lock (_available) {
      if (_available.Count != 0) {
        InProgressScratchBlock po = _available[0];
        _inUse.Add(po);
        _available.RemoveAt(0);

        return po;
      }
      else {
        InProgressScratchBlock po = new InProgressScratchBlock();
        po.Init();
        _inUse.Add(po);

        return po;
      }
    }
  }

  public static void ReleaseInProgressScratchBlock(InProgressScratchBlock po) {
    po.Init();

    lock (_available) {
      _available.Add(po);
      _inUse.Remove(po);
    }
  }
}

public class DebugDisplayPane : MonoBehaviour {
  private const float kSlowDriveSpeed_mmps = 30.0f;
  private const float kMediumDriveSpeed_mmps = 45.0f;
  private const float kFastDriveSpeed_mmps = 60.0f;
  private const float kDriveDist_mm = 44.0f; // length of one light cube
  private const float kDegreesToRadians = Mathf.PI / 180.0f;
  private const float kTurnAngle = 90.0f * kDegreesToRadians;

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
  private float _TimeLastReportedObservedFaceToScratch;
  private float _TimeLastReportedObservedCubeToScratch;
  private float _TimeLastObservedCube;
  private int _LastObservedObjectID;

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

    // TODO Check that this initialization doesn't cause unwanted behavior if have to turn on/off vision system for Scratch.
    _TimeLastReportedObservedFaceToScratch = float.MinValue;
    _TimeLastReportedObservedCubeToScratch = float.MinValue;
    _TimeLastObservedCube = float.MinValue;
    _LastObservedObjectID = -1;

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.DeviceDataMessage>(HandleDeviceDataMessage);
    RobotEngineManager.Instance.SendRequestDeviceData();
  }

  private void OnDestroy() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.DeviceDataMessage>(HandleDeviceDataMessage);

    if (_WebViewObject != null) {
      RobotEngineManager.Instance.RemoveCallback<RobotObservedFace>(HandleRobotObservedFace);
      RobotEngineManager.Instance.RemoveCallback<RobotObservedObject>(HandleRobotObservedObject);
      LightCube.TappedAction -= HandleTap;

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

      RobotEngineManager.Instance.AddCallback<RobotObservedFace>(HandleRobotObservedFace);
      RobotEngineManager.Instance.AddCallback<RobotObservedObject>(HandleRobotObservedObject);
      LightCube.TappedAction += HandleTap;

      Debug.Log("Index file = " + indexFile);
      webViewObjectComponent.LoadURL(indexFile);
    }

    /*
    // TODO Consider using when close webview
    Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Freeplay);
    RobotEngineManager.Instance.CurrentRobot.ExitSDKMode();
    */
  }

  private void HandleRobotObservedFace(RobotObservedFace message) {
    //Debug.Log("HandleRobotObservedFace");

    // If Cozmo sees a face (and hasn't seen a face for 2.0 seconds), resume the face block.
    if (Time.time - _TimeLastReportedObservedFaceToScratch > 2.0f) {
      WebViewObject webViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
      webViewObjectComponent.EvaluateJS(@"window.vm.runtime.onCozmoSawFace();");
      _TimeLastReportedObservedFaceToScratch = Time.time;
    }
  }

  private void HandleRobotObservedObject(RobotObservedObject message) {
    //Debug.Log("HandleRobotObservedObject objectID = " + message.objectID);
    _LastObservedObjectID = message.objectID;
    _TimeLastObservedCube = Time.time;

    // If Cozmo sees a cube (and hasn't seen a cube for 2.0 seconds), resume the cube block.
    if (Time.time - _TimeLastReportedObservedCubeToScratch > 2.0f) {
      WebViewObject webViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
      webViewObjectComponent.EvaluateJS(@"window.vm.runtime.onCozmoSawCube();");
      _TimeLastReportedObservedCubeToScratch = Time.time;
    }
  }

  private void HandleTap(int id, int tappedTimes, float timeStamp) {
    //Debug.Log("HandleTap");
    WebViewObject webViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
    webViewObjectComponent.EvaluateJS(@"window.vm.runtime.onCozmoCubeWasTapped();");
  }

  private void HandleAlignWithObjectResponse(bool success) {
    // TODO Try to use Promise with DockWithCube
    if (!success) {
      PlayAngryAnimation();
    }
  }

  private void DockWithCube(bool previousActionSucceeded) {
    bool success = false;

    if (previousActionSucceeded) {
      // Dock with a cube we observed within the last 0.2 seconds, if any.
      LightCube cube = null;
      RobotEngineManager.Instance.CurrentRobot.LightCubes.TryGetValue(_LastObservedObjectID, out cube);

      if ((Time.time - _TimeLastObservedCube) < 0.2f && cube != null) {
        RobotEngineManager.Instance.CurrentRobot.AlignWithObject(cube, 0.0f, callback: HandleAlignWithObjectResponse, alignmentType: Anki.Cozmo.AlignmentType.LIFT_PLATE);
        success = true;
      }
    }

    if (!success) {
      // Play angry animation since Cozmo wasn't able to complete the task
      PlayAngryAnimation();
    }
  }

  private void PlayAngryAnimation() {
    RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.FrustratedByFailureMajor);
  }

  private void WebViewCallback(string text) {
    string jsonStringFromJS = string.Format("{0}", text);
    Debug.Log("JSON from JavaScript: " + jsonStringFromJS);
    ScratchRequest scratchRequest = JsonConvert.DeserializeObject<ScratchRequest>(jsonStringFromJS, GlobalSerializerSettings.JsonSettings);

    InProgressScratchBlock inProgressScratchBlock = InProgressScratchBlockPool.GetInProgressScratchBlock();
    inProgressScratchBlock.Init(scratchRequest.requestId, _WebViewObject.GetComponent<WebViewObject>());

    if (scratchRequest.command == "cozmoDriveForward") {
      // Here, argFloat represents the number selected from the dropdown under the "drive forward" block
      float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
      RobotEngineManager.Instance.CurrentRobot.DriveStraightAction(getDriveSpeed(scratchRequest), dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
    }
    else if (scratchRequest.command == "cozmoDriveBackward") {
      // Here, argFloat represents the number selected from the dropdown under the "drive backward" block
      float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
      RobotEngineManager.Instance.CurrentRobot.DriveStraightAction(-getDriveSpeed(scratchRequest), -dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
    }
    else if (scratchRequest.command == "cozmoPlayAnimation") {
      Anki.Cozmo.AnimationTrigger animationTrigger = GetAnimationTriggerForScratchName(scratchRequest.argString);
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(animationTrigger, inProgressScratchBlock.AdvanceToNextBlock);
    }
    else if (scratchRequest.command == "cozmoTurnLeft") {
      // Turn 90 degrees to the left
      RobotEngineManager.Instance.CurrentRobot.TurnInPlace(kTurnAngle, 0.0f, 0.0f, inProgressScratchBlock.AdvanceToNextBlock);
    }
    else if (scratchRequest.command == "cozmoTurnRight") {
      // Turn 90 degrees to the right
      RobotEngineManager.Instance.CurrentRobot.TurnInPlace(-kTurnAngle, 0.0f, 0.0f, inProgressScratchBlock.AdvanceToNextBlock);
    }
    else if (scratchRequest.command == "cozmoSays") {
      // TODO Add profanity filter
      RobotEngineManager.Instance.CurrentRobot.SayTextWithEvent(scratchRequest.argString, Anki.Cozmo.AnimationTrigger.Count, callback: inProgressScratchBlock.AdvanceToNextBlock);
    }
    else if (scratchRequest.command == "cozmoHeadAngle") {
      float desiredHeadAngle = (CozmoUtil.kIdealBlockViewHeadValue + CozmoUtil.kIdealFaceViewHeadValue) * 0.5f; // medium setting
      if (scratchRequest.argString == "low") {
        desiredHeadAngle = CozmoUtil.kIdealBlockViewHeadValue;
      }
      else if (scratchRequest.argString == "high") {
        desiredHeadAngle = CozmoUtil.kIdealFaceViewHeadValue;
      }

      if (System.Math.Abs(desiredHeadAngle - RobotEngineManager.Instance.CurrentRobot.HeadAngle) > float.Epsilon) {
        RobotEngineManager.Instance.CurrentRobot.SetHeadAngle(desiredHeadAngle, inProgressScratchBlock.AdvanceToNextBlock);
      }
    }
    else if (scratchRequest.command == "cozmoDockWithCube") {
      RobotEngineManager.Instance.CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue, callback: DockWithCube);
    }
    else if (scratchRequest.command == "cozmoForklift") {
      float liftHeight = 0.5f; // medium setting
      if (scratchRequest.argString == "low") {
        liftHeight = 0.0f;
      }
      else if (scratchRequest.argString == "high") {
        liftHeight = 1.0f;
      }

      RobotEngineManager.Instance.CurrentRobot.SetLiftHeight(liftHeight, inProgressScratchBlock.AdvanceToNextBlock);
    }
    else if (scratchRequest.command == "cozmoSetBackpackColor") {
      RobotEngineManager.Instance.CurrentRobot.SetAllBackpackBarLED(scratchRequest.argUInt);
      inProgressScratchBlock.AdvanceToNextBlock(true);
    }
    else {
      Debug.LogError("Scratch: no match for command");
    }

    return;
  }

  // Check if cozmoDriveFaster JavaScript bool arg is set by checking ScratchRequest.argBool and set speed appropriately.
  private float getDriveSpeed(ScratchRequest scratchRequest) {
    float driveSpeed_mmps = kSlowDriveSpeed_mmps;
    if (scratchRequest.argString == "medium") {
      driveSpeed_mmps = kMediumDriveSpeed_mmps;
    }
    else if (scratchRequest.argString == "fast") {
      driveSpeed_mmps = kFastDriveSpeed_mmps;
    }

    return driveSpeed_mmps;
  }

  private Anki.Cozmo.AnimationTrigger GetAnimationTriggerForScratchName(string scratchAnimationName) {
    if (scratchAnimationName == "happy") {
      return Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentCelebration;
    }
    else if (scratchAnimationName == "victory") {
      return Anki.Cozmo.AnimationTrigger.BuildPyramidSuccess;
    }
    else if (scratchAnimationName == "unhappy") {
      return Anki.Cozmo.AnimationTrigger.FrustratedByFailureMajor;
    }
    else if (scratchAnimationName == "surprise") {
      return Anki.Cozmo.AnimationTrigger.DroneModeTurboDrivingStart;
    }
    else if (scratchAnimationName == "dog") {
      return Anki.Cozmo.AnimationTrigger.PetDetectionDog;
    }
    else if (scratchAnimationName == "cat") {
      return Anki.Cozmo.AnimationTrigger.PetDetectionCat;
    }
    else if (scratchAnimationName == "sneeze") {
      return Anki.Cozmo.AnimationTrigger.PetDetectionSneeze;
    }
    else if (scratchAnimationName == "excited") {
      return Anki.Cozmo.AnimationTrigger.SuccessfulWheelie;
    }
    else if (scratchAnimationName == "thinking") {
      return Anki.Cozmo.AnimationTrigger.HikingReactToPossibleMarker;
    }
    else if (scratchAnimationName == "bored") {
      return Anki.Cozmo.AnimationTrigger.NothingToDoBoredEvent;
    }
    else if (scratchAnimationName == "frustrated") {
      return Anki.Cozmo.AnimationTrigger.AskToBeRightedRight;
    }
    else if (scratchAnimationName == "chatty") {
      return Anki.Cozmo.AnimationTrigger.BuildPyramidReactToBase;
    }
    else if (scratchAnimationName == "dejected") {
      return Anki.Cozmo.AnimationTrigger.FistBumpLeftHanging;
    }
    else if (scratchAnimationName == "sleep") {
      return Anki.Cozmo.AnimationTrigger.Sleeping;
    }

    // TODO Error: Shouldn't get here.
    DAS.Error("Scratch.BadTriggerName", "Unexpected name '" + scratchAnimationName + "'");
    return Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentCelebration;
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
