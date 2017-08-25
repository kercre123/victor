using UnityEngine;
using Anki.Cozmo;
using Anki.Cozmo.Audio;
using Anki.Cozmo.ExternalInterface;
using Newtonsoft.Json;
using DataPersistence;
using System;
using System.Collections.Generic;
using System.IO;
using Cozmo.UI;
using Cozmo.MinigameWidgets;

namespace CodeLab {
  public class CodeLabSampleProject {
    public uint DisplayOrder;
    public Guid ProjectUUID;
    public uint VersionNum;
    public bool IsVertical;
    public string ProjectIconName;
    public string ProjectXML;
    public string ProjectName;
  }

  public class CodeLabGame : GameBase {

    // When the webview is opened to display the Code Lab workspace,
    // we can display one of the following projects.
    private enum RequestToOpenProjectOnWorkspace {
      DisplayNoProject,     // Unset value
      CreateNewProject,     // Workspace displays only green flag
      DisplayUserProject,   // Display a previously-saved user project. User project UUID saved in _ProjectUUIDToOpen.
      DisplaySampleProject  // Display a previously-saved sample project. Sample project UUID saved in _ProjectUUIDToOpen.
    }

    private enum MusicRoundStates {
      WorkspaceMusicRound = 1,
      LobbyMusicRound = 2
    }

    // Look at CodeLabGameUnlockable.asset in editor to set when unlocked.
    private GameObject _WebViewObject;
    private WebViewObject _WebViewObjectComponent;

    [SerializeField]
    private GameObject _BlackScreenPrefab;

    private RequestToOpenProjectOnWorkspace _RequestToOpenProjectOnWorkspace;
    private string _ProjectUUIDToOpen;
    private List<CodeLabSampleProject> _CodeLabSampleProjects;

    // SessionState tracks the entirety of the current CodeLab Game session, including any currently running program.
    private SessionState _SessionState = new SessionState();

    // Any requests from Scratch that occur whilst Cozmo is resetting to home pose are queued
    private Queue<ScratchRequest> _queuedScratchRequests = new Queue<ScratchRequest>();

    private CodeLabCozmoFaceDisplay _CozmoFaceDisplay = new CodeLabCozmoFaceDisplay();

    private int _PendingResetToHomeActions = 0;
    private bool _HasQueuedResetToHomePose = false;
    private bool _RequiresResetToNeutralFace = false;
    private bool _IsDrivingOffCharger = false;

    private const string kCodeLabGameDrivingAnimLock = "code_lab_game";

    private CubeColors[] _CubeLightColors = { new CubeColors(),
                                     new CubeColors(),
                                     new CubeColors() };

    private class CubeColors {
      public Color[] Colors { get; set; }

      public CubeColors() {
        Colors = new Color[4];
        SetAll(Color.black);
      }

      public void SetAll(Color color) {
        for (int i = 0; i < 4; i++) {
          Colors[i] = color;
        }
      }

      public void SetColor(Color color, int lightIndex) {
        Colors[lightIndex] = color;
      }
    }

    // GameToGameConn is used by dev-builds only (no GameToGame or SDK connection possible in CodeLab in Shipping builds)
    private class GameToGameConn {
      private bool _Enabled = false;
      private string _PendingMessageType = null;
      private string _PendingMessage = null;
      private int _PendingMessageNextIndex = -1;
      private int _PendingMessageCount = -1;

      public void Enable() {
        _Enabled = true;
      }

      public bool IsEnabled {
        get { return _Enabled; }
      }

      public string PendingMessageType {
        get { return _PendingMessageType; }
      }

      public string PendingMessage {
        get { return _PendingMessage; }
      }

      public bool HasPendingMessage() {
        return _PendingMessageNextIndex > 0;
      }

      public void ResetPendingMessage() {
        _PendingMessageType = null;
        _PendingMessage = null;
        _PendingMessageNextIndex = -1;
        _PendingMessageCount = -1;
      }

      private bool IsExpectedPart(GameToGame msg) {
        if (HasPendingMessage()) {
          return ((_PendingMessageNextIndex == msg.messagePartIndex) && (_PendingMessageCount == msg.messagePartCount) && (_PendingMessageType == msg.messageType));
        }

        return (msg.messagePartIndex == 0);
      }

      public bool AppendMessage(GameToGame msg) {
        if (!IsExpectedPart(msg)) {
          DAS.Error("AppendMessage.UnexpectedPart", string.Format("Expected {0}/{1} '{2}', got {3}/{4} '{5}'",
                                                                  _PendingMessageNextIndex, _PendingMessageCount, _PendingMessageType,
                                                                  msg.messagePartIndex, msg.messagePartCount, msg.messageType));
          ResetPendingMessage();
          return false;
        }
        else {
          if (HasPendingMessage()) {
            _PendingMessage += msg.payload;
          }
          else {
            _PendingMessageType = msg.messageType;
            _PendingMessage = msg.payload;
            _PendingMessageCount = msg.messagePartCount;
          }
          _PendingMessageNextIndex = msg.messagePartIndex + 1;
          return (_PendingMessageNextIndex == _PendingMessageCount);
        }
      }
    }

    private GameToGameConn _GameToGameConn = new GameToGameConn(); // Dev-Connection for non-SHIPPING builds
    private string _CurrentURLFilename = null; // The currently displayed URL
    private string _DevLoadPath = null; // Custom path for loading assets in dev builds
    private bool _DevLoadPathFirstRequest = true;

    private uint _ChallengeBookmark = 1;

    private const float kNormalDriveSpeed_mmps = 70.0f;
    private const float kFastDriveSpeed_mmps = 200.0f;
    private const float kDriveDist_mm = 44.0f; // length of one light cube
    private const float kTurnAngle = 90.0f * Mathf.Deg2Rad;
    private const float kToleranceAngle = 10.0f * Mathf.Deg2Rad;
    private const float kLiftSpeed_rps = 2.0f;
    private const float kTimeoutForResetToHomePose_s = 3.0f;

    private const string kHorizontalIndexFilename = "index.html";
    private const string kVerticalIndexFilename = "index_vertical.html";

    protected override void InitializeGame(ChallengeConfigBase challengeConfigData) {
      SetRequestToOpenProject(RequestToOpenProjectOnWorkspace.DisplayNoProject, null);

      DAS.Debug("Loading Webview", "");
      UIManager.Instance.ShowTouchCatcher();

      // Since webview takes awhile to load, keep showing the "cozmo is getting ready"
      // load screen instead of the white background that usually shows in front ( shown in gamebase before calling this function )
      SharedMinigameView.HideMiddleBackground();
      SharedMinigameView.HideQuitButton();

      // In GameBase.cs, in the function InitializeChallenge(), the lights are set to allow the Challenge to control them.
      // We don't want this behavior for CodeLab.
      CurrentRobot.SetEnableFreeplayLightStates(true);

      // Load sample projects json from file and cache in list
      string pathToFile = "/Scratch/sample-projects.json";
#if UNITY_EDITOR || UNITY_IOS
      string path = Application.streamingAssetsPath + pathToFile;
#elif UNITY_ANDROID
string path = PlatformUtil.GetResourcesBaseFolder() + pathToFile;
#endif

      string json = File.ReadAllText(path);
      _CodeLabSampleProjects = JsonConvert.DeserializeObject<List<CodeLabSampleProject>>(json);

      RobotEngineManager.Instance.AddCallback<GameToGame>(HandleGameToGame);
      if (_SessionState.GetGrammarMode() == GrammarMode.Vertical) {
        RobotEngineManager.Instance.CurrentRobot.EnableCubeSleep(true, true);
      }

      LoadWebView();
    }

    private void HandleGameToGameContents(string messageType, string payload) {
      // Used by dev-builds only (no GameToGame or SDK connection possible in CodeLab in Shipping builds)
      // This is used by the codelab.py script to support various debug/dev features including:
      // 1: hot-reloading of assets
      // 2: running Scratch in a remote server on a host computer in e.g. Chrome
      switch (messageType) {
      case "WebViewCallback":
        WebViewCallback(payload);
        break;
      case "UseHorizontalGrammar":
        _SessionState.SetGrammarMode(GrammarMode.Horizontal);
        if (_CurrentURLFilename == kVerticalIndexFilename) {
          LoadURL(kHorizontalIndexFilename);
        }
        break;
      case "UseVerticalGrammar":
        _SessionState.SetGrammarMode(GrammarMode.Vertical);
        if (_CurrentURLFilename == kHorizontalIndexFilename) {
          LoadURL(kVerticalIndexFilename);
        }
        break;
      case "LoadURL":
        LoadURL(payload);
        break;
      case "ReloadURL":
        // Re-opens the current page (but potentially from a different file location - cache vs assets etc.)
        if (_CurrentURLFilename != null) {
          LoadURL(_CurrentURLFilename);
        }
        break;
      case "ReInit":
        if (_WebViewObject != null) {
          GameObject.Destroy(_WebViewObject);
          _WebViewObject = new GameObject("WebView", typeof(WebViewObject));
          _WebViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
          _WebViewObjectComponent.Init(WebViewCallback, false, err: WebViewError, ld: WebViewLoaded, enableWKWebView: true);
          if (_CurrentURLFilename != null) {
            LoadURL(_CurrentURLFilename);
          }
        }
        break;
      case "SetAppPath":
        _DevLoadPath = null;
        DAS.Info("CodeLab.SetAppPath", "_DevLoadPath = null");
        break;
      case "SetCachePath":
        _DevLoadPath = Application.temporaryCachePath + payload;
        DAS.Info("CodeLab.SetCachePath", "_DevLoadPath = '" + _DevLoadPath + "'");
        break;
      default:
        DAS.Error("HandleGameToGameContents.BadType", "Unhandled type: " + messageType);
        break;
      }
    }

    private void HandleGameToGame(GameToGame webToUnity) {
      // Used by dev-builds only (no GameToGame or SDK connection possible in CodeLab in Shipping builds)

      if (webToUnity.messageType == "EnableGameToGame") {
        _GameToGameConn.Enable();
      }
      else if (_GameToGameConn.IsEnabled) {
        if (_GameToGameConn.AppendMessage(webToUnity)) {
          try {
            HandleGameToGameContents(_GameToGameConn.PendingMessageType, _GameToGameConn.PendingMessage);
          }
          finally {
            _GameToGameConn.ResetPendingMessage();
          }
        }
      }
    }

    protected override void CleanUpOnDestroy() {
      _WebViewObjectComponent = null;

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.CancelAction(RobotActionType.UNKNOWN);  // Cancel all current actions
        robot.RemoveDrivingAnimations(kCodeLabGameDrivingAnimLock);
        _SessionState.EndSession();
        robot.ExitSDKMode(false);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.EstimatingFacialExpression, false);
      }

      InProgressScratchBlockPool.ReleaseAllInUse();
      StopVerticalHatBlockListeners();

      if (_WebViewObject != null) {
        GameObject.Destroy(_WebViewObject);
        _WebViewObject = null;
      }
      UIManager.Instance.HideTouchCatcher();

      RobotEngineManager.Instance.RemoveCallback<GameToGame>(HandleGameToGame);
    }

    private CozmoStateForCodeLab _LatestCozmoState = new CozmoStateForCodeLab(); // The latest world state sent to JS

    private void SetCubeStateForCodeLab(CubeStateForCodeLab cubeDest, LightCube cubeSrc) {
      if (cubeSrc == null) {
        cubeDest.pos = new Vector3(0.0f, 0.0f, 0.0f);
        cubeDest.camPos = new Vector2(0.0f, 0.0f);
        cubeDest.isValid = false;
        cubeDest.isVisible = false;
        cubeDest.pitch_d = 0.0f;
        cubeDest.roll_d = 0.0f;
        cubeDest.yaw_d = 0.0f;
      }
      else {
        cubeDest.pos = cubeSrc.WorldPosition;
        cubeDest.camPos = cubeSrc.VizRect.center;
        cubeDest.isValid = true;
        const int kMaxVisionFramesSinceSeeingCube = 30;
        cubeDest.isVisible = (cubeSrc.NumVisionFramesSinceLastSeen < kMaxVisionFramesSinceSeeingCube);
        cubeDest.pitch_d = cubeSrc.PitchDegrees;
        cubeDest.roll_d = cubeSrc.RollDegrees;
        cubeDest.yaw_d = cubeSrc.YawDegrees;
      }
    }

    public void EvaluateJS(string text) {
      if (_WebViewObjectComponent != null) {
        _WebViewObjectComponent.EvaluateJS(text);
      }
      if ((_GameToGameConn != null) && _GameToGameConn.IsEnabled) {
        const string messageType = "EvaluateJS";

        int maxPayloadSize = 2024 - messageType.Length; // 2048 is an engine-side limit for clad messages, reserve an extra 24 for rest of the tags and members

        byte partCount = 1;
        if (text.Length > maxPayloadSize) {
          partCount = (byte)(((text.Length - 1) / maxPayloadSize) + 1);
        }

        var rEM = RobotEngineManager.Instance;
        for (byte i = 0; i < partCount; ++i) {
          string textToSend = text;
          if (textToSend.Length > maxPayloadSize) {
            textToSend = textToSend.Substring(0, maxPayloadSize);
            text = text.Substring(maxPayloadSize);
          }
          rEM.Message.GameToGame = Singleton<GameToGame>.Instance.Initialize(i, partCount, messageType, textToSend);
          rEM.SendMessage();
        }
      }
    }

    private void SendWorldStateToWebView() {
      // Send entire current world state over to JS in Web View

      var robot = RobotEngineManager.Instance.CurrentRobot;

      if ((robot != null) && (_WebViewObjectComponent != null)) {
        // Set Cozmo data

        _LatestCozmoState.pos = robot.WorldPosition;
        _LatestCozmoState.poseYaw_d = robot.PoseAngle * Mathf.Rad2Deg;
        _LatestCozmoState.posePitch_d = robot.PitchAngle * Mathf.Rad2Deg;
        _LatestCozmoState.poseRoll_d = robot.RollAngle * Mathf.Rad2Deg;
        _LatestCozmoState.liftHeightFactor = robot.LiftHeightFactor;
        _LatestCozmoState.headAngle_d = robot.HeadAngle * Mathf.Rad2Deg;

        // Set cube data

        LightCube cube1 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE1);
        LightCube cube2 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE2);
        LightCube cube3 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE3);

        SetCubeStateForCodeLab(_LatestCozmoState.cube1, cube1);
        SetCubeStateForCodeLab(_LatestCozmoState.cube2, cube2);
        SetCubeStateForCodeLab(_LatestCozmoState.cube3, cube3);

        // Set Face data

        Face face = InProgressScratchBlock.GetMostRecentlySeenFace();
        const int kMaxVisionFramesSinceSeeingFace = 30;
        if (face != null) {
          _LatestCozmoState.face.pos = face.WorldPosition;
          _LatestCozmoState.face.camPos = face.VizRect.center;
          _LatestCozmoState.face.name = face.Name;
          _LatestCozmoState.face.isVisible = (face.NumVisionFramesSinceLastSeen < kMaxVisionFramesSinceSeeingFace);
          _LatestCozmoState.face.expression = GetFaceExpressionName(face);
        }
        else {
          _LatestCozmoState.face.pos = new Vector3(0.0f, 0.0f, 0.0f);
          _LatestCozmoState.face.camPos = new Vector2(0.0f, 0.0f);
          _LatestCozmoState.face.name = "";
          _LatestCozmoState.face.isVisible = false;
          _LatestCozmoState.face.expression = "";
        }

        // Set Device data
        _LatestCozmoState.device.pitch_d = Input.gyro.attitude.eulerAngles.y;
        _LatestCozmoState.device.roll_d = Input.gyro.attitude.eulerAngles.x;
        _LatestCozmoState.device.yaw_d = Input.gyro.attitude.eulerAngles.z;

        // Serialize _LatestCozmoState to JSON and send to Web / Javascript side
        string cozmoStateAsJSON = JsonConvert.SerializeObject(_LatestCozmoState);
        this.EvaluateJS(@"window.setCozmoState('" + cozmoStateAsJSON + "');");
      }
    }

    public string GetFaceExpressionName(Face face) {
      var expression = face.Expression;
      string expressionName = "";
      int expressionScore = face.ExpressionScore;
      switch (expression) {
      case Anki.Vision.FacialExpression.Happiness:
        expressionName = "happy";
        break;
      case Anki.Vision.FacialExpression.Anger:
      case Anki.Vision.FacialExpression.Sadness:
        const int kMinUpsetExpressionScore = 75;
        if (expressionScore >= kMinUpsetExpressionScore) {
          expressionName = "upset";
        }
        else {
          expressionName = "unknown";
        }
        break;
      default:
        expressionName = "unknown";
        break;
      }
      return expressionName;
    }

    protected override void Update() {
      base.Update();

      if (_SessionState.GetGrammarMode() == GrammarMode.Vertical && IsDisplayingWorkspacePage()) {
        SendWorldStateToWebView();
      }
    }

    private void LoadWebView() {
      // Send EnterSDKMode to engine as we enter this view
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        // TODO: Once we have UI support for selecting horizontal / vertical this setting will come from elsewhere
        bool useVertical = DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.UseVerticalGrammarCodelab;
        GrammarMode grammarMode = useVertical ? GrammarMode.Vertical : GrammarMode.Horizontal;
        _SessionState.StartSession(grammarMode);
        robot.PushDrivingAnimations(AnimationTrigger.Count, AnimationTrigger.Count, AnimationTrigger.Count, kCodeLabGameDrivingAnimLock);
        robot.EnterSDKMode(false);
        robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CodeLabEnter);
      }
      else {
        DAS.Error("Codelab.LoadWebView.NullRobot", "");
      }

      if (_WebViewObject == null) {
        _WebViewObject = new GameObject("WebView", typeof(WebViewObject));
        _WebViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
        _WebViewObjectComponent.Init(WebViewCallback, false, err: WebViewError, ld: WebViewLoaded, enableWKWebView: true);

        int timesPlayedCodeLab = 0;
        DataPersistenceManager.Instance.Data.DefaultProfile.TotalGamesPlayed.TryGetValue(ChallengeID, out timesPlayedCodeLab);
        if (timesPlayedCodeLab <= 0) {
          LoadURL("extra/tutorial.html");
        }
        else {
          LoadURL("extra/projects.html");
        }
      }
    }

    // SetHeadAngleLazy only calls SetHeadAngle if difference is far enough
    private bool SetHeadAngleLazy(float desiredHeadAngle, RobotCallback callback,
                    QueueActionPosition queueActionPosition = QueueActionPosition.NOW, float speed_radPerSec = -1, float accel_radPerSec2 = -1) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      var currentHeadAngle = robot.HeadAngle;
      var angleDiff = desiredHeadAngle - currentHeadAngle;
      var kAngleEpsilon = 0.05; // ~2.8deg // if closer than this then skip the action

      if (System.Math.Abs(angleDiff) > kAngleEpsilon) {
        robot.SetHeadAngle(desiredHeadAngle, callback, queueActionPosition, useExactAngle: true, speed_radPerSec: speed_radPerSec, accel_radPerSec2: accel_radPerSec2);
        return true;
      }
      else {
        return false;
      }
    }

    // SetLiftHeightLazy only calls SetLiftHeight if difference is far enough
    private bool SetLiftHeightLazy(float desiredHeightFactor, RobotCallback callback,
                     QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                     float speed_radPerSec = kLiftSpeed_rps, float accel_radPerSec2 = -1.0f) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      var currentLiftHeightFactor = robot.LiftHeightFactor; // 0..1
      var heightFactorDiff = desiredHeightFactor - currentLiftHeightFactor;
      var kHeightFactorEpsilon = 0.05; // 5% of range // if closer than this then skip the action

      if (System.Math.Abs(heightFactorDiff) > kHeightFactorEpsilon) {
        robot.SetLiftHeight(desiredHeightFactor, callback, queueActionPosition, speed_radPerSec: speed_radPerSec, accel_radPerSec2: accel_radPerSec2);
        return true;
      }
      else {
        return false;
      }
    }

    private bool IsResettingToHomePose() {
      return _HasQueuedResetToHomePose || (_PendingResetToHomeActions > 0);
    }

    private void OnTimeoutForResetToHomeCompleted(bool success) {
      if (success && _HasQueuedResetToHomePose) {
        DAS.Info("CodeLab.OnTimeoutForResetToHomeCompleted.DoStuff", "");
        _HasQueuedResetToHomePose = false;
        ResetRobotToHomePos();
      }
      else {
        DAS.Warn("CodeLab.OnTimeoutForResetToHomeCompleted.WasCancelled", "success = " + success + ", wasQueued = " + _HasQueuedResetToHomePose);
      }
    }

    private void QueueResetRobotToHomePos() {
      if (_HasQueuedResetToHomePose) {
        DAS.Warn("CodeLab.QueueResetRobotToHomePos.AlreadyQueued", "Ignoring");
        return;
      }
      var robot = RobotEngineManager.Instance.CurrentRobot;
      robot.CancelAction(RobotActionType.UNKNOWN);  // Cancel any pending actions
      robot.WaitAction(kTimeoutForResetToHomePose_s, this.OnTimeoutForResetToHomeCompleted);
      _HasQueuedResetToHomePose = true;
    }

    private void CancelQueueResetRobotToHomePos() {
      if (_HasQueuedResetToHomePose) {
        _HasQueuedResetToHomePose = false;
        var robot = RobotEngineManager.Instance.CurrentRobot;
        robot.CancelCallback(OnTimeoutForResetToHomeCompleted);
        // Cancelling all wait actions will cause the handler to receive success=false on the next engine tick
        robot.CancelAction(RobotActionType.WAIT);
      }
    }

    private void ExecuteAllQueuedScratchRequests() {
      if (UpdateIsGettingOffCharger()) {
        // Wait until Cozmo is off the charger
        DAS.Info("CodeLab.ExecuteAllQueuedScratchRequests.Wait", "Queue length = " + _queuedScratchRequests.Count.ToString());
      }
      else {
        DAS.Info("CodeLab.ExecuteAllQueuedScratchRequests.Exec", "Queue length = " + _queuedScratchRequests.Count.ToString());
        // Unqueue all requests (could be one per green-flag in the case of parallel scripts)
        while (_queuedScratchRequests.Count > 0) {
          var scratchRequest = _queuedScratchRequests.Dequeue();
          DAS.Info("CodeLab.ExecuteAllQueuedScratchRequests.UnQueue", "command = '" + scratchRequest.command + "', id = " + scratchRequest.requestId);

          HandleBlockScratchRequest(scratchRequest);
        }
      }
    }

    private void OnResetToHomeCompleted(bool success) {
      --_PendingResetToHomeActions;

      if (_PendingResetToHomeActions <= 0) {
        if (_PendingResetToHomeActions < 0) {
          DAS.Error("CodeLab.OnResetToHomeCompleted.NegativeHomeActions", "_PendingResetToHomeActions = " + _PendingResetToHomeActions);
        }

        DAS.Info("CodeLab.OnResetToHomeCompleted.ExecuteQueue", "");
        ExecuteAllQueuedScratchRequests();
      }
    }

    private void ResetRobotToHomePos() {
      // Reset the robot to a default pose (head straight, lift down)
      // Only does anything if not already close to that pose.
      // Any other in-progress actions will be cancelled by the first queued (NOW) action

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot == null) {
        // Robot has been deleted, app is disconnecting
        return;
      }

      // Cancel any queued up requests and just do it immediately
      CancelQueueResetRobotToHomePos();

      Anki.Cozmo.QueueActionPosition queuePos = Anki.Cozmo.QueueActionPosition.NOW;

      robot.TurnOffAllBackpackBarLED();

      if (_SessionState.GetGrammarMode() == GrammarMode.Vertical) {
        robot.TurnOffAllLights(true);
        robot.DriveWheels(0.0f, 0.0f);
        robot.EnableCubeSleep(true, true);

        //turn off all cube lights
        for (int i = 0; i < 3; i++) {
          _CubeLightColors[i].SetAll(Color.black);
        }
      }

      if (SetHeadAngleLazy(0.0f, callback: this.OnResetToHomeCompleted, queueActionPosition: queuePos)) {
        ++_PendingResetToHomeActions;
        // Ensure subsequent reset actions run in parallel with this
        queuePos = Anki.Cozmo.QueueActionPosition.IN_PARALLEL;
      }

      if (SetLiftHeightLazy(0.0f, callback: this.OnResetToHomeCompleted, queueActionPosition: queuePos)) {
        ++_PendingResetToHomeActions;
        // Ensure subsequent reset actions run in parallel with this
        queuePos = Anki.Cozmo.QueueActionPosition.IN_PARALLEL;
      }

      // Only wait for the neutral face animation if already waiting for something, or there's definitely a face animation to clear
      if ((_PendingResetToHomeActions > 0) || _RequiresResetToNeutralFace) {
        _RequiresResetToNeutralFace = false;
        robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.NeutralFace, this.OnResetToHomeCompleted, queuePos);
        ++_PendingResetToHomeActions;
      }

      if (_PendingResetToHomeActions > 0) {
        DAS.Info("CodeLab.ResetRobotToHomePos.Started", _PendingResetToHomeActions + " Pending Actions");
      }
      else {
        DAS.Info("CodeLab.ResetRobotToHomePos.AlreadyHome", "");
      }
    }

    private void OnGreenFlagClicked() {
      DAS.Info("Codelab.OnGreenFlagClicked", "");
      _SessionState.OnGreenFlagClicked();
    }

    private void OnScriptStart() {
      DAS.Info("Codelab.OnScriptStart", "");
      _SessionState.StartProgram();
      _queuedScratchRequests.Clear();
      ResetRobotToHomePos();
    }

    private void OnScriptStopped() {
      if (_SessionState.GetGrammarMode() == GrammarMode.Horizontal) {
        // We enable the facial expressions when first needed, and disable when scripts end (rather than ref-counting need)
        RobotEngineManager.Instance.CurrentRobot.SetVisionMode(VisionMode.EstimatingFacialExpression, false);
      }
      _SessionState.EndProgram();
      // Release any remaining in-progress scratch blocks (otherwise any waiting on observation will stay alive)
      InProgressScratchBlockPool.ReleaseAllInUse();
      _queuedScratchRequests.Clear();
      QueueResetRobotToHomePos();
    }

    private void OnStopAll() {
      DAS.Info("Codelab.OnStopAll", "");

      _SessionState.OnStopAll();

      // Release any remaining in-progress scratch blocks (otherwise any waiting on observation will stay alive)
      InProgressScratchBlockPool.ReleaseAllInUse();

      _queuedScratchRequests.Clear();

      // Cancel any in-progress actions (unless we're resetting to position)
      // (if we're resetting to position then we will have already canceled other actions,
      // and we don't want to cancel the in-progress reset animations).
      if (!IsResettingToHomePose()) {
        RobotEngineManager.Instance.CurrentRobot.CancelAction(RobotActionType.UNKNOWN);
      }
    }

    private void OnGetCozmoUserAndSampleProjectLists(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnGetCozmoUserAndSampleProjectLists", "");

      // Check which projects we want to display: vertical or horizontal.
      bool showVerticalProjects = (_SessionState.GetGrammarMode() == GrammarMode.Vertical);

      PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;

      // Provide save and load UI with JSON arrays of the user and sample projects.
      string jsCallback = scratchRequest.argString;

      // Provide JavaScript with a list of user projects as JSON, sorted projects by most recently modified.
      // Don't include XML as the JavaScript chokes on it and it isn't necessary to send along with this list.
      defaultProfile.CodeLabProjects.Sort((proj1, proj2) => -proj1.DateTimeLastModifiedUTC.CompareTo(proj2.DateTimeLastModifiedUTC));
      List<CodeLabProject> copyCodeLabProjectList = new List<CodeLabProject>();
      for (int i = 0; i < defaultProfile.CodeLabProjects.Count; i++) {
        var project = defaultProfile.CodeLabProjects[i];

        if (showVerticalProjects && !project.IsVertical) {
          // We want to show only vertical projects so skip the horizontal projects.
          continue;
        }
        else if (!showVerticalProjects && project.IsVertical) {
          // We want to show only horizontal projects so skip the vertical projects.
          continue;
        }

        CodeLabProject proj = new CodeLabProject();
        proj.ProjectUUID = project.ProjectUUID;
        proj.ProjectName = EscapeProjectName(project.ProjectName);
        proj.IsVertical = project.IsVertical;

        copyCodeLabProjectList.Add(proj);
      }
      string userProjectsAsJSON = JsonConvert.SerializeObject(copyCodeLabProjectList);

      // Provide JavaScript with a list of sample projects as JSON, sorted by DisplayOrder.
      // Again, don't include the XML as the JavaScript chokes on it and it isn't necessary to send along with this list.
      _CodeLabSampleProjects.Sort((proj1, proj2) => proj1.DisplayOrder.CompareTo(proj2.DisplayOrder));
      List<CodeLabSampleProject> copyCodeLabSampleProjectList = new List<CodeLabSampleProject>();
      for (int i = 0; i < _CodeLabSampleProjects.Count; i++) {
        var project = _CodeLabSampleProjects[i];

        if (showVerticalProjects && !project.IsVertical) {
          // We want to show only vertical projects so skip the horizontal projects.
          continue;
        }
        else if (!showVerticalProjects && project.IsVertical) {
          // We want to show only horizontal projects so skip the vertical projects.
          continue;
        }

        CodeLabSampleProject proj = new CodeLabSampleProject();
        proj.ProjectUUID = project.ProjectUUID;
        proj.ProjectIconName = project.ProjectIconName;
        proj.ProjectName = EscapeProjectName(project.ProjectName);
        proj.IsVertical = project.IsVertical;
        copyCodeLabSampleProjectList.Add(proj);
      }

      string sampleProjectsAsJSON = JsonConvert.SerializeObject(copyCodeLabSampleProjectList);

      // Call jsCallback with list of serialized Code Lab user projects and sample projects
      this.EvaluateJS(jsCallback + "('" + userProjectsAsJSON + "','" + sampleProjectsAsJSON + "');");
    }

    private void OnSetChallengeBookmark(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnSetChallengeBookmark", "");
      _SessionState.OnChallengesSetSlideNumber(scratchRequest.argUInt);
      _ChallengeBookmark = scratchRequest.argUInt;
    }

    private void OnGetChallengeBookmark(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnGetChallengeBookmark", "");

      // Provide js with current challenge bookmark location
      string jsCallback = scratchRequest.argString;

      this.EvaluateJS(jsCallback + "('" + _ChallengeBookmark + "');");
    }

    private void OnCozmoSaveUserProject(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnCozmoSaveUserProject", "UUID=" + scratchRequest.argUUID);
      // Save both new and existing user projects.
      // Check if this is a new project.
      string projectUUID = scratchRequest.argUUID;
      string projectXML = scratchRequest.argString;

      PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;

      if (String.IsNullOrEmpty(projectUUID)) {
        string newUserProjectName = null;
        CodeLabProject newProject = null;
        try {
          if (defaultProfile == null) {
            DAS.Error("OnCozmoSaveUserProject.NullDefaultProfile", "In saving new Code Lab user project, defaultProfile is null");
          }

          // Create new project with the XML stored in projectXML.

          // Create project name: "My Project 1", "My Project 2", etc.
          newUserProjectName = Localization.GetWithArgs(LocalizationKeys.kCodeLabHorizontalUserProjectMyProject, defaultProfile.CodeLabUserProjectNum);
          defaultProfile.CodeLabUserProjectNum++;

          bool isVertical = _SessionState.GetGrammarMode() == GrammarMode.Vertical;
          newProject = new CodeLabProject(newUserProjectName, projectXML, isVertical);

          if (defaultProfile.CodeLabProjects == null) {
            DAS.Error("OnCozmoSaveUserProject.NullCodeLabProjects", "defaultProfile.CodeLabProjects is null");
          }
          defaultProfile.CodeLabProjects.Add(newProject);

          // Inform workspace that the current work on workspace has been saved to a project.
          this.EvaluateJS("window.newProjectCreated('" + newProject.ProjectUUID + "','" + newProject.ProjectName + "'); ");

          _SessionState.OnCreatedProject(newProject);
        }
        catch (NullReferenceException) {
          DAS.Error("OnCozmoSaveUserProject.NullReferenceExceptionSaveNewProject", "Save new Code Lab user project. CodeLabUserProjectNum = " + defaultProfile.CodeLabUserProjectNum + ", newProject = " + newProject);
        }
      }
      else {
        CodeLabProject projectToUpdate = null;
        try {
          // Project already has a guid. Locate the project then update it.
          projectToUpdate = FindUserProjectWithUUID(projectUUID);
          projectToUpdate.ProjectXML = projectXML;
          projectToUpdate.DateTimeLastModifiedUTC = DateTime.UtcNow;

          _SessionState.OnUpdatedProject(projectToUpdate);
        }
        catch (NullReferenceException) {
          DAS.Error("OnCozmoSaveUserProject.NullReferenceExceptionUpdateProject", "Save existing CodeLab user project. projectUUID = " + projectUUID + ", projectToUpdate = " + projectToUpdate);
        }
      }

      this.EvaluateJS(@"window.saveProjectCompleted();");
    }

    private void OnCozmoDeleteUserProject(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnCozmoDeleteUserProject", "UUID=" + scratchRequest.argString);
      CodeLabProject projectToDelete = FindUserProjectWithUUID(scratchRequest.argString);
      if (projectToDelete != null) {
        PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;
        defaultProfile.CodeLabProjects.Remove(projectToDelete);

        SessionState.DAS_Event("robot.code_lab.deleted_user_project", defaultProfile.CodeLabProjects.Count.ToString());
      }
    }

    private void OnCozmoLoadProjectPage() {
      DAS.Info("Codelab.OnCozmoLoadProjectPage", "");
      SessionState.DAS_Event("robot.code_lab.load_project_page", "");

      OnExitWorkspace();

      LoadURL("extra/projects.html");
    }

    private bool HandleNonBlockScratchRequest(ScratchRequest scratchRequest) {
      // Handle any Scratch requests that don't need an InProgressScratchBlock initializing
      switch (scratchRequest.command) {
      case "cozmoScriptStarted":
        OnScriptStart();
        return true;
      case "cozmoScriptStopped":
        OnScriptStopped();
        return true;
      case "cozmoGreenFlag":
        OnGreenFlagClicked();
        return true;
      case "cozmoStopAll":
        OnStopAll();
        return true;
      case "cozmoSaveOnQuitCompleted":
        // JavaScript is confirming project has been saved,
        // so now we can exit Code Lab. 
        RaiseChallengeQuit();
        return true;
      case "getCozmoUserAndSampleProjectLists":
        OnGetCozmoUserAndSampleProjectLists(scratchRequest);
        return true;
      case "cozmoSetChallengeBookmark":
        OnSetChallengeBookmark(scratchRequest);
        return true;
      case "cozmoGetChallengeBookmark":
        OnGetChallengeBookmark(scratchRequest);
        return true;
      case "cozmoSaveUserProject":
        OnCozmoSaveUserProject(scratchRequest);
        return true;
      case "cozmoRequestToOpenUserProject":
        SessionState.DAS_Event("robot.code_lab.open_user_project", "");
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.DisplayUserProject, scratchRequest.argString);
        return true;
      case "cozmoRequestToOpenSampleProject":
        SessionState.DAS_Event("robot.code_lab.open_sample_project", scratchRequest.argString);
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.DisplaySampleProject, scratchRequest.argString);
        return true;
      case "cozmoRequestToCreateProject":
        SessionState.DAS_Event("robot.code_lab.create_project", "");
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.CreateNewProject, null);
        return true;
      case "cozmoDeleteUserProject":
        OnCozmoDeleteUserProject(scratchRequest);
        return true;
      case "cozmoLoadProjectPage":
        OnCozmoLoadProjectPage();
        return true;
      case "cozmoCloseCodeLab":
        DAS.Info("Codelab.cozmoCloseCodeLab", "");
        RaiseChallengeQuit();
        return true;
      case "cozmoChallengesOpen":
        _SessionState.OnChallengesOpen();
        return true;
      case "cozmoChallengesClose":
        _SessionState.OnChallengesClose();
        return true;
      default:
        return false;
      }
    }

    private void OnDrivingOffChargerCompleted(bool success) {
      _IsDrivingOffCharger = false;
      DAS.Info("CodeLab.OnDrivingOffChargerCompleted.ExecuteQueue", "");
      ExecuteAllQueuedScratchRequests();
    }

    private bool UpdateIsGettingOffCharger() {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot == null) {
        return false;
      }
      bool isOnCharger = ((robot.RobotStatus & RobotStatusFlag.IS_ON_CHARGER) != 0);
      if (isOnCharger && !_IsDrivingOffCharger) {
        DAS.Info("CodeLab.DriveOffCharger", "");
        _IsDrivingOffCharger = true;
        robot.DriveOffChargerContacts(callback: OnDrivingOffChargerCompleted, queueActionPosition: QueueActionPosition.IN_PARALLEL);
      }
      return _IsDrivingOffCharger;
    }

    private void WebViewCallback(string jsonStringFromJS) {
      // Note that prior to WebViewCallback being called, WebViewObject.CallFromJS() calls WWW.UnEscapeURL(), unencoding the jsonStringFromJS.
      string logJSONStringFromJS = jsonStringFromJS;
      if (logJSONStringFromJS.Contains("cozmo_says") || logJSONStringFromJS.Contains("cozmoSays")) {
        // TODO Temporary solution for removing PII from logs.
        // For vertical will need to check more than just CozmoSays,
        // potentially. Also, ideally only strip out CozmoSays
        // payload, not entire JSON string.
        logJSONStringFromJS = PrivacyGuard.HidePersonallyIdentifiableInfo(logJSONStringFromJS);
      }

      ScratchRequest scratchRequest = null;
      try {
        DAS.Info("CodeLabGame.WebViewCallback.Data", "WebViewCallback - JSON from JavaScript: " + logJSONStringFromJS);

        scratchRequest = JsonConvert.DeserializeObject<ScratchRequest>(jsonStringFromJS, GlobalSerializerSettings.JsonSettings);
      }
      catch (Exception exception) {
        if (exception is JsonReaderException || exception is JsonSerializationException) {
          DAS.Error("CodeLabGame.WebViewCallback.Fail", "JSON exception with text: " + logJSONStringFromJS);
          return;
        }
        else {
          throw;
        }
      }

      if (HandleNonBlockScratchRequest(scratchRequest)) {
        return;
      }

      if (IsResettingToHomePose() || UpdateIsGettingOffCharger()) {
        // Any requests from Scratch that occur whilst Cozmo is resetting to home pose are queued
        DAS.Info("CodeLab.QueueScratchReq", "command = '" + scratchRequest.command + "', id = " + scratchRequest.requestId + ", argString = " + scratchRequest.argString);
        _queuedScratchRequests.Enqueue(scratchRequest);
      }
      else {
        HandleBlockScratchRequest(scratchRequest);
      }
    }

    private void TurnInPlace(float turnAngle, RobotCallback callback) {
      float finalTurnAngle = turnAngle;
      const float kExtraAngle = 2.0f * Mathf.Deg2Rad; // All turns seem to, on average, be this short
      if (finalTurnAngle < 0.0f) {
        finalTurnAngle -= kExtraAngle;
      }
      else {
        finalTurnAngle += kExtraAngle;
      }

      var robot = RobotEngineManager.Instance.CurrentRobot;
      //DAS.Info("CodeLab.TurnInPlace.Start", "Turn " + (finalTurnAngle * Mathf.Rad2Deg) + "d from " + (robot.PoseAngle * Mathf.Rad2Deg) + "d");
      robot.TurnInPlace(finalTurnAngle, 0.0f, 0.0f, kToleranceAngle, callback);
    }

    private void TurnInPlaceVertical(float turnAngle, float speed_deg_per_sec, RobotCallback callback) {
      float finalTurnAngle = turnAngle * Mathf.Deg2Rad;
      const float kExtraAngle = 0.0f * Mathf.Deg2Rad; // No adjustment in vertical for now...
      if (finalTurnAngle < 0.0f) {
        finalTurnAngle -= kExtraAngle;
      }
      else {
        finalTurnAngle += kExtraAngle;
      }
      float speed_rad_per_sec = speed_deg_per_sec * Mathf.Deg2Rad;
      float accel_rad_per_sec2 = 0.0f;
      float toleranceAngle = ((Math.Abs(turnAngle) > 25.0f) ? 10.0f : 5.0f) * Mathf.Deg2Rad;
      var robot = RobotEngineManager.Instance.CurrentRobot;
      robot.TurnInPlace(finalTurnAngle, speed_rad_per_sec, accel_rad_per_sec2, toleranceAngle, callback, QueueActionPosition.IN_PARALLEL);
    }

    private bool HandleDrawOnFaceRequest(ScratchRequest scratchRequest) {
      switch (scratchRequest.command) {
      case "cozVertCozmoFaceClear":
        _CozmoFaceDisplay.ClearScreen(0);
        return true;
      case "cozVertCozmoFaceDisplay":
        _CozmoFaceDisplay.Display();
        return true;
      case "cozVertCozmoFaceDrawLine": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          float x2 = scratchRequest.argFloat3;
          float y2 = scratchRequest.argFloat4;
          byte drawColor = scratchRequest.argBool ? (byte)1 : (byte)0;
          _CozmoFaceDisplay.DrawLine(x1, y1, x2, y2, drawColor);
          return true;
        }
      case "cozVertCozmoFaceFillRect": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          float x2 = scratchRequest.argFloat3;
          float y2 = scratchRequest.argFloat4;
          byte drawColor = scratchRequest.argBool ? (byte)1 : (byte)0;
          _CozmoFaceDisplay.FillRect(x1, y1, x2, y2, drawColor);
          return true;
        }
      case "cozVertCozmoFaceDrawRect": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          float x2 = scratchRequest.argFloat3;
          float y2 = scratchRequest.argFloat4;
          byte drawColor = scratchRequest.argBool ? (byte)1 : (byte)0;
          _CozmoFaceDisplay.DrawRect(x1, y1, x2, y2, drawColor);
          return true;
        }
      case "cozVertCozmoFaceFillCircle": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          float radius = scratchRequest.argFloat3;
          byte drawColor = scratchRequest.argBool ? (byte)1 : (byte)0;
          _CozmoFaceDisplay.FillCircle(x1, y1, radius, drawColor);
          return true;
        }
      case "cozVertCozmoFaceDrawCircle": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          float radius = scratchRequest.argFloat3;
          byte drawColor = scratchRequest.argBool ? (byte)1 : (byte)0;
          _CozmoFaceDisplay.DrawCircle(x1, y1, radius, drawColor);
          return true;
        }
      case "cozVertCozmoFaceDrawText": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          float scale = scratchRequest.argFloat3;
          string text = scratchRequest.argString;
          byte drawColor = scratchRequest.argBool ? (byte)1 : (byte)0;
          _CozmoFaceDisplay.DrawText(x1, y1, scale, text, drawColor);
          return true;
        }
      default:
        return false;
      }
    }

    private void HandleBlockScratchRequest(ScratchRequest scratchRequest) {
      InProgressScratchBlock inProgressScratchBlock = InProgressScratchBlockPool.GetInProgressScratchBlock();
      inProgressScratchBlock.Init(scratchRequest.requestId, this);
      var robot = RobotEngineManager.Instance.CurrentRobot;

      if (robot == null) {
        // No robot - it probably disconnected, advance to the next scratch block
        inProgressScratchBlock.AdvanceToNextBlock(true);
        return;
      }

      if (HandleDrawOnFaceRequest(scratchRequest)) {
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozVertPathOffset") {
        float offsetX = scratchRequest.argFloat;
        float offsetY = scratchRequest.argFloat2;
        float offsetAngle = scratchRequest.argFloat3 * Mathf.Deg2Rad;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(offsetX.ToString() + " , " + offsetY.ToString() + " , " + offsetAngle.ToString()));
        // Offset is in current robot space, so rotate        
        float currentAngle = robot.PoseAngle;
        float cosAngle = Mathf.Cos(currentAngle);
        float sinAngle = Mathf.Sin(currentAngle);
        float newX = robot.WorldPosition.x + cosAngle * offsetX - sinAngle * offsetY;
        float newY = robot.WorldPosition.y + sinAngle * offsetX + cosAngle * offsetY;
        float newAngle = robot.PoseAngle + offsetAngle;
        bool level = false;
        bool useManualSpeed = false;
        robot.GotoPose(newX, newY, newAngle, level, useManualSpeed, inProgressScratchBlock.AdvanceToNextBlock, QueueActionPosition.IN_PARALLEL);
      }
      else if (scratchRequest.command == "cozVertPathTo") {
        float newX = scratchRequest.argFloat;
        float newY = scratchRequest.argFloat2;
        float newAngle = scratchRequest.argFloat3 * Mathf.Deg2Rad;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(newX.ToString() + " , " + newY.ToString() + " , " + newAngle.ToString()));
        bool level = false;
        bool useManualSpeed = false;
        robot.GotoPose(newX, newY, newAngle, level, useManualSpeed, inProgressScratchBlock.AdvanceToNextBlock, QueueActionPosition.IN_PARALLEL);
      }
      else if (scratchRequest.command == "cozVertHeadAngle") {
        float angle = scratchRequest.argFloat * Mathf.Deg2Rad;
        float speed = scratchRequest.argFloat2 * Mathf.Deg2Rad;
        float accel = -1.0f;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(angle.ToString() + " , " + speed.ToString()));
        if (!SetHeadAngleLazy(angle, inProgressScratchBlock.AdvanceToNextBlock, QueueActionPosition.IN_PARALLEL, speed, accel)) {
          inProgressScratchBlock.AdvanceToNextBlock(true);
        }
      }
      else if (scratchRequest.command == "cozVertLiftHeight") {
        float liftHeight = scratchRequest.argFloat;
        float speed = scratchRequest.argFloat2 * Mathf.Deg2Rad;
        float accel = -1.0f;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(liftHeight.ToString() + " , " + speed.ToString()));

        if (!SetLiftHeightLazy(liftHeight, inProgressScratchBlock.AdvanceToNextBlock, QueueActionPosition.IN_PARALLEL, speed, accel)) {
          inProgressScratchBlock.AdvanceToNextBlock(true);
        }
      }
      else if (scratchRequest.command == "cozVertMoveLift") {
        float speed = scratchRequest.argFloat * Mathf.Deg2Rad;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(speed.ToString()));

        robot.MoveLift(speed);
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozVertTurn") {
        float angle = scratchRequest.argFloat;
        float speed = scratchRequest.argFloat2;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(angle.ToString() + " , " + speed.ToString()));
        TurnInPlaceVertical(angle, speed, inProgressScratchBlock.CompletedTurn);
      }
      else if (scratchRequest.command == "cozVertDrive") {
        float dist_mm = scratchRequest.argFloat;
        float speed = scratchRequest.argFloat2;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString() + " , " + speed.ToString()));
        robot.DriveStraightAction(speed, dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock, QueueActionPosition.IN_PARALLEL);
      }
      else if (scratchRequest.command == "cozVertDriveWheels") {
        float leftSpeed = scratchRequest.argFloat;
        float rightSpeed = scratchRequest.argFloat2;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(leftSpeed.ToString() + " , " + rightSpeed.ToString()));
        robot.DriveWheels(leftSpeed, rightSpeed);
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozVertStopMotor") {
        string motorToStop = scratchRequest.argString;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(motorToStop));
        switch (motorToStop) {
        case "wheels":
          robot.DriveWheels(0.0f, 0.0f);
          break;
        case "head":
          robot.DriveHead(0.0f);
          break;
        case "lift":
          robot.MoveLift(0.0f);
          break;
        case "all":
          robot.StopAllMotors();
          break;
        }
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozmoDriveForward") {
        // argFloat represents the number selected from the dropdown under the "drive forward" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        robot.DriveStraightAction(kNormalDriveSpeed_mmps, dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoDriveForwardFast") {
        // argFloat represents the number selected from the dropdown under the "drive forward fast" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        robot.DriveStraightAction(kFastDriveSpeed_mmps, dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoDriveBackward") {
        // argFloat represents the number selected from the dropdown under the "drive backward" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        robot.DriveStraightAction(kNormalDriveSpeed_mmps, -dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoDriveBackwardFast") {
        // argFloat represents the number selected from the dropdown under the "drive backward fast" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        robot.DriveStraightAction(kFastDriveSpeed_mmps, -dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoPlayAnimation") {
        Anki.Cozmo.AnimationTrigger animationTrigger = GetAnimationTriggerForScratchName(scratchRequest.argString);
        bool wasMystery = (scratchRequest.argUInt != 0);
        bool shouldIgnoreBodyTrack = false;
        bool shouldIgnoreHead = false;
        bool shouldIgnoreLift = false;
        if (_SessionState.GetGrammarMode() == GrammarMode.Vertical) {
          shouldIgnoreBodyTrack = scratchRequest.argBool;
          shouldIgnoreHead = scratchRequest.argBool2;
          shouldIgnoreLift = scratchRequest.argBool3;
        }
        _SessionState.ScratchBlockEvent(scratchRequest.command + (wasMystery ? "Mystery" : ""), DASUtil.FormatExtraData(scratchRequest.argString));
        robot.SendAnimationTrigger(animationTrigger, inProgressScratchBlock.NeutralFaceThenAdvanceToNextBlock, ignoreBodyTrack: shouldIgnoreBodyTrack, ignoreHeadTrack: shouldIgnoreHead, ignoreLiftTrack: shouldIgnoreLift);
        _RequiresResetToNeutralFace = true;
      }
      else if (scratchRequest.command == "cozmoTurnLeft") {
        // Turn 90 degrees to the left
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(kTurnAngle.ToString()));
        TurnInPlace(kTurnAngle, inProgressScratchBlock.CompletedTurn);
      }
      else if (scratchRequest.command == "cozmoTurnRight") {
        // Turn 90 degrees to the right
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData((-kTurnAngle).ToString()));
        TurnInPlace(-kTurnAngle, inProgressScratchBlock.CompletedTurn);
      }
      else if (scratchRequest.command == "cozmoSays") {
        string cozmoSaysText = scratchRequest.argString;
        string cozmoSaysTextCleaned = "";

        // Clean the Cozmo Says text input using the same process as Cozmo Says minigame
        for (int i = 0; i < cozmoSaysText.Length; i++) {
          char currentChar = cozmoSaysText[i];
          if (CozmoInputFilter.IsValidInput(currentChar, allowPunctuation: true, allowDigits: true)) {
            cozmoSaysTextCleaned += currentChar;
          }
        }

        bool hasBadWords = BadWordsFilterManager.Instance.Contains(cozmoSaysTextCleaned);
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(hasBadWords.ToString()));  // deliberately don't send string as it's PII
        if (hasBadWords) {
          robot.SendAnimationTrigger(AnimationTrigger.CozmoSaysBadWord, inProgressScratchBlock.AdvanceToNextBlock);
        }
        else {
          robot.SayTextWithEvent(cozmoSaysTextCleaned, AnimationTrigger.Count, callback: inProgressScratchBlock.AdvanceToNextBlock);
        }
      }
      else if (scratchRequest.command == "cozmoHeadAngle") {
        float desiredHeadAngle = (CozmoUtil.kIdealBlockViewHeadValue + CozmoUtil.kIdealFaceViewHeadValue) * 0.5f; // medium setting
        if (scratchRequest.argString == "low") {
          desiredHeadAngle = CozmoUtil.kIdealBlockViewHeadValue;
        }
        else if (scratchRequest.argString == "high") {
          desiredHeadAngle = CozmoUtil.kIdealFaceViewHeadValue;
        }
        // convert angle-factor to an actual angle
        desiredHeadAngle = CozmoUtil.HeadAngleFactorToRadians(desiredHeadAngle, false);

        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData((desiredHeadAngle * Mathf.Rad2Deg).ToString()));

        if (!SetHeadAngleLazy(desiredHeadAngle, inProgressScratchBlock.AdvanceToNextBlock)) {
          // Trigger a short wait action to ensure that our promise is met after this method exits
          robot.WaitAction(0.01f, inProgressScratchBlock.AdvanceToNextBlock);
        }
      }
      else if (scratchRequest.command == "cozmoDockWithCube") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        float desiredHeadAngle = CozmoUtil.HeadAngleFactorToRadians(CozmoUtil.kIdealBlockViewHeadValue, false);
        if (!SetHeadAngleLazy(desiredHeadAngle, inProgressScratchBlock.DockWithCube)) {
          // Trigger a very short wait action first instead to ensure callbacks happen after this method exits
          robot.WaitAction(0.01f, callback: inProgressScratchBlock.DockWithCube);
        }
      }
      else if (scratchRequest.command == "cozVertDockWithCubeById") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        uint cubeIndex = scratchRequest.argUInt;
        LightCube cubeToDockWith = robot.GetLightCubeWithObjectType(GetLightCubeIdFromIndex(cubeIndex));
        if ((cubeToDockWith != null)) {
          robot.AlignWithObject(cubeToDockWith, 0.0f, callback: inProgressScratchBlock.AdvanceToNextBlock, usePreDockPose: true, alignmentType: Anki.Cozmo.AlignmentType.LIFT_PLATE, queueActionPosition: QueueActionPosition.IN_PARALLEL, numRetries: 2);
        }
        else {
          DAS.Warn("DockWithCube.NoCube", "CubeId: " + cubeIndex);
          inProgressScratchBlock.AdvanceToNextBlock(true);
        }
      }
      else if (scratchRequest.command == "cozmoForklift") {
        float liftHeight = 0.5f; // medium setting
        if (scratchRequest.argString == "low") {
          liftHeight = 0.0f;
        }
        else if (scratchRequest.argString == "high") {
          liftHeight = 1.0f;
        }

        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(liftHeight.ToString()));
        if (!SetLiftHeightLazy(liftHeight, inProgressScratchBlock.AdvanceToNextBlock)) {
          // Trigger a short wait action to ensure that our promise is met after this method exits
          robot.WaitAction(0.01f, inProgressScratchBlock.AdvanceToNextBlock);
        }
      }
      else if (scratchRequest.command == "cozmoSetBackpackColor") {
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(scratchRequest.argString));
        robot.SetAllBackpackBarLED(scratchRequest.argUInt);
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozmoVerticalSetBackpackColor") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        robot.SetAllBackpackBarLED(scratchRequest.argUInt);
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozVertSetCubeLightCorner") {
        uint color = scratchRequest.argUInt;
        uint cubeIndex = scratchRequest.argUInt2;
        uint lightIndex = scratchRequest.argUInt3;
        LightCube cubeToLight = robot.GetLightCubeWithObjectType(GetLightCubeIdFromIndex(cubeIndex));
        if (cubeToLight != null) {
          switch (lightIndex) {
          case 4:
            //This is the case where the user selects all lights to be set the same color.
            _CubeLightColors[cubeIndex - 1].SetAll(color.ToColor());
            break;
          default:
            //The user is changing only one cube light, so we keep the other corners set to their current light.
            _CubeLightColors[cubeIndex - 1].SetColor(color.ToColor(), (int)lightIndex);
            break;
          }
          cubeToLight.SetLEDs(_CubeLightColors[cubeIndex - 1].Colors);
        }
        else {
          DAS.Error("CodeLab.NullCube", "No connected cube with index " + cubeIndex.ToString());
        }
      }
      else if (scratchRequest.command == "cozVertCubeAnimation") {
        uint color = scratchRequest.argUInt;
        uint cubeIndex = scratchRequest.argUInt2;
        var cubeToAnimate = robot.GetLightCubeWithObjectType(GetLightCubeIdFromIndex(cubeIndex));
        if (cubeToAnimate != null) {
          string cubeAnim = scratchRequest.argString;
          cubeToAnimate.SetLEDsOff();
          switch (cubeAnim) {
          case "spin":
            Color[] colorArray = { color.ToColor(), Color.black, Color.black, Color.black };
            StartCycleCube(cubeToAnimate.ID, colorArray, 0.05f);
            break;
          case "blink":
            cubeToAnimate.SetFlashingLEDs(color.ToColor());
            break;
          }
        }
        else {
          DAS.Error("CodeLab.NullCube", "No connected cube with index " + cubeIndex.ToString());
        }
      }
      else if (scratchRequest.command == "cozmoWaitUntilSeeFace") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        RobotEngineManager.Instance.AddCallback<RobotObservedFace>(inProgressScratchBlock.RobotObservedFace);
      }
      else if (scratchRequest.command == "cozmoWaitUntilSeeHappyFace") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.EstimatingFacialExpression, true);
        RobotEngineManager.Instance.AddCallback<RobotObservedFace>(inProgressScratchBlock.RobotObservedHappyFace);
      }
      else if (scratchRequest.command == "cozmoWaitUntilSeeSadFace") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.EstimatingFacialExpression, true);
        RobotEngineManager.Instance.AddCallback<RobotObservedFace>(inProgressScratchBlock.RobotObservedSadFace);
      }
      else if (scratchRequest.command == "cozmoWaitUntilSeeCube") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        RobotEngineManager.Instance.AddCallback<RobotObservedObject>(inProgressScratchBlock.RobotObservedObject);
      }
      else if (scratchRequest.command == "cozmoWaitForCubeTap") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        LightCube.TappedAction += inProgressScratchBlock.CubeTapped;
      }
      else {
        inProgressScratchBlock.ReleaseFromPool();
        DAS.Error("HandleBlockScratchRequest.UnknownCommand", "Command = '" + scratchRequest.command + "'");
      }

      return;
    }

    private ObjectType GetLightCubeIdFromIndex(uint cubeIndex) {
      switch (cubeIndex) {
      case 1:
        return ObjectType.Block_LIGHTCUBE1;
      case 2:
        return ObjectType.Block_LIGHTCUBE2;
      case 3:
        return ObjectType.Block_LIGHTCUBE3;
      default:
        DAS.Error("CodeLab.BadCubeIndex", "cubeIndex: " + cubeIndex.ToString());
        return ObjectType.UnknownObject;
      }
    }
    private ObjectType GetLightCubeIndexFromId(int cubeId) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      var cube1 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE1);
      var cube2 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE2);
      var cube3 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE3);
      if (cube1 != null && cubeId == cube1.ID) {
        return ObjectType.Block_LIGHTCUBE1;
      }
      else if (cube2 != null && cubeId == cube2.ID) {
        return ObjectType.Block_LIGHTCUBE2;
      }
      else if (cube3 != null && cubeId == cube3.ID) {
        return ObjectType.Block_LIGHTCUBE3;
      }
      else {
        DAS.Error("CodeLab.BadCubeId", "cubeId " + cubeId.ToString());
        return ObjectType.UnknownObject;
      }
    }

    private void OpenCodeLabProject(RequestToOpenProjectOnWorkspace request, string projectUUID) {
      DAS.Info("Codelab.OpenCodeLabProject", "request=" + request + ", UUID=" + projectUUID);
      // Cache the request to open project. These vars will be used after the webview is loaded but before it is visible.
      SetRequestToOpenProject(request, projectUUID);
      ShowGettingReadyScreen();
      if (_SessionState.GetGrammarMode() == GrammarMode.Vertical) {
        LoadURL(kVerticalIndexFilename);
      }
      else {
        LoadURL(kHorizontalIndexFilename);
      }
    }

    // Display blue "Cozmo is getting ready to play" while the Scratch workspace is finishing setup
    private void ShowGettingReadyScreen() {
      _WebViewObjectComponent.SetVisibility(false);

      SharedMinigameView.HideQuitButton();
      PopulateTitleWidget();

      SharedMinigameView.InitializeColor(UIColorPalette.GameBackgroundColor);

      SharedMinigameView.ShowWideSlideWithText(LocalizationKeys.kMinigameLabelCozmoPrep, null);
      SharedMinigameView.ShowShelf();
      SharedMinigameView.ShowSpinnerWidget();
    }

    private string GetRootPath() {
      if (_DevLoadPath != null) {
        // Loading assets from a cache path instead of the baked-in assets
        return _DevLoadPath;
      }
      else {
#if UNITY_EDITOR
        return Application.streamingAssetsPath + "/Scratch/";
#else
        return PlatformUtil.GetResourcesBaseFolder() + "/Scratch/";
#endif
      }
    }

    private string GetUrlPath(string filename) {
#if UNITY_EDITOR
      return filename;
#else
      string urlProtocol = "file://";
      if ((_DevLoadPath != null) && _DevLoadPathFirstRequest) {
#if UNITY_IOS
        // For some odd reason iOS will only load from cache after a (failed) initial request at "file:////var..."
        urlProtocol += "/";
#endif
      }
      return urlProtocol + filename;
#endif // UNITY_EDITOR
    }

    private bool IsDisplayingWorkspacePage() {
      if (_CurrentURLFilename.IndexOf(kHorizontalIndexFilename, StringComparison.Ordinal) > -1) {
        return true;
      }
      else if (_CurrentURLFilename.IndexOf(kVerticalIndexFilename, StringComparison.Ordinal) > -1) {
        return true;
      }
      return false;
    }

    private void LoadURL(string scratchPathToHTML) {
      if (_SessionState.IsProgramRunning()) {
        // Program is currently running and we won't receive OnScriptStopped notification
        // when the active page and Javascript is nuked, so manually force an OnScriptStopped event
        DAS.Info("Codelab.ManualOnScriptStopped", "");
        OnScriptStopped();
      }

      if (_WebViewObjectComponent == null) {
        DAS.Error("CodeLab.LoadURL.NullWebView", "Ignoring request to open '" + scratchPathToHTML + "'");
        return;
      }

      string baseFilename = GetRootPath() + scratchPathToHTML;
      string urlPath = GetUrlPath(baseFilename);

      // Append locale parameter to URL
      string locale = Localization.GetStringsLocale();
      urlPath += "?locale=" + locale;

      DAS.Info("CodeLab.LoadURL", "urlPath = '" + urlPath + "'");

      _CurrentURLFilename = scratchPathToHTML;
      _WebViewObjectComponent.LoadURL(urlPath);
      if ((_DevLoadPath != null) && _DevLoadPathFirstRequest) {
        _DevLoadPathFirstRequest = false;
      }

      // Set "lobby" versus "workspace" music
      if (IsDisplayingWorkspacePage()) {
        // Workspace music: plays for horizontal and vertical workspace as well as challenges
        GameAudioClient.SetMusicRoundState((int)MusicRoundStates.WorkspaceMusicRound);
      }
      else {
        // Lobby music: plays for tutorial and save/load UI
        GameAudioClient.SetMusicRoundState((int)MusicRoundStates.LobbyMusicRound);
      }
    }

    private void OnExitWorkspace() {
      if (_SessionState.GetGrammarMode() == GrammarMode.Vertical) {
        StopVerticalHatBlockListeners();
      }
    }

    private void SetRequestToOpenProject(RequestToOpenProjectOnWorkspace request, string projectUUID) {
      _RequestToOpenProjectOnWorkspace = request;
      _ProjectUUIDToOpen = projectUUID;
    }

    private Anki.Cozmo.AnimationTrigger GetAnimationTriggerForScratchName(string scratchAnimationName) {

      switch (scratchAnimationName) {
      case "bored":
        return Anki.Cozmo.AnimationTrigger.CodeLabBored;
      case "cat":
        return Anki.Cozmo.AnimationTrigger.CodeLabCat;
      case "chatty":
        return Anki.Cozmo.AnimationTrigger.CodeLabChatty;
      case "dejected":
        return Anki.Cozmo.AnimationTrigger.CodeLabDejected;
      case "dog":
        return Anki.Cozmo.AnimationTrigger.CodeLabDog;
      case "excited":
        return Anki.Cozmo.AnimationTrigger.CodeLabExcited;
      case "frustrated":
        return Anki.Cozmo.AnimationTrigger.CodeLabFrustrated;
      case "happy":
        return Anki.Cozmo.AnimationTrigger.CodeLabHappy;
      case "sleep":
        return Anki.Cozmo.AnimationTrigger.CodeLabSleep;
      case "sneeze":
        return Anki.Cozmo.AnimationTrigger.CodeLabSneeze;
      case "surprise":
        return Anki.Cozmo.AnimationTrigger.CodeLabSurprise;
      case "thinking":
        return Anki.Cozmo.AnimationTrigger.CodeLabThinking;
      case "unhappy":
        return Anki.Cozmo.AnimationTrigger.CodeLabUnhappy;
      case "victory":
        return Anki.Cozmo.AnimationTrigger.CodeLabVictory;
      default:
        DAS.Error("CodeLab.BadTriggerName", "Unexpected name '" + scratchAnimationName + "'");
        break;
      }
      return Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentCelebration;
    }

    CodeLabProject FindUserProjectWithUUID(string uuid) {
      Guid projectGuid = new Guid(uuid);
      CodeLabProject codeLabProject = null;

      Predicate<DataPersistence.CodeLabProject> findProject = (CodeLabProject p) => { return p.ProjectUUID == projectGuid; };
      codeLabProject = DataPersistenceManager.Instance.Data.DefaultProfile.CodeLabProjects.Find(findProject);

      return codeLabProject;
    }

    private void WebViewError(string text) {
      DAS.Error("Codelab.WebViewError", string.Format("CallOnError[{0}]", text));
    }

    private void WebViewLoaded(string text) {
      DAS.Info("Codelab.WebViewLoaded", string.Format("CallOnLoaded[{0}]", text));
      RequestToOpenProjectOnWorkspace cachedRequestToOpenProject = _RequestToOpenProjectOnWorkspace;

      switch (_RequestToOpenProjectOnWorkspace) {
      case RequestToOpenProjectOnWorkspace.CreateNewProject:
        // Open workspace and display only a green flag on the workspace.
        this.EvaluateJS("window.ensureGreenFlagIsOnWorkspace();");
        break;

      case RequestToOpenProjectOnWorkspace.DisplayUserProject:
        if (_ProjectUUIDToOpen != null) {
          CodeLabProject projectToOpen = FindUserProjectWithUUID(_ProjectUUIDToOpen);
          if (projectToOpen != null) {
            // Escape quotes in user project name and project XML
            // TODO Should we be fixing this in a different way? May need to make this more robust for vertical release.
            String projectNameEscaped = EscapeProjectName(projectToOpen.ProjectName);
            String projectXMLEscaped = EscapeXML(projectToOpen.ProjectXML);

            // Open requested project in webview
            this.EvaluateJS("window.openCozmoProject('" + projectToOpen.ProjectUUID + "','" + projectNameEscaped + "',\"" + projectXMLEscaped + "\",'false');");
          }
        }
        else {
          DAS.Error("CodeLab.NullUserProject", "User project empty for _ProjectUUIDToOpen = '" + _ProjectUUIDToOpen + "'");
        }
        break;

      case RequestToOpenProjectOnWorkspace.DisplaySampleProject:
        if (_ProjectUUIDToOpen != null) {
          Guid projectGuid = new Guid(_ProjectUUIDToOpen);
          CodeLabSampleProject codeLabSampleProject = null;

          Predicate<CodeLabSampleProject> findProject = (CodeLabSampleProject p) => { return p.ProjectUUID == projectGuid; };
          codeLabSampleProject = _CodeLabSampleProjects.Find(findProject);

          String sampleProjectName = Localization.Get(codeLabSampleProject.ProjectName);

          // Escape quotes in XML and project name
          // TODO Should we be fixing this in a different way? May need to make this more robust for vertical release.
          String sampleProjectNameEscaped = EscapeProjectName(sampleProjectName);
          String projectXMLEscaped = EscapeXML(codeLabSampleProject.ProjectXML);

          // Open requested project in webview
          this.EvaluateJS("window.openCozmoProject('" + codeLabSampleProject.ProjectUUID + "','" + sampleProjectNameEscaped + "',\"" + projectXMLEscaped + "\",'true');");
        }
        else {
          DAS.Error("CodeLab.NullSampleProject", "Sample project empty for _ProjectUUIDToOpen = '" + _ProjectUUIDToOpen + "'");
        }
        break;

      default:
        break;
      }

      SetRequestToOpenProject(RequestToOpenProjectOnWorkspace.DisplayNoProject, null);

      // If we are displaying workspace, give the webview a little time to get ready.
      uint delayInSeconds = 0;
      if (cachedRequestToOpenProject != RequestToOpenProjectOnWorkspace.DisplayNoProject) {
#if UNITY_EDITOR || UNITY_IOS
        delayInSeconds = 1;
#elif UNITY_ANDROID
        delayInSeconds = 2;
#endif
      }
      Invoke("UnhideWebView", delayInSeconds);

      if (_SessionState.GetGrammarMode() == GrammarMode.Vertical) {
        StartVerticalHatBlockListeners();

        //in Vertical, disable cubes illuminating blue when Cozmo sees them
        RobotEngineManager.Instance.CurrentRobot.EnableCubeSleep(true, true);
      }
    }

    private String EscapeProjectName(String projectName) {
      String tempProjectName = projectName.Replace("\"", "\\\"");
      return tempProjectName.Replace("'", "\\'");
    }

    private String EscapeXML(String xml) {
      return xml.Replace("\"", "\\\"");
    }

    private void StartVerticalHatBlockListeners() {
      // Listen for face events so that we can kick off vertical face hat blocks
      // (including wait for happy, frown and any face)
      RobotEngineManager.Instance.CurrentRobot.SetVisionMode(VisionMode.EstimatingFacialExpression, true);
      RobotEngineManager.Instance.AddCallback<RobotObservedFace>(RobotObservedFaceVerticalHatBlock);

      // Listen for "robot saw a cube" events so we can kick off vertical "wait until see cube" hat block
      RobotEngineManager.Instance.AddCallback<RobotObservedObject>(RobotObservedObjectVerticalHatBlock);

      // Listen for cube tapped events so we can kick off vertical "wait for cube tap" hat block
      LightCube.TappedAction += CubeTappedVerticalHatBlock;

      // Listen for cube moved events so we can kick off vertical "wait for cube moved" hat block
      LightCube.OnMovedAction += CubeMovedVerticalHatBlock;
    }

    private void StopVerticalHatBlockListeners() {
      RobotEngineManager.Instance.RemoveCallback<RobotObservedFace>(RobotObservedFaceVerticalHatBlock);
      RobotEngineManager.Instance.RemoveCallback<RobotObservedObject>(RobotObservedObjectVerticalHatBlock);
      LightCube.TappedAction -= CubeTappedVerticalHatBlock;
      LightCube.OnMovedAction -= CubeMovedVerticalHatBlock;
    }

    public void RobotObservedFaceVerticalHatBlock(RobotObservedFace message) {
      if (message.expression == Anki.Vision.FacialExpression.Happiness) {
        EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_happy_face', null);");
      }
      else if (FaceIsSad(message)) {
        EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_sad_face', null);");
      }

      EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_face', null);");
    }

    public bool FaceIsSad(RobotObservedFace message) {
      // Match on Angry or Sad, and only if score is high (minimize false positives)
      if ((message.expression == Anki.Vision.FacialExpression.Anger) ||
        (message.expression == Anki.Vision.FacialExpression.Sadness)) {
        var expressionScore = message.expressionValues[(int)message.expression];
        const int kMinSadExpressionScore = 75;
        if (expressionScore >= kMinSadExpressionScore) {
          return true;
        }
      }

      return false;
    }

    public void RobotObservedObjectVerticalHatBlock(RobotObservedObject message) {
      EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_see_cube', null);");
    }

    public void CubeTappedVerticalHatBlock(int id, int tappedTimes, float timeStamp) {
      EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_cube_tap', {CUBE_SELECT: \"" + ((int)GetLightCubeIndexFromId(id)) + "\"});");
    }

    public void CubeMovedVerticalHatBlock(int id, float XAccel, float YAccel, float ZAccel) {
      EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_cube_moved', {CUBE_SELECT: \"" + ((int)GetLightCubeIndexFromId(id)) + "\"});");
    }

    void UnhideWebView() {
      SharedMinigameView.HideMiddleBackground();

      // Add in black screen, so it looks like less of a pop.
      SharedMinigameView.ShowFullScreenGameStateSlide(_BlackScreenPrefab, "BlackScreen");
      SharedMinigameView.HideSpinnerWidget();

      _WebViewObjectComponent.SetVisibility(true);
    }
  }
}
