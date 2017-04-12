using System;
using System.Linq;
using UnityEngine;

public class AndroidConnectionFlowStage : JavaMessageReceiver.JavaBehaviour {
  public Action OnStageComplete;

  public void Destroy() {
    // unregister java message listeners immediately
    ClearJavaListeners();
    GameObject.Destroy(this.gameObject);
  }
}

/*
 * Parent of all Android connection flow stages. Each stage is spawned, in order,
 * by this master flow controller.
 * 
 * Stages can be skipped if prerequisite conditions are met indicating that a
 * given stage doesn't need to be run. For example, there's no need to visit
 * the "select a Cozmo" stage if we only scan one Cozmo.
 * 
 * Stages are...
 * - Permissions (AndroidGetPermissions): check that the phone is in a stage where
 *   we can receive valid wifi results and request any permissions (i.e. location,
 *   on android 5+) or user actions (i.e. turn wifi on, enable location services on
 *   android 5+) necessary for us to get wifi scan results. Skipped if we determine
 *   that we're already receiving wifi scan results.
 * - FindAny (AndroidFindNetworks): instruct the user to turn on their Cozmo and wait
 *   until we successfully scan one. Skipped if we already see a Cozmo in wifi scan
 *   results.
 * - Select (AndroidSelectNetwork): ask the user which Cozmo they'd like to connect
 *   to. Skipped if we only detect one Cozmo in wifi scan results.
 * - Password (AndroidEnterPassword): ask the user to enter the password for the
 *   Cozmo they're attempting to connect to. Skipped if we detect that the OS
 *   already has a wifi configuration for this Cozmo.
 * - Connecting (AndroidConnectToNetwork): shows connection status as we wait for
 *   the OS to connect to the given network. Never skipped.
 * 
 * In a couple instances, we can go back to previous stages:
 * - If we get to the Password screen but the right Cozmo isn't selected, the user
 *   can return to the Select screen to pick a different Cozmo
 * - If Java tells us that a connection failed on the Connecting screen because the
 *   password was incorrect, we can kick the user back to the password screen to
 *   enter it again
 * 
 * Together, these stages attempt to handle all edge cases related to connection
 * while still providing a smooth and effortless connection experience when no
 * issues arise. Once a connection is established, the typical flow through these
 * screens should be...
 * - Permissions is skipped because we have the permissions we need
 * - FindAny is skipped because (hopefully) Cozmo is already turned on and we've
 *   scanned it
 * - Password is skipped because we've already connected successfully in the past
 * - Connecting is the only screen the user sees, and it progresses as soon as a
 *   successful connection is detected
 */
public class AndroidConnectionFlow : JavaMessageReceiver.JavaBehaviour {

  private const int kPingTimeoutMs = 1000;
  private const int kPingRetryDelayMs = 500;

  public static AndroidConnectionFlow Instance;

  [SerializeField]
  private AndroidConnectionFlowStage _PermissionsPrefab;

  [SerializeField]
  private AndroidFindNetworks _ScanNetworksPrefab;

  [SerializeField]
  private AndroidSelectNetwork _SelectSSIDPrefab;

  [SerializeField]
  private AndroidEnterPassword _EnterPasswordPrefab;

  [SerializeField]
  private AndroidConnectToNetwork _ConnectingPrefab;

  [SerializeField]
  private JavaMessageReceiver _MessageReceiver;

  [NonSerialized]
  public Action<bool> OnScreenComplete;
  [NonSerialized]
  public Action OnRestartFlow;
  [NonSerialized]
  public Action OnCancelFlow;
  [NonSerialized]
  public Action OnStartConnect;

  // how long to wait for connection attempt to time out
  public const int kTimeoutMs = 10000;

  private enum Stage {
    Init,
    Permissions,
    FindAny,
    Select,
    Password
  }

  // stage evaluation functions
  private delegate bool StageSkipHandler();
  private StageSkipHandler[] _StageSkipHandlers;

  private AndroidConnectionFlowStage[] _StagePrefabs;
  private AndroidConnectionFlowStage _StageInstance;

  private Stage _Stage;

  private int _PermissionIssueCount;
  private bool _ForceSelectScreen;
  private bool _Disabled = false;

  [NonSerialized]
  public string[] CozmoSSIDs;
  [NonSerialized]
  public string SelectedSSID;
  [NonSerialized]
  public string Password;

  private void Start() {
    if (Instance != null) {
      DAS.Error("AndroidConnectionFlow.Start", "Instance already exists!");
    }
    Instance = this;

    // Collection of callbacks to determine if any individual
    // stage should be skipped
    _StageSkipHandlers = new StageSkipHandler[] {
      null,
      SkipPermissions,
      SkipFindAny,
      SkipSelectNetwork,
      SkipPassword
    };

    _StagePrefabs = new AndroidConnectionFlowStage[] {
      null,
      _PermissionsPrefab,
      _ScanNetworksPrefab,
      _SelectSSIDPrefab,
      _EnterPasswordPrefab
    };

    _Stage = Stage.Init;
    SelectNextStage();

    // An attempt to ping Cozmo will run in the background throughout the duration
    // of this flow; if at any point we detect we can successfully ping a Cozmo, we
    // kick the user out of this platform-specific flow and continue with normal
    // connection flow. This ping test is the successful exit point of this flow;
    // the only other way out is if the user hits a button to leave and use the old
    // flow, or if we detect failure on the Connecting screen.
    StartPingTest();
    StartCoroutine("CheckConnectivity");

    RegisterJavaListener(_MessageReceiver, "scanPermissionsIssue", p => NeedPermissions());
  }

  public void Disable() {
    if (_Disabled) {
      return;
    }
    StopPingTest();
    StopCoroutine("CheckConnectivity");
    if (_StageInstance != null) {
      DestroyStage();
    }
    if (Instance == this) {
      Instance = null;
    }
    _Disabled = true;
  }

  protected override void OnDestroy() {
    Disable();
    base.OnDestroy();
  }

  public static void StartPingTest() {
    CallJava("beginPingTest", RobotEngineManager.kRobotIP, kPingTimeoutMs, kPingRetryDelayMs);
  }

  public static void StopPingTest() {
    CallJava("endPingTest");
  }

  // test if we're already connected and shut down the ping test if so
  // return if connected or not
  public static bool HandleAlreadyOnCozmoWifi() {
    if (CallJava<bool>("isPingSuccessful")) {
      StopPingTest();
      return true;
    } else {
      return false;
    }
  }

  private void DestroyStage() {
    DAS.Event("android.stage.end", _Stage.ToString());
    _StageInstance.Destroy();
    _StageInstance = null;
  }

  private void SelectNextStage() {
    bool isSkipping = true;
    int numStages = Enum.GetNames(typeof(Stage)).Length;

    // destroy existing stage instance before creating new one
    if (_StageInstance != null) {
      DestroyStage();
    }

    // increment stage until we arrive at the one we want to use next
    while (isSkipping && (int)_Stage < numStages) {
      _Stage++;
      if ((int)_Stage >= numStages) {
        return;
      } else {
        var skipHandler = _StageSkipHandlers[(int)_Stage];
        if (!skipHandler()) {
          DAS.Info("AndroidConnectionFlow.NextStage", "selected " + _Stage.ToString());
          isSkipping = false;
        } else {
          DAS.Info("AndroidConnectionFlow.NextStage", "skipping " + _Stage.ToString());
        }
      }
    }

    // instantiate new stage prefab
    var stagePrefab = _StagePrefabs[(int)_Stage];
    if (stagePrefab != null) {
      DAS.Event("android.stage.begin", _Stage.ToString());
      _StageInstance = UIManager.CreateUIElement(stagePrefab).GetComponent<AndroidConnectionFlowStage>();
      _StageInstance.OnStageComplete += HandleStageComplete;
    }
  }

  private void ActivatePriorStage(Stage stage) {
    _Stage = (Stage)(((int)stage) - 1);
    SelectNextStage();
  }

  private System.Collections.IEnumerator CheckConnectivity() {
    while (true) {
      if (CallJava<bool>("isPingSuccessful")) {
        DAS.Info("AndroidConnectionFlow.CheckConnectivity", "ping succeeded, exiting");
        OnScreenComplete(true);
        yield break;
      }
      yield return new WaitForSeconds(0.5f);
    }
  }

  private bool SkipPermissions() {
    // if we haven't yet determined that we need permission, skip
    bool permissionIssue = AndroidConnectionFlow.CallJava<bool>("isPermissionIssue");
    bool wifiEnabled = AndroidConnectionFlow.CallJava<bool>("isWifiEnabled");

    bool noProblemsDetected = wifiEnabled && !permissionIssue;
    bool notDirectedHere = _PermissionIssueCount == 0;
    return noProblemsDetected && notDirectedHere;
  }

  private bool SkipFindAny() {
    var SSIDs = GetCozmoSSIDs();
    if (SSIDs.Length > 0) {
      CozmoSSIDs = SSIDs;
      return true;
    } else {
      return false;
    }
  }

  private bool SkipSelectNetwork() {
    if (CozmoSSIDs.Length == 1 && !_ForceSelectScreen) {
      SelectedSSID = CozmoSSIDs[0];
      return true;
    } else {
      return false;
    }
  }

  private bool SkipPassword() {
    bool shouldSkip = CallJava<bool>("isExistingConfigurationForNetwork", SelectedSSID);
    if (shouldSkip) {
      // initiate connection
      bool result = ConnectExisting(SelectedSSID);
      DAS.Info("AndroidConnectFlow.SkipPassword", "connect result to existing network " + SelectedSSID + " is " + result);
    }
    return shouldSkip;
  }

  private bool SkipConnecting() {
    return false;
  }

  private void HandleStageComplete() {
    SelectNextStage();
  }

  public void AddConnectingPrefab(GameObject parentObject) {
    var connectingOverlay = GameObject.Instantiate(_ConnectingPrefab);
    connectingOverlay.gameObject.transform.SetParent(parentObject.transform, false);
  }

  public static bool IsAvailable() {
    return AnkiPrefs.GetInt(AnkiPrefs.Prefs.AndroidAutoConnectDisabled) == 0;
  }

  #region Shared utility functions for flow/stages

  public static ReturnType CallJava<ReturnType>(string method, params object[] parameters) {
    #if UNITY_ANDROID && !UNITY_EDITOR
    return CozmoBinding.GetWifiUtilClass().CallStatic<ReturnType>(method, parameters);
    #else
    return default(ReturnType);
    #endif
  }
  public static ReturnType CallLocationJava<ReturnType>(string method, params object[] parameters) {
    #if UNITY_ANDROID && !UNITY_EDITOR
    return CozmoBinding.GetLocationUtilClass().CallStatic<ReturnType>(method, parameters);
    #else
    return default(ReturnType);
    #endif
  }
  public static void CallJava(string method, params object[] parameters) {
    #if UNITY_ANDROID && !UNITY_EDITOR
    CozmoBinding.GetWifiUtilClass().CallStatic(method, parameters);
    #endif
  }
  public static void CallLocationJava(string method, params object[] parameters) {
    #if UNITY_ANDROID && !UNITY_EDITOR
    CozmoBinding.GetLocationUtilClass().CallStatic(method, parameters);
    #endif
  }

  public static string[] GetCozmoSSIDs() {
    return GetCozmoSSIDs(CallJava<string[]>("getSSIDs"));
  }
  public static string[] GetCozmoSSIDs(string[] allSSIDs) {
    return allSSIDs.Where(s => s.StartsWith("Cozmo_")).ToArray();
  }

  public bool Connect(string SSID, string password) {
    bool result = CallJava<bool>("connect", SSID, password, kTimeoutMs);
    OnStartConnect();
    return result;
  }

  public bool ConnectExisting(string SSID) {
    bool result = CallJava<bool>("connectExisting", SelectedSSID, kTimeoutMs);
    OnStartConnect();
    return result;
  }

  public void RestartFlow() {
    OnRestartFlow();
  }

  public void UseOldFlow() {
    OnCancelFlow();
  }

  public void NeedPermissions() {
    if (_Stage <= Stage.Permissions) {
      return;
    }

    if (_PermissionIssueCount > 0) {
      OnCancelFlow();
    }
    else {
      _PermissionIssueCount++;
      ActivatePriorStage(Stage.Permissions);
    }
  }

  public void WrongCozmoSelected() {
    // force selection of the SelectNetwork screen so they can tell us what their cozmo is
    _ForceSelectScreen = true;
    Password = null;
    SelectedSSID = null;
    ActivatePriorStage(Stage.Select);
  }

  public void HandleWrongPassword() {
    // reset password and reset stage to right before password stage
    Password = default(string);
    ActivatePriorStage(Stage.Password);
    if (_StageInstance is AndroidEnterPassword) {
      ((AndroidEnterPassword)_StageInstance).WrongPassword = true;
    }
  }

  public JavaMessageReceiver GetMessageReceiver() {
    return _MessageReceiver;
  }

  public void ConnectionFailed() {
    OnScreenComplete(false);
  }

  #endregion

}
