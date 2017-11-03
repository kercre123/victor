#if !SHIPPING
#define ENABLE_TEST_PROJECTS
#endif

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
    public string ProjectJSON;
    public string ProjectName; // Stored in sample-projects.js as string key.
    public string DASProjectName; // DAS-friendly name for analytics (same in all languages, and never changes)
    public string Language;
  }

  // CodeLabFeaturedProject are only used in vertical grammar.
  public class CodeLabFeaturedProject {
    public uint DisplayOrder;
    public Guid ProjectUUID;
    public uint VersionNum;
    public string ProjectJSON;
    public string ProjectName; // Stored in featured-projects.js as string key.
    public string DASProjectName; // DAS-friendly name for analytics (same in all languages, and never changes)
    public string FeaturedProjectDescription; // Stored in featured-projects.js as string key.
    public string FeaturedProjectImageName;
    public string FeaturedProjectBackgroundColor;
    public string FeaturedProjectTitleTextColor;
    public string FeaturedProjectInstructions;
    public string Language;
  }

  public class CodeLabGame : GameBase {
    // When the webview is opened to display the Code Lab workspace,
    // we can display one of the following projects.
    private enum RequestToOpenProjectOnWorkspace {
      DisplayNoProject,     // Unset value
      CreateNewProject,     // Workspace displays only green flag
      DisplayUserProject,
      DisplaySampleProject,
      DisplayFeaturedProject
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
    private List<CodeLabFeaturedProject> _CodeLabFeaturedProjects;

    // SessionState tracks the entirety of the current CodeLab Game session, including any currently running program.
    private SessionState _SessionState = new SessionState();

    // Any requests from Scratch that occur whilst Cozmo is resetting to home pose are queued
    private Queue<ScratchRequest> _queuedScratchRequests = new Queue<ScratchRequest>();

    private CodeLabCozmoFaceDisplay _CozmoFaceDisplay = new CodeLabCozmoFaceDisplay();

#if !UNITY_EDITOR
#if UNITY_ANDROID
    private AndroidJavaObject _CozmoAndroidActivity;
#endif
#endif

    private int _PendingResetToHomeActions = 0;
    private bool _HasQueuedResetToHomePose = false;
    private bool _RequiresResetToNeutralFace = false;
    private bool _IsDrivingOffCharger = false;
    private bool _VerticalHatBlocksStarted = false;
    private string _LastOpenedTab = "featured";

    private const string kCodeLabGameDrivingAnimLock = "code_lab_game";
    private static readonly string kCodelabPrefix = "CODELAB:";
    private static readonly long kMaximumDescriptionLength = 1000; // assuming any file title over 1000 characters is suspect
    private static readonly long kMaximumCodelabDataLength = 10000000; // assuming any file over 10 Mb is suspect

#if ANKI_DEV_CHEATS
    PerformanceStats _PerformanceStats = new PerformanceStats();
#endif // ANKI_DEV_CHEATS

    private CubeColors[] _CubeLightColors = { new CubeColors(),
                                     new CubeColors(),
                                     new CubeColors() };

    private static readonly List<string> BlocksWithPIIRisks = new List<string>() {
      "cozmo_says", "cozmo_play_animation_from_dropdown", "cozmo_play_animation_by_name", "cozmo_play_animation_by_triggername",
      "event_broadcast", "event_broadcastandwait", // NOTE: Currently messages cannot be named and are not PII risks, but I wanted to future proof against that change
      "cozmo_vert_cozmoface_draw_text",
      "cozmoSays", "cozmoPlayAnimation", "cozVertPlayNamedAnim", "cozVertPlayNamedTriggerAnim",
      "cozVertCozmoFaceDrawText" };

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

    private bool _IsOrchestraPlaying = false;
    private bool _IsPerformingIntroActions = false;

    private const float kNormalDriveSpeed_mmps = 70.0f;
    private const float kFastDriveSpeed_mmps = 200.0f;
    private const float kDriveDist_mm = 44.0f; // length of one light cube
    private const float kTurnAngle = 90.0f * Mathf.Deg2Rad;
    private const float kToleranceAngle = 10.0f * Mathf.Deg2Rad;
    private const float kLiftSpeed_rps = 2.0f;
    private const float kTimeoutForResetToHomePose_s = 3.0f;

    private const string kHorizontalIndexFilename = "index.html";
    private const string kVerticalIndexFilename = "index_vertical.html";

    public const int kMaxFramesSinceCubeTapped = 999999999;
    private const int kMaxFramesToReportCubeTap = 15; // 15 frames ~=0.5 seconds

    private const float kMaxAngleClamp = 3600.0f; // 10 full rotations
    private const float kMinAngularSpeedClamp = 2.5f; // 2.5 degrees per second
    private const float kMaxAngularSpeedClamp = 1800.0f; // 5 full rotations per second - Cozmo's lift can supposedly do up to 1700
    private const float kMaxDistanceClamp = 100000.0f; // 100 meters
    private const float kMinSpeedClamp = 3.0f; // 3 m/s is extremely slow
    private const float kMaxSpeedClamp = 1000.0f; // 1 m/s (Cozmo's real max is ~220m/s)

    private float ClampAngleInput(float inputAngle) {
      return Mathf.Clamp(inputAngle, -kMaxAngleClamp, kMaxAngleClamp);
    }

    private float ClampHeadAngleInput(float inputAngle) {
      return Mathf.Clamp(inputAngle, CozmoUtil.kMinHeadAngle, CozmoUtil.kMaxHeadAngle);
    }

    private float ClampAngularSpeedInput(float inputSpeed) {
      return Mathf.Clamp(inputSpeed, -kMaxAngularSpeedClamp, kMaxAngularSpeedClamp);
    }

    private float ClampPositiveAngularSpeedInput(float inputSpeed) {
      return Mathf.Clamp(inputSpeed, kMinAngularSpeedClamp, kMaxAngularSpeedClamp);
    }

    private float ClampDistanceInput(float inputDistance) {
      return Mathf.Clamp(inputDistance, -kMaxDistanceClamp, kMaxDistanceClamp);
    }

    private float ClampSpeedInput(float inputSpeed) {
      return Mathf.Clamp(inputSpeed, -kMaxSpeedClamp, kMaxSpeedClamp);
    }

    private float ClampPositiveSpeedInput(float inputSpeed) {
      return Mathf.Clamp(inputSpeed, kMinSpeedClamp, kMaxSpeedClamp);
    }

    private float ClampPercentageInput(float inputValue, float maxValue = 100.0f) {
      return Mathf.Clamp(inputValue, 0.0f, maxValue);
    }

    // @TODO: Currently nothing is done with this list.
    //  it should be used when we want to surface errors in the codelab file import process that occur before the ui
    //  is ready for them.  At present the list is not cleared out, and could get quite long if many external files are
    //  loaded incorrectly during the same session.
    private static List<string> _importErrors = new List<string>();

    protected override void InitializeGame(ChallengeConfigBase challengeConfigData) {

      _SessionState.StartSession();

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

#if !UNITY_EDITOR
#if UNITY_ANDROID
      _CozmoAndroidActivity = new AndroidJavaClass("com.unity3d.player.UnityPlayer").GetStatic<AndroidJavaObject>("currentActivity");
#endif
#endif

      // Cache sample and featured projects locally.
      _CodeLabSampleProjects = this.LoadSampleProjects("sample-projects.json");
      _CodeLabFeaturedProjects = this.LoadFeaturedProjects("featured-projects.json");

#if ENABLE_TEST_PROJECTS
      if (DataPersistenceManager.Instance.Data.DebugPrefs.LoadTestCodeLabProjects) {
        var testProjects = this.LoadSampleProjects("test-projects.json");
        _CodeLabSampleProjects.AddRange(testProjects);
      }
#endif

      RobotEngineManager.Instance.AddCallback<GameToGame>(HandleGameToGame);

      LoadWebView();
    }

    // Return sample projects as list.
    private List<CodeLabSampleProject> LoadSampleProjects(string projectFile) {
      string scratchFolder = "/Scratch/";
#if UNITY_EDITOR || UNITY_IOS
      string streamingAssetsPath = Application.streamingAssetsPath + scratchFolder;
#elif UNITY_ANDROID
      string streamingAssetsPath = PlatformUtil.GetResourcesBaseFolder() + scratchFolder;
#endif

      // Load projects json from file and return list
      string path = streamingAssetsPath + projectFile;
      string json = File.ReadAllText(path);
      return JsonConvert.DeserializeObject<List<CodeLabSampleProject>>(json);
    }

    // Return featured projects as list.
    private List<CodeLabFeaturedProject> LoadFeaturedProjects(string projectFile) {
      string scratchFolder = "/Scratch/";
#if UNITY_EDITOR || UNITY_IOS
      string streamingAssetsPath = Application.streamingAssetsPath + scratchFolder;
#elif UNITY_ANDROID
      string streamingAssetsPath = PlatformUtil.GetResourcesBaseFolder() + scratchFolder;
#endif

      // Load projects json from file and return list
      string path = streamingAssetsPath + projectFile;
      string json = File.ReadAllText(path);
      return JsonConvert.DeserializeObject<List<CodeLabFeaturedProject>>(json);
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
      case "EnsureProjectExists":
        // Ensure that there is a project on C# side with given UUID and name
        // this allows subsequent calls from an attached Python-served workspace to still
        // save to Unity too

        PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;
        if (defaultProfile == null) {
          DAS.Error("CodeLab.EnsureProjectExists.NullDefaultProfile", "");
        }
        else {
          // Extract from message
          var payloadArgs = payload.Split(',');
          if (payloadArgs.Length != 3) {
            DAS.Error("CodeLab.EnsureProjectExists.WrongArgLength", "Length: " + payloadArgs.Length);
          }
          else {
            string projectUUID = payloadArgs[0];
            string projectName = payloadArgs[1];
            bool isVertical = payloadArgs[2] != "0";

            _SessionState.SetGrammarMode(isVertical ? GrammarMode.Vertical : GrammarMode.Horizontal);

            DAS.Info("CodeLab.EnsureProjectExists.Adding", "uuid='" + projectUUID + "' isVertical=" + isVertical);

            CodeLabProject project = FindUserProjectWithUUID(projectUUID);
            if (project == null) {
              // Create it
              project = new CodeLabProject(projectName, null, isVertical);
              project.ProjectUUID = new Guid(projectUUID);
              defaultProfile.CodeLabProjects.Add(project);
            }
          }
        }
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
        robot.ExitSDKMode(false);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.EstimatingFacialExpression, false);
        // Put cubes back to normal Freeplay behavior
        robot.SetEnableFreeplayLightStates(true);
        robot.EnableCubeSleep(false);
      }

      _SessionState.EndSession();

      ResetAllCodeLabAudio();
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

    private void ResetLatestCozmoState() {
      // Only reset data that persists or affects multiple frames (most data is set directly each frame before sending)
      _LatestCozmoState.cube1.framesSinceTapped = kMaxFramesSinceCubeTapped;
      _LatestCozmoState.cube2.framesSinceTapped = kMaxFramesSinceCubeTapped;
      _LatestCozmoState.cube2.framesSinceTapped = kMaxFramesSinceCubeTapped;
    }

    private void ResetCozmoOnNewWorkspace() {
      ResetLatestCozmoState();
      _CozmoFaceDisplay.ClearScreen();
    }

    public void EvaluateJS(string text) {
#if ANKI_DEV_CHEATS
      _PerformanceStats.AddEvaluateJSCall();
#endif // ANKI_DEV_CHEATS
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

    private int CheckLatestCubeTapped(CubeStateForCodeLab cubeState, int cubeId, int mostRecentTap) {
      if (cubeState.framesSinceTapped < mostRecentTap) {
        _LatestCozmoState.lastTappedCube = cubeId;
        mostRecentTap = cubeState.framesSinceTapped;
      }
      return mostRecentTap;
    }

    private float Convert360AngleToMinus180To180(float inAngle) {
      if (inAngle < 180.0f) {
        if (inAngle < 0.0f) {
          if (inAngle < -0.1f) {
            DAS.Error("CodeLab.Convert360AngleToMinus180To180.TooSmall", "inAngle: " + inAngle);
          }
          return 0.0f;
        }
        return inAngle; // 1st half maps 0 to 180
      }
      else {
        if (inAngle > 360.0f) {
          if (inAngle > 360.1f) {
            DAS.Error("CodeLab.Convert360AngleToMinus180To180.TooBig", "inAngle: " + inAngle);
          }
          return 0.0f;
        }
        return inAngle - 360.0f; // 2nd half maps to -180 to -0
      }
    }

    private Vector3 EnginePosToCodeLabPos(Vector3 worldPos) {
      // We invert Y so it's to Cozmo's right instead of left (to match how turn right with positive rotations)
      return new Vector3(worldPos.x, -worldPos.y, worldPos.z);
    }

    private void SetCubeStateForCodeLab(CubeStateForCodeLab cubeDest, LightCube cubeSrc) {
      if (cubeSrc == null) {
        cubeDest.pos.Set(0.0f, 0.0f, 0.0f);
        cubeDest.camPos.Set(0.0f, 0.0f);
        cubeDest.isValid = false;
        cubeDest.isVisible = false;
        cubeDest.pitch_d = 0.0f;
        cubeDest.roll_d = 0.0f;
        cubeDest.yaw_d = 0.0f;
      }
      else {
        cubeDest.pos.Set(EnginePosToCodeLabPos(cubeSrc.WorldPosition));
        cubeDest.camPos.Set(cubeSrc.VizRect.center);
        cubeDest.isValid = true;
        const int kMaxVisionFramesSinceSeeingCube = 30;
        cubeDest.isVisible = (cubeSrc.NumVisionFramesSinceLastSeen < kMaxVisionFramesSinceSeeingCube);
        // All cube angles are in 0..360 range so must be re-mapped to match robot angle range
        cubeDest.pitch_d = ValueSanitizer.SanitizeFloat(Convert360AngleToMinus180To180(cubeSrc.PitchDegrees));
        cubeDest.roll_d = ValueSanitizer.SanitizeFloat(Convert360AngleToMinus180To180(cubeSrc.RollDegrees));
        cubeDest.yaw_d = ValueSanitizer.SanitizeFloat(-Convert360AngleToMinus180To180(cubeSrc.YawDegrees)); // Yaw is inverted as we switched to clockwise rotations for Code Lab
      }

      cubeDest.wasJustTapped = (cubeDest.framesSinceTapped < kMaxFramesToReportCubeTap);
    }

    private Matrix4x4 MatrixFromQuaternion(Quaternion Q) {
      Matrix4x4 result = new Matrix4x4();
      result.m00 = 1.0f - 2.0f * Q.y * Q.y - 2.0f * Q.z * Q.z;
      result.m01 = 2.0f * Q.x * Q.y - 2.0f * Q.z * Q.w;
      result.m02 = 2.0f * Q.x * Q.z + 2.0f * Q.y * Q.w;

      result.m10 = 2.0f * Q.x * Q.y + 2.0f * Q.z * Q.w;
      result.m11 = 1.0f - 2.0f * Q.x * Q.x - 2.0f * Q.z * Q.z;
      result.m12 = 2.0f * Q.y * Q.z - 2.0f * Q.x * Q.w;

      result.m20 = 2.0f * Q.x * Q.z - 2.0f * Q.y * Q.w;
      result.m21 = 2.0f * Q.y * Q.z + 2.0f * Q.x * Q.w;
      result.m22 = 1.0f - 2.0f * Q.x * Q.x - 2.0f * Q.y * Q.y;
      return result;
    }

    private void SendWorldStateToWebView() {
      // Send entire current world state over to JS in Web View

      var robot = RobotEngineManager.Instance.CurrentRobot;

      if ((robot != null) && (_WebViewObjectComponent != null)) {
        // Set Cozmo data

        _LatestCozmoState.pos.Set(EnginePosToCodeLabPos(robot.WorldPosition));
        _LatestCozmoState.poseYaw_d = ValueSanitizer.SanitizeFloat(-robot.PoseAngle * Mathf.Rad2Deg); // Yaw is inverted as we switched to clockwise rotations for Code Lab
        _LatestCozmoState.posePitch_d = ValueSanitizer.SanitizeFloat(robot.PitchAngle * Mathf.Rad2Deg);
        _LatestCozmoState.poseRoll_d = ValueSanitizer.SanitizeFloat(robot.RollAngle * Mathf.Rad2Deg);
        _LatestCozmoState.liftHeightPercentage = ValueSanitizer.SanitizeFloat(robot.LiftHeightFactor * 100.0f);
        _LatestCozmoState.headAngle_d = ValueSanitizer.SanitizeFloat(robot.HeadAngle * Mathf.Rad2Deg);

        // Set cube data

        LightCube cube1 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE1);
        LightCube cube2 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE2);
        LightCube cube3 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE3);

        SetCubeStateForCodeLab(_LatestCozmoState.cube1, cube1);
        SetCubeStateForCodeLab(_LatestCozmoState.cube2, cube2);
        SetCubeStateForCodeLab(_LatestCozmoState.cube3, cube3);

        _LatestCozmoState.lastTappedCube = 0;
        int mostRecentTap = kMaxFramesSinceCubeTapped;
        mostRecentTap = CheckLatestCubeTapped(_LatestCozmoState.cube1, 1, mostRecentTap);
        mostRecentTap = CheckLatestCubeTapped(_LatestCozmoState.cube2, 2, mostRecentTap);
        mostRecentTap = CheckLatestCubeTapped(_LatestCozmoState.cube3, 3, mostRecentTap);

        // Set Face data

        Face face = InProgressScratchBlock.GetMostRecentlySeenFace();
        const int kMaxVisionFramesSinceSeeingFace = 30;
        if (face != null) {
          _LatestCozmoState.face.pos.Set(EnginePosToCodeLabPos(face.WorldPosition));
          _LatestCozmoState.face.camPos.Set(face.VizRect.center);
          _LatestCozmoState.face.name = face.Name;
          _LatestCozmoState.face.isVisible = (face.NumVisionFramesSinceLastSeen < kMaxVisionFramesSinceSeeingFace);
          _LatestCozmoState.face.expression = GetFaceExpressionName(face);
        }
        else {
          _LatestCozmoState.face.pos.Set(0.0f, 0.0f, 0.0f);
          _LatestCozmoState.face.camPos.Set(0.0f, 0.0f);
          _LatestCozmoState.face.name = "";
          _LatestCozmoState.face.isVisible = false;
          _LatestCozmoState.face.expression = "";
        }

        StoreDeviceOrientationInState();

        // Serialize _LatestCozmoState to JSON and send to Web / Javascript side
        string cozmoStateAsJSON = JsonConvert.SerializeObject(_LatestCozmoState);
        this.EvaluateJS(@"window.setCozmoState('" + cozmoStateAsJSON + "');");
      }
    }

    private enum UnreliableOrientationDetectionState {
      NoInformation = 0,
      GyroSeeminglyNonpresent,
      GyroDetected
    }
    private bool _DeviceGyroSupportReportingInaccurrate = SystemInfo.deviceModel.Contains("Amazon");
    private UnreliableOrientationDetectionState unreliableOrientationDetectionState = UnreliableOrientationDetectionState.NoInformation;

    private void StoreDeviceOrientationInState() {
      bool gyroPresent = SystemInfo.supportsGyroscope && Input.gyro.enabled;
      bool accelerometerPresent = SystemInfo.supportsAccelerometer;

      if (_DeviceGyroSupportReportingInaccurrate) {
        // The kindle i'm testing on says it has a gyro, but the gyro is returning (0, 0, 1, 0) regardless of orientation
        // This is not as cleanly diagnosable as "wrong" as if it were returning identity, but is clearly not useful
        bool suspiciouslyPreciseGyro =
               Input.gyro.attitude.x == Math.Floor(Input.gyro.attitude.x) &&
               Input.gyro.attitude.y == Math.Floor(Input.gyro.attitude.y) &&
               Input.gyro.attitude.z == Math.Floor(Input.gyro.attitude.z) &&
               Input.gyro.attitude.w == Math.Floor(Input.gyro.attitude.w);

        switch (unreliableOrientationDetectionState) {
        case UnreliableOrientationDetectionState.NoInformation:
          if (gyroPresent && suspiciouslyPreciseGyro) {
            unreliableOrientationDetectionState = UnreliableOrientationDetectionState.GyroSeeminglyNonpresent;
          }
          break;
        case UnreliableOrientationDetectionState.GyroSeeminglyNonpresent:
          if (!suspiciouslyPreciseGyro) {
            unreliableOrientationDetectionState = UnreliableOrientationDetectionState.GyroDetected;
          }
          break;
        case UnreliableOrientationDetectionState.GyroDetected:
          break;
        }

        if (unreliableOrientationDetectionState == UnreliableOrientationDetectionState.GyroSeeminglyNonpresent) {
          gyroPresent = false;
        }
      }

      if (gyroPresent) {
        // Transform gyro quaternion to matrix
        Matrix4x4 deviceAttitude = MatrixFromQuaternion(Input.gyro.attitude);

        float sy = Mathf.Sqrt(deviceAttitude.m21 * deviceAttitude.m21 + deviceAttitude.m22 * deviceAttitude.m22);

        float pitch, yaw, roll;

        bool singular = sy < 1e-6;
        if (!singular) {
          // matrix parameter extraction taken from: http://planning.cs.uiuc.edu/node103.html
          pitch = Mathf.Atan2(deviceAttitude.m22, deviceAttitude.m21);
          yaw = Mathf.Atan2(deviceAttitude.m00, deviceAttitude.m10);
          roll = Mathf.Atan2(sy, -deviceAttitude.m20);
        }
        else {
          pitch = Mathf.Atan2(deviceAttitude.m11, -deviceAttitude.m12);
          yaw = 0.0f;
          roll = Mathf.Atan2(sy, -deviceAttitude.m20);
        }

        // Invert the roll (we want clockwise roll to be positive), and offset it so that facing upward is roll 0 instead of -90
        roll = (Mathf.PI * 0.5f) - roll;
        if (roll > Mathf.PI) {
          roll -= Mathf.PI * 2.0f;
        }

        _LatestCozmoState.device.pitch_d = ValueSanitizer.SanitizeFloat(pitch * Mathf.Rad2Deg);
        _LatestCozmoState.device.yaw_d = ValueSanitizer.SanitizeFloat(yaw * Mathf.Rad2Deg);
        _LatestCozmoState.device.roll_d = ValueSanitizer.SanitizeFloat(roll * Mathf.Rad2Deg);
      }
      else if (accelerometerPresent) {
        // calculate pitch and roll values that work while device is held in a near landscape neutral angle
        float standardPitchStrength = -Mathf.Atan2(Input.acceleration.z, -Input.acceleration.y);
        float yzStrength = (Mathf.Abs(Input.acceleration.y) + Mathf.Abs(Input.acceleration.z)) * -Math.Sign(Input.acceleration.y);
        float standardRollStrength = Mathf.Atan2(Input.acceleration.x, yzStrength);
        // correct when upside down - since the sign of Y may not exactly corrolate to what the user thinks of as rolling at high pitch
        if (Mathf.Abs(standardPitchStrength) >= Mathf.PI / 2) {
          standardRollStrength = (Mathf.PI - Mathf.Abs(standardRollStrength)) * Mathf.Sign(standardRollStrength);
        }

        // calculate a roll factor for when the device is held close to on its side (where the yz interactions are no longer useful)
        float sidewaysRollStrength = Mathf.Atan2(Input.acceleration.x, -Input.acceleration.y);

        // generate a lerp factor to transition between the two perspectives
        // unfortunately there really isn't enough information to make an informed decision about pitch close to vertical.
        float conversionStartFactor = 0.875f; // a bit over 45 degrees
        float conversionEndFactor = 0.975f; // pretty close to vertical
        float truncatedDotProduct = Mathf.Max(0.0f, Mathf.Abs(Vector3.Dot(Input.acceleration, Vector3.right)) - conversionStartFactor);
        float conversionFactor = Mathf.Min(1.0f, truncatedDotProduct / (conversionEndFactor - conversionStartFactor));

        float pitch = Mathf.Lerp(standardPitchStrength, 0.0f, conversionFactor);
        float roll = Mathf.Lerp(standardRollStrength, sidewaysRollStrength, conversionFactor);

        _LatestCozmoState.device.pitch_d = ValueSanitizer.SanitizeFloat(pitch * Mathf.Rad2Deg);
        _LatestCozmoState.device.yaw_d = 0.0f;
        _LatestCozmoState.device.roll_d = ValueSanitizer.SanitizeFloat(roll * Mathf.Rad2Deg);
      }
      else {
        _LatestCozmoState.device.pitch_d = 0.0f;
        _LatestCozmoState.device.yaw_d = 0.0f;
        _LatestCozmoState.device.roll_d = 0.0f;
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

    static int ClampedIncrement(int intValue, int maxValue) {
      // Increment up to a maximum, avoiding overflow
      return (intValue < maxValue) ? (intValue + 1) : maxValue;
    }

    static void ClampedPostIncrement(ref int inOutValue, int maxValue) {
      inOutValue = ClampedIncrement(inOutValue, maxValue);
    }

    protected override void Update() {
      base.Update();

      if (_SessionState.GetGrammarMode() == GrammarMode.Vertical && IsDisplayingWorkspacePage()) {
        ClampedPostIncrement(ref _LatestCozmoState.cube1.framesSinceTapped, kMaxFramesSinceCubeTapped);
        ClampedPostIncrement(ref _LatestCozmoState.cube2.framesSinceTapped, kMaxFramesSinceCubeTapped);
        ClampedPostIncrement(ref _LatestCozmoState.cube3.framesSinceTapped, kMaxFramesSinceCubeTapped);

        SendWorldStateToWebView();
        // NOTE:" UpdateBlocks is currently only required on Vertical workspaces, and is purely to allow custom blocks
        // like "WaitForActions(ActionType)" to work, as they need to be polled each update.
        InProgressScratchBlockPool.UpdateBlocks();
      }

#if ANKI_DEV_CHEATS
      _PerformanceStats.Update(this, _SessionState.IsProgramRunning());
#endif
    }

    private void LoadProjectsURL() {
      SessionState.DAS_Event("robot.code_lab.load_project_page", _LastOpenedTab);

      Dictionary<string, string> urlParameters = new Dictionary<string, string>();
      urlParameters["projects"] = _LastOpenedTab;

      LoadURL("extra/projects.html", urlParameters);
    }

    private void OnLoadedOnChargerStep1Complete(bool success) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        DAS.Info("CodeLab.OnLoadedOnChargerStep1", "Step2: Drive further");
        _IsPerformingIntroActions = true;
        robot.DriveStraightAction(kNormalDriveSpeed_mmps, 80, false, OnLoadedOnChargerStep2Complete);
      }
      else {
        DAS.Error("CodeLab.OnLoadedOnChargerStep1Complete.NullRobot", "");
        _IsPerformingIntroActions = false;
      }
    }

    private void OnLoadedOnChargerStep2Complete(bool success) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        DAS.Info("CodeLab.OnLoadedOnChargerStep2Complete", "Step2 Complete: success = " + success + " Start Step3: Anim");
        _IsPerformingIntroActions = true;
        robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CodeLabEnter, OnLoadedOnChargerStep3Complete);
      }
      else {
        DAS.Error("CodeLab.OnLoadedOnChargerStep2Complete.NullRobot", "");
        _IsPerformingIntroActions = false;
      }
    }

    private void OnLoadedOnChargerStep3Complete(bool success) {
      DAS.Info("CodeLab.OnLoadedOnChargerStep3Complete", "Step3 Complete: success = " + success);
      _IsPerformingIntroActions = false;
    }

    private void LoadWebView() {
      // Send EnterSDKMode to engine as we enter this view
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.PushDrivingAnimations(AnimationTrigger.Count, AnimationTrigger.Count, AnimationTrigger.Count, kCodeLabGameDrivingAnimLock);
        robot.EnterSDKMode(false);
        bool isOnCharger = ((robot.RobotStatus & RobotStatusFlag.IS_ON_CHARGER) != 0);
        if (isOnCharger) {
          DAS.Info("CodeLab.LoadWebView.OnCharger", "Step1: Drive off contacts");
          _IsPerformingIntroActions = true;
          robot.DriveOffChargerContacts(callback: OnLoadedOnChargerStep1Complete);
        }
        else {
          DAS.Info("CodeLab.LoadWebView.OffCharger", "Last Step");
          OnLoadedOnChargerStep2Complete(true);
        }
      }
      else {
        DAS.Error("Codelab.LoadWebView.NullRobot", "");
      }

      if (_WebViewObject == null) {
        _WebViewObject = new GameObject("WebView", typeof(WebViewObject));
        _WebViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
        _WebViewObjectComponent.Init(WebViewCallback, false, err: WebViewError, ld: WebViewLoaded, enableWKWebView: true);

        if (Cozmo.WhatsNew.WhatsNewModalManager.ShouldAutoOpenProject) {
          OpenCodeLabProject(RequestToOpenProjectOnWorkspace.DisplayFeaturedProject,
                             Cozmo.WhatsNew.WhatsNewModalManager.AutoOpenCodeLabProjectGuid.ToString(),
                             isVertical: true);
        }
        else {
          LoadProjectsURL();
        }
      }
    }

    // SetHeadAngleLazy only calls SetHeadAngle if difference is far enough
    private uint SetHeadAngleLazy(float desiredHeadAngle, RobotCallback callback,
                    QueueActionPosition queueActionPosition = QueueActionPosition.NOW, float speed_radPerSec = -1, float accel_radPerSec2 = -1) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      var currentHeadAngle = robot.HeadAngle;
      var angleDiff = desiredHeadAngle - currentHeadAngle;
      var kAngleEpsilon = 0.05; // ~2.8deg // if closer than this then skip the action

      if (System.Math.Abs(angleDiff) > kAngleEpsilon) {
        uint idTag = robot.SetHeadAngle(desiredHeadAngle, callback, queueActionPosition, useExactAngle: true, speed_radPerSec: speed_radPerSec, accel_radPerSec2: accel_radPerSec2);
        return idTag;
      }
      else {
        return (uint)ActionConstants.INVALID_TAG;
      }
    }

    // SetLiftHeightLazy only calls SetLiftHeight if difference is far enough
    private uint SetLiftHeightLazy(float desiredHeightFactor, RobotCallback callback,
                     QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                     float speed_radPerSec = kLiftSpeed_rps, float accel_radPerSec2 = -1.0f) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      var currentLiftHeightFactor = robot.LiftHeightFactor; // 0..1
      var heightFactorDiff = desiredHeightFactor - currentLiftHeightFactor;
      var kHeightFactorEpsilon = 0.05; // 5% of range // if closer than this then skip the action

      if (System.Math.Abs(heightFactorDiff) > kHeightFactorEpsilon) {
        uint idTag = robot.SetLiftHeight(desiredHeightFactor, callback, queueActionPosition, speed_radPerSec: speed_radPerSec, accel_radPerSec2: accel_radPerSec2);
        return idTag;
      }
      else {
        return (uint)ActionConstants.INVALID_TAG;
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

    private void ResetAllCodeLabAudio() {
      DAS.Info("CodeLab.ResetAllCodeLabAudio", "Stopping All Audio - _IsOrchestraPlaying = " + _IsOrchestraPlaying.ToString());

      // Stop any audio the user may have started in their Code Lab program
      GameAudioClient.PostCodeLabEvent(Anki.AudioMetaData.GameEvent.Codelab.Sfx_Global_Stop,
                                        Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventComplete,
                                        (callbackInfo) => { /* callback */ });
      GameAudioClient.PostCodeLabEvent(Anki.AudioMetaData.GameEvent.Codelab.Music_Global_Stop,
                                        Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventComplete,
                                        (callbackInfo) => { /* callback */ });
      _IsOrchestraPlaying = false;

      // Restore background music in case it was turned off
      GameAudioClient.PostCodeLabEvent(Anki.AudioMetaData.GameEvent.Codelab.Music_Background_Silence_Off,
                                        Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventComplete,
                                        (callbackInfo) => { /* callback */ });
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

      robot.CancelAction(RobotActionType.UNKNOWN); // Cancel all current actions
      robot.TurnOffAllBackpackBarLED();

      if (_SessionState.GetGrammarMode() == GrammarMode.Vertical) {
        StopVerticalHatBlockListeners();

        robot.TurnOffAllLights(true);
        robot.StopAllMotors();

        SetupCubeLights();  // Turn off the cube lights

        //turn off all cube lights
        for (int i = 0; i < 3; i++) {
          _CubeLightColors[i].SetAll(Color.black);
        }
      }

      ResetAllCodeLabAudio();

      if (SetHeadAngleLazy(0.0f, callback: this.OnResetToHomeCompleted, queueActionPosition: queuePos) != (uint)ActionConstants.INVALID_TAG) {
        ++_PendingResetToHomeActions;
        // Ensure subsequent reset actions run in parallel with this
        queuePos = Anki.Cozmo.QueueActionPosition.IN_PARALLEL;
      }

      if (SetLiftHeightLazy(0.0f, callback: this.OnResetToHomeCompleted, queueActionPosition: queuePos) != (uint)ActionConstants.INVALID_TAG) {
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

      if (_SessionState.GetGrammarMode() == GrammarMode.Vertical) {
        StartVerticalHatBlockListeners();
      }
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

      if (_SessionState.GetGrammarMode() == GrammarMode.Vertical) {
        StopVerticalHatBlockListeners();
      }

      _SessionState.OnStopAll();

      // Release any remaining in-progress scratch blocks (otherwise any waiting on observation will stay alive)
      InProgressScratchBlockPool.ReleaseAllInUse();

      // Force all audio to stop immediaetly
      ResetAllCodeLabAudio();

      _queuedScratchRequests.Clear();

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        if (_IsPerformingIntroActions) {
          DAS.Info("CodeLab.OnStopAll.StillPerformingIntro", "Not stopping motors or cancelling actions");
        }
        else {
          if (_PendingResetToHomeActions == 0) {
            // If not already actively moving to reset, then stop all motors immediately
            // The motors are stopped eventually in ResetRobotToHomePos, but we want to
            // stop them even earlier if user actively presses the stop button.
            robot.StopAllMotors();
          }

          // Cancel any in-progress actions (unless we're resetting to position)
          // (if we're resetting to position then we will have already canceled other actions,
          // and we don't want to cancel the in-progress reset animations).
          if (!IsResettingToHomePose()) {
            robot.CancelAction(RobotActionType.UNKNOWN);
          }
        }
      }
    }

    // Called by js to retrieve list of user and sample projects to display in lobby.
    private void OnGetCozmoUserAndSampleProjectLists(ScratchRequest scratchRequest, bool showVerticalProjects) {
      DAS.Info("Codelab.OnGetCozmoUserAndSampleProjectLists", "");

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
        proj.ProjectName = project.ProjectName;
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

        if (!string.IsNullOrEmpty(project.Language) && project.Language != Localization.GetStringsLocale()) {
          continue;
        }
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
        proj.ProjectName = project.ProjectName; // value is string key
        proj.IsVertical = project.IsVertical;
        proj.VersionNum = project.VersionNum;
        if (proj.VersionNum > CodeLabProject.kCurrentVersionNum) {
          // NOTE: While sending ProjectName's is generally a PII risk, Sample projects will never have user configurable names
          DAS.Warn("Codelab.OnAppLoadedFromData.BadVersionNumber", "sample project " + proj.ProjectName + "'s version number " + project.VersionNum.ToString() + " is greater than the app's codelab version " + CodeLabProject.kCurrentVersionNum.ToString());
        }
        else {
          copyCodeLabSampleProjectList.Add(proj);
        }
      }

      string sampleProjectsAsJSON = JsonConvert.SerializeObject(copyCodeLabSampleProjectList);

      // Call jsCallback with list of serialized Code Lab user projects and sample projects
      this.EvaluateJS(jsCallback + "('" + EscapeProjectText(userProjectsAsJSON) + "','" + EscapeProjectText(sampleProjectsAsJSON) + "');");
    }

    // Called by js to retrieve list of featured projects to display in lobby.
    private void OnGetCozmoFeaturedProjectList(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnGetCozmoFeaturedProjectList", "");

      // Provide JavaScript with a list of featured projects as JSON, sorted by DisplayOrder.
      // Don't include the ProjectJSON as it isn't necessary to send along with this list.
      string jsCallback = scratchRequest.argString;

      _CodeLabFeaturedProjects.Sort((proj1, proj2) => proj1.DisplayOrder.CompareTo(proj2.DisplayOrder));
      List<CodeLabFeaturedProject> copyCodeLabFeaturedProjectList = new List<CodeLabFeaturedProject>();
      for (int i = 0; i < _CodeLabFeaturedProjects.Count; i++) {
        var project = _CodeLabFeaturedProjects[i];

        if (!string.IsNullOrEmpty(project.Language) && project.Language != Localization.GetStringsLocale()) {
          continue;
        }

        CodeLabFeaturedProject proj = new CodeLabFeaturedProject();
        proj.ProjectUUID = project.ProjectUUID;
        proj.ProjectName = project.ProjectName; // value is string key
        proj.VersionNum = project.VersionNum;
        proj.FeaturedProjectDescription = project.FeaturedProjectDescription; // value is string key
        proj.FeaturedProjectImageName = project.FeaturedProjectImageName;
        proj.FeaturedProjectBackgroundColor = project.FeaturedProjectBackgroundColor;
        proj.FeaturedProjectTitleTextColor = project.FeaturedProjectTitleTextColor;
        proj.FeaturedProjectInstructions = project.FeaturedProjectInstructions;

        if (proj.VersionNum > CodeLabProject.kCurrentVersionNum) {
          // NOTE: While sending ProjectName's is generally a PII risk, Sample projects will never have user configurable names
          DAS.Warn("Codelab.OnAppLoadedFromData.BadVersionNumber", "featured project " + proj.ProjectName + "'s version number " + project.VersionNum.ToString() + " is greater than the app's codelab version " + CodeLabProject.kCurrentVersionNum.ToString());
        }
        else {
          copyCodeLabFeaturedProjectList.Add(proj);
        }
      }

      string featuredProjectsAsJSON = JsonConvert.SerializeObject(copyCodeLabFeaturedProjectList);

      // Call jsCallback with list of serialized Code Lab featured projects
      this.EvaluateJS(jsCallback + "('" + EscapeProjectText(featuredProjectsAsJSON) + "');");
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
      string projectJSON = scratchRequest.argString;

      PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;

      if (String.IsNullOrEmpty(projectUUID)) {
        string newUserProjectName = null;
        CodeLabProject newProject = null;
        try {
          if (defaultProfile == null) {
            DAS.Error("OnCozmoSaveUserProject.NullDefaultProfile", "In saving new Code Lab user project, defaultProfile is null");
          }

          // Create new project with the JSON stored in projectJSON.

          // Create project name: "My Project 1", "My Project 2", etc.
          bool isVertical = _SessionState.GetGrammarMode() == GrammarMode.Vertical;

          if (isVertical) {
            newUserProjectName = Localization.GetWithArgs(LocalizationKeys.kCodeLabHorizontalUserProjectMyProject, defaultProfile.CodeLabUserProjectNumVertical);
            defaultProfile.CodeLabUserProjectNumVertical++;
          }
          else {
            newUserProjectName = Localization.GetWithArgs(LocalizationKeys.kCodeLabHorizontalUserProjectMyProject, defaultProfile.CodeLabUserProjectNum);
            defaultProfile.CodeLabUserProjectNum++;
          }

          newProject = new CodeLabProject(newUserProjectName, projectJSON, isVertical);

          if (defaultProfile.CodeLabProjects == null) {
            DAS.Error("OnCozmoSaveUserProject.NullCodeLabProjects", "defaultProfile.CodeLabProjects is null");
          }
          defaultProfile.CodeLabProjects.Add(newProject);

          // Inform workspace that the current work on workspace has been saved to a project.
          this.EvaluateJS("window.newProjectCreated('" + newProject.ProjectUUID + "','" + newProject.ProjectName + "'); ");

          _SessionState.OnCreatedProject(newProject, false);
        }
        catch (NullReferenceException) {
          string sanitizedNewProject = PrivacyGuard.HidePersonallyIdentifiableInfo(newProject.ProjectJSON);
          DAS.Error("OnCozmoSaveUserProject.NullReferenceExceptionSaveNewProject", "Save new Code Lab user project. CodeLabUserProjectNum = " + defaultProfile.CodeLabUserProjectNum + ", newProject = " + sanitizedNewProject);
        }
      }
      else {
        // Project already has a guid. Locate the project then update it.
        CodeLabProject projectToUpdate = FindUserProjectWithUUID(projectUUID);
        if (projectToUpdate != null) {
          projectToUpdate.ProjectJSON = projectJSON;
          projectToUpdate.DateTimeLastModifiedUTC = DateTime.UtcNow;
          projectToUpdate.ProjectXML = null; // Set ProjectXML to null as this project might have previously been in XML and we don't want to store it anymore.
          projectToUpdate.VersionNum = CodeLabProject.kCurrentVersionNum;

          _SessionState.OnUpdatedProject(projectToUpdate);
        }
        else {
          DAS.Error("OnCozmoSaveUserProject.NullUserProject", "User project unexpectedly not found.");
        }
      }

      this.EvaluateJS(@"window.saveProjectCompleted();");

      // Save data to disk
      DataPersistenceManager.Instance.Save();
    }

    private void OnCozmoDeleteUserProject(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnCozmoDeleteUserProject", "UUID=" + scratchRequest.argString);
      CodeLabProject projectToDelete = FindUserProjectWithUUID(scratchRequest.argString);
      if (projectToDelete != null) {
        PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;
        defaultProfile.CodeLabProjects.Remove(projectToDelete);

        SessionState.DAS_Event("robot.code_lab.deleted_user_project", defaultProfile.CodeLabProjects.Count.ToString(),
                               SessionState.DAS_Format_IsVertical(projectToDelete.IsVertical));
      }
    }

    private void OnCozmoLoadProjectPage() {
      DAS.Info("Codelab.OnCozmoLoadProjectPage", "");

      OnExitWorkspace();

      LoadProjectsURL();
    }

    private void OnCozmoSwitchProjectTab(ScratchRequest scratchRequest) {
      SessionState.DAS_Event("robot.code_lab.switch_project_tab", scratchRequest.argString);
      _LastOpenedTab = scratchRequest.argString;
    }

    // Given a UUID, find the existing project (from the user, sample or featured project lists) and return a new project of type CodeLabProject
    // with a new UUID.
    private CodeLabProject FindProjectByUUIDAndCreateCodeLabProject(string projectUUID, string projectType) {
      CodeLabProject codeLabProject = null;
      if (String.IsNullOrEmpty(projectUUID)) {
        DAS.Error("Codelab.FindProjectByUUID.BadUUID", "Attempt to find project with no project UUID specified.");
      }
      else {
        switch (projectType) {
        case "user":
          CodeLabProject codeLabUserProject = FindUserProjectWithUUID(projectUUID);
          if (codeLabUserProject != null) {
            codeLabProject = new CodeLabProject(codeLabUserProject.ProjectName, codeLabUserProject.ProjectJSON, codeLabUserProject.IsVertical);
            codeLabProject.BaseDASProjectName = codeLabUserProject.BaseDASProjectName ?? "";
            codeLabProject.VersionNum = codeLabUserProject.VersionNum;
            break;
          }

          break;
        case "sample":
          // @TODO: when we pass in "featured" as a project type, break out this behavior into two cases
          Guid projectGuid = new Guid(projectUUID);

          Predicate<CodeLabSampleProject> findSampleProject = (CodeLabSampleProject p) => { return p.ProjectUUID == projectGuid; };
          CodeLabSampleProject sampleProject = _CodeLabSampleProjects.Find(findSampleProject);
          if (sampleProject != null) {
            codeLabProject = new CodeLabProject(sampleProject.ProjectName, sampleProject.ProjectJSON, sampleProject.IsVertical);
            codeLabProject.BaseDASProjectName = sampleProject.DASProjectName;
            codeLabProject.VersionNum = sampleProject.VersionNum;
            break;
          }

          Predicate<CodeLabFeaturedProject> findFeaturedProject = (CodeLabFeaturedProject p) => { return p.ProjectUUID == projectGuid; };
          CodeLabFeaturedProject featuredProject = _CodeLabFeaturedProjects.Find(findFeaturedProject);
          if (featuredProject != null) {
            codeLabProject = new CodeLabProject(featuredProject.ProjectName, featuredProject.ProjectJSON, true);
            codeLabProject.BaseDASProjectName = featuredProject.DASProjectName;
            codeLabProject.VersionNum = featuredProject.VersionNum;
          }

          break;
        }
      }

      return codeLabProject;
    }

    // This callback manages identifying a CodeLabProject from a user project export request from the workspace and handling it appropriately.
    private void OnCozmoExportProject(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnCozmoExportProject.Called", "User intends to share project");

      string projectUUID = scratchRequest.argUUID;

      if (String.IsNullOrEmpty(projectUUID)) {
        DAS.Error("Codelab.OnCozmoExportProject.BadUUID", "Attempt to export project with no project UUID specified.");
      }
      else {
        string projectType = scratchRequest.argString;

        CodeLabProject projectToExport = FindProjectByUUIDAndCreateCodeLabProject(projectUUID, projectType);

        if (projectToExport != null) {
          string projectToExportJSON = kCodelabPrefix + WWW.EscapeURL(projectToExport.GetSerializedJson());

          SessionState.DAS_Event("robot.code_lab.export_project", projectType,
                                 DASUtil.FormatExtraData(projectToExportJSON.Length.ToString()));

          System.Action<string, string> sendFileCall = null;

#if !UNITY_EDITOR
#if UNITY_IPHONE
          sendFileCall = (string name, string json) => IOS_Settings.ExportCodelabFile(name, json);
#elif UNITY_ANDROID
          sendFileCall = (string name, string json) => _CozmoAndroidActivity.Call<Boolean>("exportCodeLabFile", name, json);
#else
          sendFileCall = (string name, string json) => DAS.Error("Codelab.OnCozmoShareProject.PlatformNotSupported", "Platform not supported");
#endif
#else
          sendFileCall = (string name, string json) => DAS.Error("Codelab.OnCozmoShareProject.PlatformNotSupportedEditor", "Unity Editor does not implement sharing codelab project");
#endif

          if (string.IsNullOrEmpty(projectToExportJSON)) {
            DAS.Error("Codelab.OnCozmoShareProject.ProjectJsonNullOrEmpty", "Error creating Json string");
          }
          else if (sendFileCall == null) {
            DAS.Error("Codelab.OnCozmoShareProject.PlatformNotFound", "Could not create an export call for the current platform.");
          }
          else {
            sendFileCall(projectToExport.ProjectName, projectToExportJSON);
          }
        }
        else {
          DAS.Error("Codelab.OnCozmoExportProject.NullProject", "Could not find the project of type=" + projectType);
        }
      }
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
        OnGetCozmoUserAndSampleProjectLists(scratchRequest, scratchRequest.argBool);
        return true;
      case "getCozmoFeaturedProjectList":
        OnGetCozmoFeaturedProjectList(scratchRequest);
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
        SessionState.DAS_Event("robot.code_lab.open_user_project", "", SessionState.DAS_Format_IsVertical(scratchRequest.argBool));
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.DisplayUserProject, scratchRequest.argString, scratchRequest.argBool);
        return true;
      case "cozmoRequestToOpenSampleProject":
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.DisplaySampleProject, scratchRequest.argString, scratchRequest.argBool);
        return true;
      case "cozmoRequestToOpenFeaturedProject":
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.DisplayFeaturedProject, scratchRequest.argString, true);
        return true;
      case "cozmoRequestToCreateProject":
        SessionState.DAS_Event("robot.code_lab.create_project", "", SessionState.DAS_Format_IsVertical(scratchRequest.argBool));
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.CreateNewProject, null, scratchRequest.argBool);
        return true;
      case "cozmoRequestToRenameProject":
        OnCozmoRenameCodeLabProject(scratchRequest);
        return true;
      case "cozmoRequestToRemixProject":
        OnCozmoRemixCodeLabProject(scratchRequest);
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
      case "cozmoTutorialOpen":
        _SessionState.OnTutorialOpen();
        return true;
      case "cozmoTutorialClose":
        _SessionState.OnTutorialClose();
        return true;
      case "cozmoExportProject":
        OnCozmoExportProject(scratchRequest);
        return true;
      case "cozmoSwitchProjectTab":
        OnCozmoSwitchProjectTab(scratchRequest);
        return true;
      case "cozmoWorkspaceLoaded":
        DAS.Info("CodeLab.WorkspaceLoaded", "");
        Invoke("UnhideWebView", 0.25f);
        return true;
      case "cozmoDASLog":
        // Use for debugging from JavaScript
        string sanitizedLogString = PrivacyGuard.HidePersonallyIdentifiableInfo(scratchRequest.argString2);
        DAS.Info(scratchRequest.argString, sanitizedLogString);
        return true;
      case "cozmoDASWarn":
      case "cozmoDASError":
        // Use for recording error in DAS from JavaScript
        string sanitizedString = scratchRequest.argString2;
        if (DoesStringContainPIIRisk(sanitizedString)) {
          sanitizedString = PrivacyGuard.HidePersonallyIdentifiableInfo(sanitizedString);
        }
        if (scratchRequest.command == "cozmoDASError") {
          DAS.Error(scratchRequest.argString, sanitizedString);
        }
        else {
          DAS.Warn(scratchRequest.argString, sanitizedString);
        }
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

    private bool DoesStringContainPIIRisk(string input) {
      for (int i = 0; i < BlocksWithPIIRisks.Count; ++i) {
        if (input.Contains(BlocksWithPIIRisks[i])) {
          return true;
        }
      }
      return false;
    }

    private void WebViewCallback(string jsonStringFromJS) {
#if ANKI_DEV_CHEATS
      _PerformanceStats.AddWebViewCall();
#endif // ANKI_DEV_CHEATS

      ScratchRequest scratchRequest = null;
      try {
        //DAS.Info("CodeLabGame.WebViewCallback.Data", "WebViewCallback - JSON from JavaScript: " + jsonStringFromJS);

        scratchRequest = JsonConvert.DeserializeObject<ScratchRequest>(jsonStringFromJS, GlobalSerializerSettings.JsonSettings);
      }
      catch (Exception exception) {
        if (exception is JsonReaderException || exception is JsonSerializationException) {
          // Note that prior to WebViewCallback being called, WebViewObject.CallFromJS() calls WWW.UnEscapeURL(), unencoding the jsonStringFromJS.
          string logJSONStringFromJS = jsonStringFromJS;

          if (DoesStringContainPIIRisk(logJSONStringFromJS)) {
            // TODO Temporary solution for removing PII from logs.
            // For vertical will need to check more than just CozmoSays,
            // potentially. Also, ideally only strip out CozmoSays
            // payload, not entire JSON string.
            logJSONStringFromJS = PrivacyGuard.HidePersonallyIdentifiableInfo(logJSONStringFromJS);
          }

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
        string safeArgString = PrivacyGuard.HidePersonallyIdentifiableInfo(scratchRequest.argString);
        DAS.Info("CodeLab.QueueScratchReq", "command = '" + scratchRequest.command + "', id = " + scratchRequest.requestId + ", argString = " + safeArgString);
        _queuedScratchRequests.Enqueue(scratchRequest);
      }
      else {
        HandleBlockScratchRequest(scratchRequest);
      }
    }

    private uint TurnInPlace(float turnAngle, RobotCallback callback) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      //DAS.Info("CodeLab.TurnInPlace.Start", "Turn " + (finalTurnAngle * Mathf.Rad2Deg) + "d from " + (robot.PoseAngle * Mathf.Rad2Deg) + "d");
      return robot.TurnInPlace(turnAngle, 0.0f, 0.0f, kToleranceAngle, callback);
    }

    private uint TurnInPlaceVertical(float turnAngle_deg, float speed_deg_per_sec, RobotCallback callback) {
      float finalTurnAngle = turnAngle_deg * Mathf.Deg2Rad;
      float speed_rad_per_sec = speed_deg_per_sec * Mathf.Deg2Rad;
      float accel_rad_per_sec2 = 0.0f;
      float toleranceAngle = ((Math.Abs(turnAngle_deg) > 25.0f) ? 10.0f : 5.0f) * Mathf.Deg2Rad;
      var robot = RobotEngineManager.Instance.CurrentRobot;
      return robot.TurnInPlace(finalTurnAngle, speed_rad_per_sec, accel_rad_per_sec2, toleranceAngle, callback, QueueActionPosition.IN_PARALLEL);
    }

    private byte GetDrawColor() {
      return _SessionState.GetProgramState().GetDrawColor();
    }

    private float GetDrawTextScale() {
      return _SessionState.GetProgramState().GetDrawTextScale();
    }

    private AlignmentX GetDrawTextAlignmentX() {
      return _SessionState.GetProgramState().GetDrawTextAlignmentX();
    }

    private AlignmentY GetDrawTextAlignmentY() {
      return _SessionState.GetProgramState().GetDrawTextAlignmentY();
    }

    private bool HandleDrawOnFaceRequest(ScratchRequest scratchRequest) {
      switch (scratchRequest.command) {
      case "cozVertCozmoFaceClear":
        _CozmoFaceDisplay.ClearScreen();
        return true;
      case "cozVertCozmoFaceDisplay":
        _CozmoFaceDisplay.Display();
        return true;
      case "cozVertCozmoFaceDrawLine": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          float x2 = scratchRequest.argFloat3;
          float y2 = scratchRequest.argFloat4;
          _CozmoFaceDisplay.DrawLine(x1, y1, x2, y2, GetDrawColor());
          return true;
        }
      case "cozVertCozmoFaceFillRect": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          float x2 = scratchRequest.argFloat3;
          float y2 = scratchRequest.argFloat4;
          _CozmoFaceDisplay.FillRect(x1, y1, x2, y2, GetDrawColor());
          return true;
        }
      case "cozVertCozmoFaceDrawRect": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          float x2 = scratchRequest.argFloat3;
          float y2 = scratchRequest.argFloat4;
          _CozmoFaceDisplay.DrawRect(x1, y1, x2, y2, GetDrawColor());
          return true;
        }
      case "cozVertCozmoFaceFillCircle": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          float radius = scratchRequest.argFloat3;
          _CozmoFaceDisplay.FillCircle(x1, y1, radius, GetDrawColor());
          return true;
        }
      case "cozVertCozmoFaceDrawCircle": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          float radius = scratchRequest.argFloat3;
          _CozmoFaceDisplay.DrawCircle(x1, y1, radius, GetDrawColor());
          return true;
        }
      case "cozVertCozmoFaceDrawText": {
          float x1 = scratchRequest.argFloat;
          float y1 = scratchRequest.argFloat2;
          string text = scratchRequest.argString;
          _CozmoFaceDisplay.DrawText(x1, y1, GetDrawTextScale(), GetDrawTextAlignmentX(), GetDrawTextAlignmentY(), text, GetDrawColor());
          return true;
        }
      case "cozVertCozmoFaceSetDrawColor": {
          byte drawColor = scratchRequest.argBool ? (byte)1 : (byte)0;
          _SessionState.GetProgramState().SetDrawColor(drawColor);
          return true;
        }
      case "cozVertCozmoFaceSetTextScale": {
          float drawScale = ClampPercentageInput(scratchRequest.argFloat, 10000.0f) * 0.01f; // value from JS is a percentage
          _SessionState.GetProgramState().SetDrawTextScale(drawScale);
          return true;
        }
      case "cozVertCozmoFaceSetTextAlignment": {
          _SessionState.GetProgramState().SetDrawTextAlignment(scratchRequest.argUInt, scratchRequest.argUInt2);
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
        _SessionState.InstantScratchBlockEvent();
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozVertPathOffset") {
        float offsetX = ClampDistanceInput(scratchRequest.argFloat);
        float offsetY = -ClampDistanceInput(scratchRequest.argFloat2); // inverted as we flip the y axis
        float offsetAngle = ClampAngleInput(scratchRequest.argFloat3) * Mathf.Deg2Rad;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(offsetX.ToString() + " , " + offsetY.ToString() + " , " + offsetAngle.ToString()));
        // Offset is in current robot space, so rotate
        float currentAngle = robot.PoseAngle;
        float cosAngle = Mathf.Cos(currentAngle);
        float sinAngle = Mathf.Sin(currentAngle);
        float newX = robot.WorldPosition.x + cosAngle * offsetX - sinAngle * offsetY;
        float newY = robot.WorldPosition.y + sinAngle * offsetX + cosAngle * offsetY;
        float newAngle = robot.PoseAngle - offsetAngle; // Subtract offset as Angle / Yaw is inverted for CodeLab (so that positive angle turns right)
        bool level = false;
        bool useManualSpeed = false;
        // Cancel any current driving actions, and any wheel motor usage, so that this new action can run
        InProgressScratchBlockPool.CancelActionsOfType(ActionType.Drive);
        robot.DriveWheels(0.0f, 0.0f);
        uint idTag = robot.GotoPose(newX, newY, newAngle, level, useManualSpeed, inProgressScratchBlock.AdvanceToNextBlock, QueueActionPosition.IN_PARALLEL);
        inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
      }
      else if (scratchRequest.command == "cozVertPathTo") {
        float newX = ClampDistanceInput(scratchRequest.argFloat);
        float newY = -ClampDistanceInput(scratchRequest.argFloat2); // inverted as we flip the y axis
        float newAngle = -ClampAngleInput(scratchRequest.argFloat3) * Mathf.Deg2Rad; // Angle / Yaw is inverted for CodeLab (so that positive angle turns right)
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(newX.ToString() + " , " + newY.ToString() + " , " + newAngle.ToString()));
        bool level = false;
        bool useManualSpeed = false;
        // Cancel any current driving actions, and any wheel motor usage, so that this new action can run
        InProgressScratchBlockPool.CancelActionsOfType(ActionType.Drive);
        robot.DriveWheels(0.0f, 0.0f);
        uint idTag = robot.GotoPose(newX, newY, newAngle, level, useManualSpeed, inProgressScratchBlock.AdvanceToNextBlock, QueueActionPosition.IN_PARALLEL);
        inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
      }
      else if (scratchRequest.command == "cozVertHeadAngle") {
        float angle = ClampHeadAngleInput(scratchRequest.argFloat) * Mathf.Deg2Rad;
        float speed = ClampPositiveAngularSpeedInput(scratchRequest.argFloat2) * Mathf.Deg2Rad;
        float accel = -1.0f;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(angle.ToString() + " , " + speed.ToString()));
        // Cancel any current head actions, and any head motor usage, so that this new action can run
        InProgressScratchBlockPool.CancelActionsOfType(ActionType.Head);
        robot.DriveHead(0.0f);
        uint idTag = SetHeadAngleLazy(angle, inProgressScratchBlock.AdvanceToNextBlock, QueueActionPosition.IN_PARALLEL, speed, accel);
        if (idTag == (uint)ActionConstants.INVALID_TAG) {
          inProgressScratchBlock.AdvanceToNextBlock(true);
        }
        else {
          inProgressScratchBlock.SetActionData(ActionType.Head, idTag);
        }
      }
      else if (scratchRequest.command == "cozVertLiftHeight") {
        float liftHeight = ClampPercentageInput(scratchRequest.argFloat) * 0.01f;  // lift height comes in as a percentage
        float speed = ClampPositiveAngularSpeedInput(scratchRequest.argFloat2) * Mathf.Deg2Rad;
        float accel = -1.0f;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(liftHeight.ToString() + " , " + speed.ToString()));
        // Cancel any current lift actions, and any lift motor usage, so that this new action can run
        InProgressScratchBlockPool.CancelActionsOfType(ActionType.Lift);
        robot.MoveLift(0.0f);
        uint idTag = SetLiftHeightLazy(liftHeight, inProgressScratchBlock.AdvanceToNextBlock, QueueActionPosition.IN_PARALLEL, speed, accel);
        if (idTag == (uint)ActionConstants.INVALID_TAG) {
          inProgressScratchBlock.AdvanceToNextBlock(true);
        }
        else {
          inProgressScratchBlock.SetActionData(ActionType.Lift, idTag);
        }
      }
      else if (scratchRequest.command == "cozVertMoveLift") {
        float speed = ClampAngularSpeedInput(scratchRequest.argFloat) * Mathf.Deg2Rad;
        _SessionState.InstantScratchBlockEvent();
        // Cancel any current lift actions, so that the motors can be driven directly
        InProgressScratchBlockPool.CancelActionsOfType(ActionType.Lift);
        robot.MoveLift(speed);
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozVertTurn") {
        float angle = ClampAngleInput(scratchRequest.argFloat);
        float speed = ClampPositiveAngularSpeedInput(scratchRequest.argFloat2);
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(angle.ToString() + " , " + speed.ToString()));
        // Cancel any current driving actions, and any wheel motor usage, so that this new action can run
        InProgressScratchBlockPool.CancelActionsOfType(ActionType.Drive);
        robot.DriveWheels(0.0f, 0.0f);
        // We invert the angle here, so that Turn(90) is to the right, instead of the left, because
        // non-roboticists are more likely to assume positive=clockwise rotation
        uint idTag = TurnInPlaceVertical(-angle, speed, inProgressScratchBlock.CompletedTurn);
        inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
      }
      else if (scratchRequest.command == "cozVertPlaySoundEffects") {
        int soundToPlay = scratchRequest.argInt;
        bool waitToComplete = scratchRequest.argBool;
        Anki.AudioMetaData.GameEvent.Codelab audioEvent = this.GetAudioEvent(soundToPlay, true);
        string eventName = scratchRequest.command + (waitToComplete ? "AndWait" : "");

        if (audioEvent == Anki.AudioMetaData.GameEvent.Codelab.Invalid) {
          // Invalid sound - do nothing
          inProgressScratchBlock.AdvanceToNextBlock(true);
          _SessionState.ScratchBlockEvent(eventName + ".error", DASUtil.FormatExtraData(soundToPlay.ToString()));
        }
        else {
          _SessionState.ScratchBlockEvent(eventName, DASUtil.FormatExtraData(audioEvent.ToString()));

          Anki.AudioEngine.Multiplexer.AudioCallbackFlag callbackFlag = Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventNone;
          CallbackHandler completeHandler = null;
          if (waitToComplete) {
            callbackFlag = Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventComplete;
            completeHandler = inProgressScratchBlock.AudioPlaySoundCompleted;
          }

          // Special case for all orchestra loops - when we start any of them, we start a special event so that all
          // tracks play in parallel together.
          if (IsOrchestraPlayEvent(audioEvent) && !_IsOrchestraPlaying) {
            DAS.Info("CodeLab.Orchestra.Start", "Playing Init event");
            GameAudioClient.PostCodeLabEvent(Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Init);
            _IsOrchestraPlaying = true;
          }

          ushort playID = GameAudioClient.PostCodeLabEvent(audioEvent, callbackFlag, completeHandler);
          DAS.Info("CodeLab.Audio.PlayAudioEvent", "Posting Audio event=" + audioEvent.ToString() + " playID=" + playID);

          if (waitToComplete) {
            inProgressScratchBlock.SetAudioPlayID(playID);
          }
          else {
            inProgressScratchBlock.AdvanceToNextBlock(true);
          }
        }
      }
      else if (scratchRequest.command == "cozVertStopSoundEffects") {
        int soundToPlay = scratchRequest.argInt;
        Anki.AudioMetaData.GameEvent.Codelab audioEvent = this.GetAudioEvent(soundToPlay, false);

        _SessionState.InstantScratchBlockEvent();

        if (audioEvent == Anki.AudioMetaData.GameEvent.Codelab.Invalid) {
          // Invalid sound - do nothing
          inProgressScratchBlock.AdvanceToNextBlock(true);
        }
        else {
          GameAudioClient.PostCodeLabEvent(audioEvent,
                                            Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventComplete,
                                            (callbackInfo) => { /* callback */ });
          DAS.Info("CodeLab.Audio.StopSoundEffect", "Posting Audio event=" + audioEvent.ToString());
        }
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozVertDrive") {
        float dist_mm = ClampDistanceInput(scratchRequest.argFloat);
        float speed = ClampPositiveSpeedInput(scratchRequest.argFloat2);
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString() + " , " + speed.ToString()));
        // Cancel any current driving actions, and any wheel motor usage, so that this new action can run
        InProgressScratchBlockPool.CancelActionsOfType(ActionType.Drive);
        robot.DriveWheels(0.0f, 0.0f);
        uint idTag = robot.DriveStraightAction(speed, dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock, QueueActionPosition.IN_PARALLEL);
        inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
      }
      else if (scratchRequest.command == "cozVertDriveWheels") {
        float leftSpeed = ClampSpeedInput(scratchRequest.argFloat);
        float rightSpeed = ClampSpeedInput(scratchRequest.argFloat2);
        _SessionState.InstantScratchBlockEvent();
        // Cancel any current driving actions, so the motors can drive directly
        InProgressScratchBlockPool.CancelActionsOfType(ActionType.Drive);
        robot.DriveWheels(leftSpeed, rightSpeed);
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozVertStopMotor") {
        string motorToStop = scratchRequest.argString;
        _SessionState.InstantScratchBlockEvent();
        switch (motorToStop) {
        case "wheels":
          InProgressScratchBlockPool.CancelActionsOfType(ActionType.Drive);
          robot.DriveWheels(0.0f, 0.0f);
          break;
        case "head":
          InProgressScratchBlockPool.CancelActionsOfType(ActionType.Head);
          robot.DriveHead(0.0f);
          break;
        case "lift":
          InProgressScratchBlockPool.CancelActionsOfType(ActionType.Lift);
          robot.MoveLift(0.0f);
          break;
        case "all":
          InProgressScratchBlockPool.CancelActionsOfType(ActionType.All);
          robot.StopAllMotors();
          break;
        }
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozmoDriveForward") {
        // argFloat represents the number selected from the dropdown under the "drive forward" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        uint idTag = robot.DriveStraightAction(kNormalDriveSpeed_mmps, dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
        inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
      }
      else if (scratchRequest.command == "cozmoDriveForwardFast") {
        // argFloat represents the number selected from the dropdown under the "drive forward fast" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        uint idTag = robot.DriveStraightAction(kFastDriveSpeed_mmps, dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
        inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
      }
      else if (scratchRequest.command == "cozmoDriveBackward") {
        // argFloat represents the number selected from the dropdown under the "drive backward" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        uint idTag = robot.DriveStraightAction(kNormalDriveSpeed_mmps, -dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
        inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
      }
      else if (scratchRequest.command == "cozmoDriveBackwardFast") {
        // argFloat represents the number selected from the dropdown under the "drive backward fast" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        uint idTag = robot.DriveStraightAction(kFastDriveSpeed_mmps, -dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
        inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
      }
      else if (scratchRequest.command == "cozVertEnableAnimationTrack") {
        _SessionState.InstantScratchBlockEvent();
        bool enable = scratchRequest.argBool;
        try {
          AnimTrack animTrack = (AnimTrack)Enum.Parse(typeof(AnimTrack), scratchRequest.argString, ignoreCase: true);
          _SessionState.SetIsAnimTrackEnabled(animTrack, enable);
        }
        catch (ArgumentException) {
          string trackName = PrivacyGuard.HidePersonallyIdentifiableInfo(scratchRequest.argString);
          DAS.Error("CodeLab.EnableAnimTrack.BadAnimTrack", "Unknown track name '" + trackName + "'");
        }
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozVertEnableWaitForActions") {
        _SessionState.InstantScratchBlockEvent();
        bool enable = scratchRequest.argBool;
        _SessionState.SetShouldWaitForActions(enable);
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if ((scratchRequest.command == "cozVertWaitForActions") || (scratchRequest.command == "cozVertCancelActions")) {
        _SessionState.InstantScratchBlockEvent();
        ActionType actionType = ActionType.Count;
        try {
          actionType = (ActionType)Enum.Parse(typeof(ActionType), scratchRequest.argString, ignoreCase: true);
        }
        catch (ArgumentException) {
          string trackName = PrivacyGuard.HidePersonallyIdentifiableInfo(scratchRequest.argString);
          DAS.Error("CodeLab.WaitForActions.BadActionType", "Unknown Action Type '" + trackName + "'");
        }
        if (actionType != ActionType.Count) {
          if (scratchRequest.command == "cozVertWaitForActions") {
            inProgressScratchBlock.SetWaitOnAction(actionType);
          }
          else {
            InProgressScratchBlockPool.CancelActionsOfType(actionType);
            inProgressScratchBlock.AdvanceToNextBlock(true);
          }
        }
        else {
          inProgressScratchBlock.AdvanceToNextBlock(true);
        }
      }
      else if (scratchRequest.command == "cozmoPlayAnimation") {
        // NOTE: This block is called from Horizontal and Vertical!
        bool isVertical = (_SessionState.GetGrammarMode() == GrammarMode.Vertical);
        var animationIndex = scratchRequest.argInt;
        bool wasMystery = (animationIndex == 0);
        AnimationTrigger animationTrigger = GetAnimationTriggerForScratchIndex(animationIndex, isVertical);
        bool shouldIgnoreBodyTrack = !_SessionState.IsAnimTrackEnabled(AnimTrack.Wheels); ;
        bool shouldIgnoreHead = !_SessionState.IsAnimTrackEnabled(AnimTrack.Head);
        bool shouldIgnoreLift = !_SessionState.IsAnimTrackEnabled(AnimTrack.Lift);
        QueueActionPosition queueActionPosition = QueueActionPosition.NOW;
        RobotCallback onCompleteCallback = inProgressScratchBlock.NeutralFaceThenAdvanceToNextBlock;
        if (isVertical) {
          queueActionPosition = QueueActionPosition.IN_PARALLEL;
          onCompleteCallback = inProgressScratchBlock.VerticalOnAnimationComplete;
          // Cancel any current anim (or say which is an animation) actions so that this new action can run
          InProgressScratchBlockPool.CancelActionsOfType(ActionType.Anim);
          InProgressScratchBlockPool.CancelActionsOfType(ActionType.Say);
        }

        if (animationTrigger == AnimationTrigger.Count) {
          // Invalid animation - do nothing
          inProgressScratchBlock.AdvanceToNextBlock(true);
          _SessionState.InstantScratchBlockEvent();
        }
        else {
          string animationName = animationTrigger.ToString();
          _SessionState.ScratchBlockEvent(scratchRequest.command + (wasMystery ? "Mystery" : ""), DASUtil.FormatExtraData(animationName));
          uint idTag = robot.SendAnimationTrigger(animationTrigger, onCompleteCallback, queueActionPosition,
                                     ignoreBodyTrack: shouldIgnoreBodyTrack, ignoreHeadTrack: shouldIgnoreHead, ignoreLiftTrack: shouldIgnoreLift);
          inProgressScratchBlock.SetActionData(ActionType.Anim, idTag);
          _RequiresResetToNeutralFace = true;
        }
      }
      else if ((scratchRequest.command == "cozVertPlayNamedAnim") || (scratchRequest.command == "cozVertPlayNamedTriggerAnim")) {
        // These are dev/prototyping only blocks while we figure out the list of animations to expose
        bool ignoreBodyTrack = !_SessionState.IsAnimTrackEnabled(AnimTrack.Wheels);
        bool ignoreHeadTrack = !_SessionState.IsAnimTrackEnabled(AnimTrack.Head);
        bool ignoreLiftTrack = !_SessionState.IsAnimTrackEnabled(AnimTrack.Lift);
        bool startedAnim = false;
        // Cancel any current anim (or say which is an animation) actions so that this new action can run
        InProgressScratchBlockPool.CancelActionsOfType(ActionType.Anim);
        InProgressScratchBlockPool.CancelActionsOfType(ActionType.Say);
        if (scratchRequest.command == "cozVertPlayNamedTriggerAnim") {
          try {
            AnimationTrigger animationTrigger = (AnimationTrigger)Enum.Parse(typeof(AnimationTrigger), scratchRequest.argString);

            _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(animationTrigger.ToString()));

            uint idTag = robot.SendAnimationTrigger(animationTrigger, inProgressScratchBlock.VerticalOnAnimationComplete, QueueActionPosition.IN_PARALLEL,
                                                   ignoreBodyTrack: ignoreBodyTrack, ignoreHeadTrack: ignoreHeadTrack, ignoreLiftTrack: ignoreLiftTrack);
            inProgressScratchBlock.SetActionData(ActionType.Anim, idTag);
            startedAnim = true;
          }
          catch (ArgumentException) {
            string trackName = PrivacyGuard.HidePersonallyIdentifiableInfo(scratchRequest.argString);
            DAS.Warn("CodeLab.InvalidTrigger", "Failed to convert '" + trackName + "' to AnimationTrigger");
          }
        }
        else {
          // Unity doesn't currently maintain a list of possible animations, so we just blindly request it
          // if animation doesn't exist, then nothing will play, and block should complete next frame.
          string animationName = scratchRequest.argString;

          _SessionState.ScratchBlockEvent(scratchRequest.command); // deliberately don't pass animationName in as we don't know if it's a valid animation or e.g. PII

          uint idTag = robot.SendQueueSingleAction(Singleton<PlayAnimation>.Instance.Initialize(1, animationName, ignoreBodyTrack, ignoreHeadTrack, ignoreLiftTrack),
                                                inProgressScratchBlock.VerticalOnAnimationComplete, QueueActionPosition.IN_PARALLEL);
          inProgressScratchBlock.SetActionData(ActionType.Anim, idTag);
          startedAnim = true;
        }

        if (startedAnim) {
          _RequiresResetToNeutralFace = true;
        }
        else {
          inProgressScratchBlock.AdvanceToNextBlock(false);
        }
      }
      else if (scratchRequest.command == "cozmoTurnLeft") {
        // Turn 90 degrees to the left
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(kTurnAngle.ToString()));
        uint idTag = TurnInPlace(kTurnAngle, inProgressScratchBlock.CompletedTurn);
        inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
      }
      else if (scratchRequest.command == "cozmoTurnRight") {
        // Turn 90 degrees to the right
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData((-kTurnAngle).ToString()));
        uint idTag = TurnInPlace(-kTurnAngle, inProgressScratchBlock.CompletedTurn);
        inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
      }
      else if (scratchRequest.command == "cozmoSays") {
        // NOTE: This block is called from Horizontal and Vertical!
        bool isVertical = (_SessionState.GetGrammarMode() == GrammarMode.Vertical);
        QueueActionPosition queueActionPosition = QueueActionPosition.NOW;
        if (isVertical) {
          queueActionPosition = QueueActionPosition.IN_PARALLEL;
          // Cancel any current anim (or say which is an animation) actions so that this new action can run
          InProgressScratchBlockPool.CancelActionsOfType(ActionType.Anim);
          InProgressScratchBlockPool.CancelActionsOfType(ActionType.Say);
        }

        // Clean the Cozmo Says text input using the same process as Cozmo Says minigame
        string cozmoSaysText = scratchRequest.argString;
        string cozmoSaysTextCleaned = RemoveUnsupportedChars(cozmoSaysText);
        bool hasBadWords = BadWordsFilterManager.Instance.Contains(cozmoSaysTextCleaned);
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(hasBadWords.ToString()));  // deliberately don't send string as it's PII
        uint idTag;
        if (hasBadWords) {
          idTag = robot.SendAnimationTrigger(AnimationTrigger.CozmoSaysBadWord, inProgressScratchBlock.AdvanceToNextBlock, queueActionPosition);
        }
        else {
          idTag = robot.SayTextWithEvent(cozmoSaysTextCleaned, AnimationTrigger.Count, callback: inProgressScratchBlock.AdvanceToNextBlock, queueActionPosition: queueActionPosition);
        }
        inProgressScratchBlock.SetActionData(ActionType.Say, idTag);
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

        uint idTag = SetHeadAngleLazy(desiredHeadAngle, inProgressScratchBlock.AdvanceToNextBlock);
        if (idTag == (uint)ActionConstants.INVALID_TAG) {
          inProgressScratchBlock.AdvanceToNextBlock(true);
        }
        else {
          inProgressScratchBlock.SetActionData(ActionType.Head, idTag);
        }
      }
      else if (scratchRequest.command == "cozmoDockWithCube") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        float desiredHeadAngle = CozmoUtil.HeadAngleFactorToRadians(CozmoUtil.kIdealBlockViewHeadValue, false);
        uint idTag = SetHeadAngleLazy(desiredHeadAngle, inProgressScratchBlock.DockWithCube);
        if (idTag == (uint)ActionConstants.INVALID_TAG) {
          // Trigger a very short wait action first instead to ensure callbacks happen after this method exits
          idTag = robot.WaitAction(0.01f, callback: inProgressScratchBlock.DockWithCube);
        }

        inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
      }
      else if (scratchRequest.command == "cozVertDockWithCubeById") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        uint cubeIndex = scratchRequest.argUInt;
        LightCube cubeToDockWith = robot.GetLightCubeWithObjectType(GetLightCubeIdFromIndex(cubeIndex));
        // Cancel any current driving actions, and any wheel motor usage, so that this new action can run
        InProgressScratchBlockPool.CancelActionsOfType(ActionType.Drive);
        robot.DriveWheels(0.0f, 0.0f);
        if ((cubeToDockWith != null)) {
          uint idTag = robot.AlignWithObject(cubeToDockWith, 0.0f, callback: inProgressScratchBlock.AdvanceToNextBlock, usePreDockPose: true, alignmentType: Anki.Cozmo.AlignmentType.LIFT_PLATE, queueActionPosition: QueueActionPosition.IN_PARALLEL, numRetries: 2);
          inProgressScratchBlock.SetActionData(ActionType.Drive, idTag);
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
        uint idTag = SetLiftHeightLazy(liftHeight, inProgressScratchBlock.AdvanceToNextBlock);
        if (idTag == (uint)ActionConstants.INVALID_TAG) {
          inProgressScratchBlock.AdvanceToNextBlock(true);
        }
        else {
          inProgressScratchBlock.SetActionData(ActionType.Lift, idTag);
        }
      }
      else if (scratchRequest.command == "cozmoSetBackpackColor") {
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(scratchRequest.argString));
        robot.SetAllBackpackBarLED(scratchRequest.argUInt);
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozmoVerticalSetBackpackColor") {
        _SessionState.InstantScratchBlockEvent();
        robot.SetAllBackpackBarLED(scratchRequest.argUInt);
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozVertSetCubeLightCorner") {
        _SessionState.InstantScratchBlockEvent();
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
        inProgressScratchBlock.ReleaseFromPool();
      }
      else if (scratchRequest.command == "cozVertCubeAnimation") {
        uint color = scratchRequest.argUInt;
        uint cubeIndex = scratchRequest.argUInt2;
        var cubeToAnimate = robot.GetLightCubeWithObjectType(GetLightCubeIdFromIndex(cubeIndex));
        if (cubeToAnimate != null) {
          string cubeAnim = scratchRequest.argString;
          string dasCubeAnimName = cubeAnim;
          cubeToAnimate.SetLEDsOff();
          switch (cubeAnim) {
          case "spin":
            Color[] colorArray = { color.ToColor(), Color.black, Color.black, Color.black };
            StartCycleCube(cubeToAnimate.ID, colorArray, 0.05f);
            break;
          case "blink":
            cubeToAnimate.SetFlashingLEDs(color.ToColor());
            break;
          default:
            dasCubeAnimName = "invalid";
            break;
          }

          _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dasCubeAnimName));
        }
        else {
          DAS.Info("CodeLab.NullCube", "No connected cube with index " + cubeIndex.ToString());
        }
        inProgressScratchBlock.ReleaseFromPool();
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

      if (inProgressScratchBlock.HasActiveRequestPromise() && inProgressScratchBlock.MatchesActionType(ActionType.All) && !_SessionState.ShouldWaitForActions()) {
        // Tell Scratch to continue immediately (leave block active (don't release it) so we can still later query running actions)
        inProgressScratchBlock.ResolveRequestPromise();
      }

      return;
    }

    private bool IsOrchestraPlayEvent(Anki.AudioMetaData.GameEvent.Codelab audioEvent) {
      switch (audioEvent) {
      case Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Bass_Mode_1:
      case Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Bass_Mode_2:
      case Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Bass_Mode_3:
      case Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Glock_Pluck_Mode_1:
      case Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Glock_Pluck_Mode_2:
      case Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Glock_Pluck_Mode_3:
      case Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Strings_Mode_1:
      case Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Strings_Mode_2:
      case Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Strings_Mode_3:
        return true;
      default:
        return false;
      }
    }

    private Anki.AudioMetaData.GameEvent.Codelab GetAudioEvent(int soundToPlay, bool isStartSound) {
      Anki.AudioMetaData.GameEvent.Codelab audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Invalid;
      switch (soundToPlay) {
      case 1: // BKY_EIGHTIES_MUSIC
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Style_80S_1_159Bpm_Loop;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Style_80S_1_159Bpm_Loop_Stop;
        }
        break;
      case 2: // BKY_MAMBO_MUSIC
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Style_Mambo_1_183Bpm_Loop;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Style_Mambo_1_183Bpm_Loop_Stop;
        }
        break;
      case 3: // BKY_SOUND_DISCO_MUSIC
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Style_Disco_1_135Bpm_Loop;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Style_Disco_1_135Bpm_Loop_Stop;
        }
        break;
      case 4: // BKY_SOUND_MAGIC
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Magic8_Reveal_Stinger;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Magic8_Reveal_Stinger_Stop;
        }
        break;
      case 5: // BKY_WIN
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Game_Win;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Game_Win_Stop;
        }
        break;
      case 6: // BKY_LOSE
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Game_Lose;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Game_Lose_Stop;
        }
        break;
      case 7: // BKY_GAME_START
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Countdown;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Countdown_Stop;
        }
        break;
      case 8: // BKY_CLOCK_TICK
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Timer_Click;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Timer_Click_Stop;
        }
        break;
      case 9: // BKY_BLING
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Cube_Light_On;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Cube_Light_On_Stop;
        }
        break;
      case 10: // BKY_SUCCESS
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Success;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Success_Stop;
        }
        break;
      case 11: // BKY_FAIL
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Error;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Error_Stop;
        }
        break;
      case 12: // BKY_TIMER_WARNING
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Timer_Warning;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Timer_Warning_Stop;
        }
        break;
      case 13: // BKY_TIMER_END
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Timer_End;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Shared_Timer_End_Stop;
        }
        break;
      case 14: // BKY_SPARKLE
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Magic8_Message_Reveal;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Magic8_Message_Reveal_Stop;
        }
        break;
      case 15: // BKY_SWOOSH
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Hot_Potato_Pass;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Hot_Potato_Pass_Stop;
        }
        break;
      case 16: // BKY_PING
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Hot_Potato_Cube_Ready;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Hot_Potato_Cube_Ready_Stop;
        }
        break;
      case 17: // BKY_HOT_POTATO_END
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Hot_Potato_Timer_End;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Hot_Potato_Timer_End_Stop;
        }
        break;
      case 18: // BKY_HOT_POTATO_MUSIC_SLOW
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Hot_Potato_Level_1_Loop;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Hot_Potato_Level_1_Loop_Stop;
        }
        break;
      case 19: // BKY_HOT_POTATO_MUSIC_MEDIUM
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Hot_Potato_Level_2_Loop;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Hot_Potato_Level_2_Loop_Stop;
        }
        break;
      case 20: // BKY_HOT_POTATO_MUSIC_FAST
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Hot_Potato_Level_3_Loop;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Hot_Potato_Level_3_Loop_Stop;
        }
        break;
      case 21: // BKY_HOT_POTATO_MUSIC_SUPERFAST
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Hot_Potato_Level_4_Loop;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Hot_Potato_Level_4_Loop_Stop;
        }
        break;
      case 22: // BKY_MAGNET_PULL
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Magnet_Attract;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Magnet_Attract_Stop;
        }
        break;
      case 23: // BKY_MAGNET_REPEL
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Magnet_Repel;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Magnet_Repel_Stop;
        }
        break;
      case 24: // BKY_INSTRUMENT_1_MODE_1
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Bass_Mode_1;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Bass_Mode_1_Stop;
        }
        break;
      case 25: // BKY_INSTRUMENT_1_MODE_2
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Bass_Mode_2;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Bass_Mode_2_Stop;
        }
        break;
      case 26: // BKY_INSTRUMENT_1_MODE_3
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Bass_Mode_3;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Bass_Mode_3_Stop;
        }
        break;
      case 27: // BKY_INSTRUMENT_2_MODE_1
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Glock_Pluck_Mode_1;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Glock_Pluck_Mode_1_Stop;
        }
        break;
      case 28: // BKY_INSTRUMENT_2_MODE_2
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Glock_Pluck_Mode_2;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Glock_Pluck_Mode_2_Stop;
        }
        break;
      case 29: // BKY_INSTRUMENT_2_MODE_3
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Glock_Pluck_Mode_3;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Glock_Pluck_Mode_3_Stop;
        }
        break;
      case 30: // BKY_INSTRUMENT_3_MODE_1
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Strings_Mode_1;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Strings_Mode_1_Stop;
        }
        break;
      case 31: // BKY_INSTRUMENT_3_MODE_2
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Strings_Mode_2;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Strings_Mode_2_Stop;
        }
        break;
      case 32: // BKY_INSTRUMENT_3_MODE_3
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Strings_Mode_3;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Tiny_Orchestra_Strings_Mode_3_Stop;
        }
        break;
      case 33: // BKY_SOUND_TIMER_START
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Firetruck_Timer_Start;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Firetruck_Timer_Start_Stop;
        }
        break;
      case 34: // BKY_SOUND_FIRE_TRUCK_ALARM
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Firetruck_Timer_End;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Firetruck_Timer_End_Stop;
        }
        break;
      case 35: // BKY_SOUND_CUBE_CHARGE
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Hot_Potato_Cube_Charge;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Sfx_Hot_Potato_Cube_Charge_Stop;
        }
        break;
      case 36: // BKY_SOUND_BACKGROUND_MUSIC
        if (isStartSound) {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Background_Silence_Off;
        }
        else {
          audioEvent = Anki.AudioMetaData.GameEvent.Codelab.Music_Background_Silence_On;
        }
        break;
      default:
        // Will happen a lot in vertical now that users can pass in any value
        DAS.Info("CodeLab.BadSoundIndex", "index = " + soundToPlay.ToString());
        break;
      }

      return audioEvent;
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
        // This is now quite likely as users can set the index
        DAS.Info("CodeLab.BadCubeIndex", "cubeIndex: " + cubeIndex.ToString());
        return ObjectType.UnknownObject;
      }
    }

    private int _LastBadCubeId = -999; // Keep track of this to avoid spamming on every vision frame

    private ObjectType GetLightCubeIndexFromId(int objectID, bool warnIfCharger = true) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      var cube1 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE1);
      var cube2 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE2);
      var cube3 = robot.GetLightCubeWithObjectType(ObjectType.Block_LIGHTCUBE3);
      if (cube1 != null && objectID == cube1.ID) {
        return ObjectType.Block_LIGHTCUBE1;
      }
      else if (cube2 != null && objectID == cube2.ID) {
        return ObjectType.Block_LIGHTCUBE2;
      }
      else if (cube3 != null && objectID == cube3.ID) {
        return ObjectType.Block_LIGHTCUBE3;
      }
      else {
        var charger = robot.GetLightCubeWithObjectType(ObjectType.Charger_Basic);
        if (charger != null && objectID == charger.ID) {
          if (warnIfCharger) {
            DAS.Error("CodeLab.ChargerCubeId", "objectID=" + objectID);
          }
        }
        else {
          // Only report bad cube IDs once (otherwise it would spam on every tick)
          if (objectID != _LastBadCubeId) {
            ActiveObject activeObject = robot.GetActiveObjectById(objectID);
            if (activeObject != null) {
              DAS.Warn("CodeLab.BadCubeId.ObjectInfo", "objectID=" + objectID + " family=" + activeObject.Family + " type=" + activeObject.ObjectType);
            }
            else {
              DAS.Warn("CodeLab.BadCubeId.NullObject", "objectID=" + objectID);
            }
            _LastBadCubeId = objectID;
          }
        }

        return ObjectType.UnknownObject;
      }
    }

    private void OpenCodeLabProject(RequestToOpenProjectOnWorkspace request, string projectUUID, bool isVertical) {
      PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;

      DAS.Info("Codelab.OpenCodeLabProject", "request=" + request + ", UUID=" + projectUUID);
      // Cache the request to open project. These vars will be used after the webview is loaded but before it is visible.
      SetRequestToOpenProject(request, projectUUID);
      ShowGettingReadyScreen();

      Dictionary<string, string> parameters = new Dictionary<string, string>();

      if (!isVertical) {
        _SessionState.SetGrammarMode(GrammarMode.Horizontal);

        if (defaultProfile.CodeLabHorizontalPlayed == 0
            && !DebugMenuManager.Instance.DemoMode) {
          parameters.Add("showTutorial", "true");
          defaultProfile.CodeLabHorizontalPlayed = 1;
        }
        else {
          parameters.Add("showTutorial", "false");
        }
        LoadURL(kHorizontalIndexFilename, parameters);
      }
      else {
        _SessionState.SetGrammarMode(GrammarMode.Vertical);

        if (request == RequestToOpenProjectOnWorkspace.DisplayFeaturedProject) {
          parameters.Add("isFeaturedProject", "true");
        }
        else {
          parameters.Add("isFeaturedProject", "false");
        }

        LoadURL(kVerticalIndexFilename, parameters);
      }

      SetupCubeLights();
    }

    private void SetupCubeLights() {
      // Freeplay lights are enabled in Horizontal, but in Vertical we want user to have full control
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        if (_SessionState.GetGrammarMode() == GrammarMode.Vertical) {
          robot.EnableCubeSleep(true, true);
          robot.SetEnableFreeplayLightStates(false);
        }
        else {
          robot.SetEnableFreeplayLightStates(true);
          robot.EnableCubeSleep(false);
        }
      }
    }

    private void OnCozmoRenameCodeLabProject(ScratchRequest scratchRequest) {
      string projectUUID = scratchRequest.argUUID;
      string jsCallback = scratchRequest.argString;
      string newProjectName = scratchRequest.argString2;

      CodeLabProject projectToUpdate = null;
      try {
        projectToUpdate = FindUserProjectWithUUID(scratchRequest.argUUID);
        if (projectToUpdate != null) {
          // Don't let the project name get set to empty string.
          string cleansedName = RemoveUnsupportedChars(newProjectName);

          // Check that the entire name is not just spaces.
          bool allCharsAreSpaces = true;
          for (int i = 0; i < cleansedName.Length; i++) {
            char currentChar = cleansedName[i];
            if (currentChar != ' ') {
              allCharsAreSpaces = false;
              break;
            }
          }

          if (cleansedName != "" && !allCharsAreSpaces) {
            SessionState.DAS_Event("robot.code_lab.rename_project", cleansedName.Length.ToString(), SessionState.DAS_Format_IsVertical(projectToUpdate.IsVertical));
            projectToUpdate.ProjectName = cleansedName;
            projectToUpdate.DateTimeLastModifiedUTC = DateTime.UtcNow;
          }

          string escapedName = EscapeProjectText(projectToUpdate.ProjectName);
          this.EvaluateJS(jsCallback + "('" + escapedName + "');");

          _SessionState.OnUpdatedProject(projectToUpdate);

          DataPersistenceManager.Instance.Save();
        }
      }
      catch (NullReferenceException) {
        DAS.Error("RenameCodeLabProject.NullReferenceException", "Failure during servicing user's request to rename project. projectUUID = " + projectUUID);
      }
    }

    private void OnCozmoRemixCodeLabProject(ScratchRequest scratchRequest) {
      string newProjectName = scratchRequest.argString;
      string originalProjectType = scratchRequest.argString2; // "user" or "sample"
      string originalProjectUUID = scratchRequest.argUUID;

      try {
        CodeLabProject remixedProject = FindProjectByUUIDAndCreateCodeLabProject(originalProjectUUID, originalProjectType);

        string baseDASProjectName = remixedProject.BaseDASProjectName ?? "";
        if (remixedProject.IsVertical) {
          SessionState.DAS_Event("robot.code_lab.remix_vertical_project", originalProjectType, DASUtil.FormatExtraData(baseDASProjectName));
        }
        else {
          SessionState.DAS_Event("robot.code_lab.remix_horizontal_project", originalProjectType, DASUtil.FormatExtraData(baseDASProjectName));
        }

        remixedProject.ProjectName = newProjectName;
        _SessionState.SetCurrentProjectData(ProjectType.User, baseDASProjectName);

        PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;
        if (defaultProfile == null) {
          DAS.Error("OnCozmoSaveUserProject.NullDefaultProfile", "In saving remixed Code Lab user project, defaultProfile is null");
        }
        if (defaultProfile.CodeLabProjects == null) {
          DAS.Error("OnCozmoSaveUserProject.NullCodeLabProjects", "defaultProfile.CodeLabProjects is null");
        }
        defaultProfile.CodeLabProjects.Add(remixedProject);

        if (originalProjectType == "sample") {
          if (remixedProject.IsVertical) {
            _LastOpenedTab = "vertical";
          }
          else {
            _LastOpenedTab = "horizontal";
          }
        }

        // Was this the user's first remix? Show dialog if so.
        bool isFirstRemix = false;
        if (!defaultProfile.CodeLabRemixCreated) {
          isFirstRemix = true;
          defaultProfile.CodeLabRemixCreated = true;
        }

        string projectNameEscaped = EscapeProjectText(remixedProject.ProjectName);
        this.EvaluateJS("window.onRemixedProject('" + remixedProject.ProjectUUID + "','" + projectNameEscaped + "','" + isFirstRemix.ToString() + "');");

        _SessionState.OnCreatedProject(remixedProject, true);

        DataPersistenceManager.Instance.Save();
      }
      catch (NullReferenceException) {
        DAS.Error("RenameCodeLabProject.NullReferenceException", "Failure during servicing user's request to rename project. Project UUID to remix = " + originalProjectUUID + ", new project name = " + newProjectName);
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

    private void LoadURL(string scratchPathToHTML, Dictionary<string, string> urlParameters = null) {
      if (_SessionState.IsProgramRunning()) {
        // Program is currently running and we won't receive OnScriptStopped notification
        // when the active page and Javascript is nuked, so manually force an OnScriptStopped event
        DAS.Info("Codelab.ManualOnScriptStopped", "");
        OnScriptStopped();
      }
      ResetAllCodeLabAudio();

      if (_WebViewObjectComponent == null) {
        DAS.Error("CodeLab.LoadURL.NullWebView", "Ignoring request to open '" + scratchPathToHTML + "'");
        return;
      }

      string baseFilename = GetRootPath() + scratchPathToHTML;
      string urlPath = GetUrlPath(baseFilename);

      // Append locale parameter to URL
      string locale = Localization.GetStringsLocale();
      urlPath += "?locale=" + locale;

      if (urlParameters != null) {
        foreach (KeyValuePair<string, string> kvp in urlParameters) {
          urlPath += "&" + kvp.Key + "=" + kvp.Value;
        }
      }

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

      ResetCozmoOnNewWorkspace();
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

    private Anki.Cozmo.AnimationTrigger GetAnimationTriggerForScratchIndex(int scratchAnimationIndex, bool isVertical) {
      if (scratchAnimationIndex == 0) {
        // Special case - pick a random animation
        if (isVertical) {
          int kLastValidVerticalAnimation = 34;
          scratchAnimationIndex = UnityEngine.Random.Range(1, kLastValidVerticalAnimation);
        }
        else {
          int kLastValidHorizontalAnimation = 14;
          scratchAnimationIndex = UnityEngine.Random.Range(1, kLastValidHorizontalAnimation);
        }
      }

      switch (scratchAnimationIndex) {
      case 1: // happy
        return AnimationTrigger.CodeLabHappy;
      case 2: // winner/"victory"
        return AnimationTrigger.CodeLabVictory;
      case 3: // sad/"unhappy":
        return AnimationTrigger.CodeLabUnhappy;
      case 4: // "surprise":
        return AnimationTrigger.CodeLabSurprise;
      case 5: // "dog":
        return AnimationTrigger.CodeLabDog;
      case 6: // "cat":
        return AnimationTrigger.CodeLabCat;
      case 7: // "sneeze":
        return AnimationTrigger.CodeLabSneeze;
      case 8: // "excited":
        return AnimationTrigger.CodeLabExcited;
      case 9: // "thinking":
        return AnimationTrigger.CodeLabThinking;
      case 10: // "bored":
        return AnimationTrigger.CodeLabBored;
      case 11: // "frustrated":
        return AnimationTrigger.CodeLabFrustrated;
      case 12: // "chatty":
        return AnimationTrigger.CodeLabChatty;
      case 13: // disappointed / "dejected":
        return AnimationTrigger.CodeLabDejected;
      case 14: // snore/"sleep":
        return AnimationTrigger.CodeLabSleep;
      case 15:
        return AnimationTrigger.CodeLabReactHappy;
      case 16:
        return AnimationTrigger.CodeLabCelebrate;
      case 17:
        return AnimationTrigger.CodeLabTakaTaka;
      case 18:
        return AnimationTrigger.CodeLabAmazed;
      case 19:
        return AnimationTrigger.CodeLabCurious;
      case 20: // Agree/Yes
        return AnimationTrigger.CodeLabYes;
      case 21: // Disagree/No
        return AnimationTrigger.CodeLabNo;
      case 22: // Unsure/IDK
        return AnimationTrigger.CodeLabIDK;
      case 23:
        return AnimationTrigger.CodeLabConducting;
      case 24:
        return AnimationTrigger.CodeLabDancingMambo;
      case 25:
        return AnimationTrigger.CodeLabFireTruck;
      case 26:
        return AnimationTrigger.CodeLabPartyTime;
      case 27:
        return AnimationTrigger.CodeLabDizzy;
      case 28:
        return AnimationTrigger.CodeLabDizzyEnd;
      case 29:
        return AnimationTrigger.CodeLab123Go;
      case 30:
        return AnimationTrigger.CodeLabWin;
      case 31:
        return AnimationTrigger.CodeLabLose;
      case 32:
        return AnimationTrigger.CodeLabTapCube;
      case 33:
        return AnimationTrigger.CodeLabGetInPos;
      case 34:
        return AnimationTrigger.CodeLabIdle;
      default:
        // Will happen a lot in vertical now that users can pass in any value
        DAS.Info("CodeLab.BadAnimIndex", "Index = " + scratchAnimationIndex.ToString());
        return AnimationTrigger.Count;
      }
    }

    // Returns an existing user CodeLabProject matching the supplied uuid
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
      bool isOpeningProject = false;

      _SessionState.SetCurrentProjectData(ProjectType.User, "");

      switch (_RequestToOpenProjectOnWorkspace) {
      case RequestToOpenProjectOnWorkspace.CreateNewProject:
        // Open workspace and display only a green flag on the workspace.
        this.EvaluateJS("window.ensureGreenFlagIsOnWorkspace();");
        break;

      case RequestToOpenProjectOnWorkspace.DisplayUserProject:
        if (_ProjectUUIDToOpen != null) {
          CodeLabProject projectToOpen = FindUserProjectWithUUID(_ProjectUUIDToOpen);
          if (projectToOpen != null) {
            _SessionState.SetCurrentProjectData(ProjectType.User, projectToOpen.BaseDASProjectName);
            String projectNameEscaped = EscapeProjectText(projectToOpen.ProjectName);
            if (projectToOpen.ProjectJSON != null) {
              SessionState.DAS_Event("robot.code_lab.open_json_user_project", projectToOpen.ProjectJSON.Length.ToString(), SessionState.DAS_Format_IsVertical(projectToOpen.IsVertical));
              OpenCozmoProjectJSON(projectNameEscaped, projectToOpen.ProjectJSON, projectToOpen.ProjectUUID, "false");
            }
            else if (projectToOpen.ProjectXML != null) {
              // User project is in XML. It must have been created before the Cozmo app 2.1 release and is CodeLabProject VersionNum 2 or less.
              SessionState.DAS_Event("robot.code_lab.open_xml_user_project", projectToOpen.ProjectXML.Length.ToString(), SessionState.DAS_Format_IsVertical(projectToOpen.IsVertical));

              String projectXMLEscaped = EscapeXML(projectToOpen.ProjectXML);

              // Open requested project in webview
              this.EvaluateJS("window.openCozmoProjectXML('" + projectToOpen.ProjectUUID + "','" + projectNameEscaped + "',\"" + projectXMLEscaped + "\",'false');");
            }
            else {
              DAS.Error("CodeLab.NoUserProjectSerialized", "No ProjectJSON or ProjectXML found for user project with _ProjectUUIDToOpen = '" + _ProjectUUIDToOpen + "'");
            }
            isOpeningProject = true;
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

          if (codeLabSampleProject != null) {
            SessionState.DAS_Event("robot.code_lab.open_sample_project", codeLabSampleProject.DASProjectName, SessionState.DAS_Format_IsVertical(codeLabSampleProject.IsVertical));
            _SessionState.SetCurrentProjectData(ProjectType.Sample, codeLabSampleProject.DASProjectName);

            String sampleProjectName = null;
            if (codeLabSampleProject.ProjectName != null) {
              sampleProjectName = Localization.Get(codeLabSampleProject.ProjectName);
            }
            String sampleProjectNameEscaped = EscapeProjectText(sampleProjectName);

            if (codeLabSampleProject.ProjectJSON != null) {
              // Sample project is in JSON.
              // Open requested project in webview.

              OpenCozmoProjectJSON(sampleProjectNameEscaped, codeLabSampleProject.ProjectJSON, codeLabSampleProject.ProjectUUID, "true");
            }
            else if (codeLabSampleProject.ProjectXML != null) {
              // Sample project is still in XML. This will be true for some until we convert them all over.
              String projectXMLEscaped = EscapeXML(codeLabSampleProject.ProjectXML);

              // Open requested project in webview
              this.EvaluateJS("window.openCozmoProjectXML('" + codeLabSampleProject.ProjectUUID + "','" + sampleProjectNameEscaped + "',\"" + projectXMLEscaped + "\",'true');");
            }
            else {
              DAS.Error("CodeLab.NoSampleProjectSerialized", "No ProjectJSON or ProjectXML found for sample project with _ProjectUUIDToOpen = '" + _ProjectUUIDToOpen + "'");
            }
            isOpeningProject = true;
          }
        }
        else {
          DAS.Error("CodeLab.SampleProjectUUIDNull", "Sample project with null _ProjectUUIDToOpen");
        }
        break;

      case RequestToOpenProjectOnWorkspace.DisplayFeaturedProject:
        if (_ProjectUUIDToOpen != null) {
          Guid projectGuid = new Guid(_ProjectUUIDToOpen);
          CodeLabFeaturedProject codeLabFeaturedProject = null;

          Predicate<CodeLabFeaturedProject> findProject = (CodeLabFeaturedProject p) => { return p.ProjectUUID == projectGuid; };
          codeLabFeaturedProject = _CodeLabFeaturedProjects.Find(findProject);

          if (codeLabFeaturedProject != null) {
            bool wasLoadedFromWhatsNew = Cozmo.WhatsNew.WhatsNewModalManager.ShouldAutoOpenProject;
            Cozmo.WhatsNew.WhatsNewModalManager.ResetAutoOpenCodeLabProject();

            // featured projects are hardcoded to be vertical currently
            SessionState.DAS_Event("robot.code_lab.open_featured_project", codeLabFeaturedProject.DASProjectName, DASUtil.FormatExtraData(wasLoadedFromWhatsNew ? "1" : "0"));
            _SessionState.SetCurrentProjectData(ProjectType.Featured, codeLabFeaturedProject.DASProjectName);

            if (codeLabFeaturedProject.ProjectJSON != null) {
              String featuredProjectName = null;
              if (codeLabFeaturedProject.ProjectName != null) {
                featuredProjectName = Localization.Get(codeLabFeaturedProject.ProjectName);
              }
              String featuredProjectNameEscaped = EscapeProjectText(featuredProjectName);

              // Open requested project in webview
              OpenCozmoProjectJSON(featuredProjectNameEscaped, codeLabFeaturedProject.ProjectJSON, codeLabFeaturedProject.ProjectUUID, "true");
              isOpeningProject = true;
            }
          }
        }
        else {
          DAS.Error("CodeLab.OpenFeaturedProject.NullProject", "Null UUID");
        }
        break;

      default:
        break;
      }

      SetRequestToOpenProject(RequestToOpenProjectOnWorkspace.DisplayNoProject, null);

      // If we are displaying workspace, give the webview a little time to get ready.
      float delayInSeconds = 0.0f;
      if (cachedRequestToOpenProject != RequestToOpenProjectOnWorkspace.DisplayNoProject) {
#if UNITY_EDITOR || UNITY_IOS
        delayInSeconds = 1.0f;
#elif UNITY_ANDROID
      delayInSeconds = 2.0f;
#endif
      }
      if (isOpeningProject) {
        // Unhide will happen after we receive load success, or in 60 seconds, whichever occurs first
        delayInSeconds = 60.0f;
      }

      Invoke("UnhideWebView", delayInSeconds);

      SetupCubeLights();
    }

    private String EscapeProjectText(String projectText) {
      if (projectText == null) {
        return projectText;
      }

      String tempProjectText = projectText.Replace("\"", "\\\"");
      return tempProjectText.Replace("'", "\\'");
    }

    private String EscapeXML(String xml) {
      if (xml != null) {
        return xml.Replace("\"", "\\\"");
      }
      else {
        return null;
      }
    }

    // Open requested project in webview
    private void OpenCozmoProjectJSON(String projectName, String projectJSON, Guid projectUUID, string isSampleStr) {
      DAS.Info("CodeLabTest", "OpenCozmoProjectJSONUnity: projectUUID = " + projectUUID.ToString() + ", isSampleStr = " + isSampleStr);

      CozmoProjectOpenInWorkspaceRequest cozmoProjectRequest = new CozmoProjectOpenInWorkspaceRequest();
      cozmoProjectRequest.projectName = projectName;
      cozmoProjectRequest.projectJSON = projectJSON;
      cozmoProjectRequest.projectUUID = projectUUID;
      cozmoProjectRequest.isSampleStr = isSampleStr;

      string cozmoProjectSerialized = JsonConvert.SerializeObject(cozmoProjectRequest);

      this.EvaluateJS(@"window.openCozmoProjectJSON(" + cozmoProjectSerialized + ");");
    }

    private void StartVerticalHatBlockListeners() {
      if (!_VerticalHatBlocksStarted) {
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

        _VerticalHatBlocksStarted = true;
      }
    }

    private void StopVerticalHatBlockListeners() {
      if (_VerticalHatBlocksStarted) {
        RobotEngineManager.Instance.RemoveCallback<RobotObservedFace>(RobotObservedFaceVerticalHatBlock);
        RobotEngineManager.Instance.RemoveCallback<RobotObservedObject>(RobotObservedObjectVerticalHatBlock);
        LightCube.TappedAction -= CubeTappedVerticalHatBlock;
        LightCube.OnMovedAction -= CubeMovedVerticalHatBlock;
        _VerticalHatBlocksStarted = false;
      }
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

    public static bool FaceIsSad(RobotObservedFace message) {
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

    private const int kAnyCubeId = 4;

    public void RobotObservedObjectVerticalHatBlock(RobotObservedObject message) {
      ObjectType objectType = GetLightCubeIndexFromId(message.objectID, false);
      if (objectType != ObjectType.UnknownObject) {
        int lightCubeIndex = (int)objectType;
        EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_see_cube', {CUBE_SELECT: \"" + lightCubeIndex + "\"});");
        EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_see_cube', {CUBE_SELECT: \"" + kAnyCubeId + "\"});");
      }
    }

    public void CubeTappedVerticalHatBlock(int id, int tappedTimes, float timeStamp) {
      ObjectType objectType = GetLightCubeIndexFromId(id, false);

      switch (objectType) {
      case ObjectType.Block_LIGHTCUBE1:
        _LatestCozmoState.cube1.framesSinceTapped = 0;
        break;
      case ObjectType.Block_LIGHTCUBE2:
        _LatestCozmoState.cube2.framesSinceTapped = 0;
        break;
      case ObjectType.Block_LIGHTCUBE3:
        _LatestCozmoState.cube3.framesSinceTapped = 0;
        break;
      default:
        DAS.Error("CodeLab.CubeTapped.BadCubeId", "cubeId " + id.ToString());
        return;
      }

      // Must send latest state so that the new tap information is reflected before anything attempts to read it
      // in response to the tap event.
      SendWorldStateToWebView();

      int lightCubeIndex = (int)objectType;
      EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_cube_tap', {CUBE_SELECT: \"" + lightCubeIndex + "\"});");
      EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_cube_tap', {CUBE_SELECT: \"" + kAnyCubeId + "\"});");
    }

    public void CubeMovedVerticalHatBlock(int id, float XAccel, float YAccel, float ZAccel) {
      ObjectType objectType = GetLightCubeIndexFromId(id);
      if (objectType != ObjectType.UnknownObject) {
        int lightCubeIndex = (int)objectType;
        EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_cube_moved', {CUBE_SELECT: \"" + lightCubeIndex + "\"});");
        EvaluateJS("window.Scratch.vm.runtime.startHats('cozmo_event_on_cube_moved', {CUBE_SELECT: \"" + kAnyCubeId + "\"});");
      }
    }

    void UnhideWebView() {
      SharedMinigameView.HideMiddleBackground();

      // Add in black screen, so it looks like less of a pop.
      SharedMinigameView.ShowFullScreenGameStateSlide(_BlackScreenPrefab, "BlackScreen");
      SharedMinigameView.HideSpinnerWidget();

      _WebViewObjectComponent.SetVisibility(true);
    }

    public static bool IsRawStringValidCodelab(string data) {
      return data.Length > kCodelabPrefix.Length && data.Substring(0, kCodelabPrefix.Length) == kCodelabPrefix;
    }

    // Used for importing .codelab files.
    public static DataPersistence.CodeLabProject CreateProjectFromJsonString(string rawData) {
      if (!IsRawStringValidCodelab(rawData)) {
        DAS.Error("Codelab.OnAppLoadedFromData.BadHeader", "Attempting to load codelab file with improper prefix");
      }
      else {
        try {
          string pastPrefixString = rawData.Substring(kCodelabPrefix.Length);
          string unescapedString = WWW.UnEscapeURL(pastPrefixString);

          DataPersistence.CodeLabProject project = JsonConvert.DeserializeObject<DataPersistence.CodeLabProject>(unescapedString);
          DataPersistence.PlayerProfile defaultProfile = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile;

          if (defaultProfile == null) {
            DAS.Error("Codelab.OnAppLoadedFromData.NullDefaultProfile", "In creating new Code Lab project from external data, defaultProfile is null");
          }
          else if (defaultProfile.CodeLabProjects == null) {
            DAS.Error("Codelab.OnAppLoadedFromData.NullCodeLabProjects", "defaultProfile.CodeLabProjects is null");
          }
          else if (project.ProjectName.Length > kMaximumDescriptionLength) {
            DAS.Error("Codelab.OnAppLoadedFromData.BadFileName.Length", "new project's name is unreasonably long " + project.ProjectName.Length.ToString());
          }
          else if (project.ProjectJSON.Length > kMaximumCodelabDataLength) {
            DAS.Error("Codelab.OnAppLoadedFromData.BadFileData.Length", "new project's internal data is unreasonably long " + project.ProjectJSON.Length.ToString());
          }
          else if (project.VersionNum > CodeLabProject.kCurrentVersionNum) {
            DAS.Warn("Codelab.OnAppLoadedFromData.BadVersionNumber", "new project's version number " + project.VersionNum.ToString() + " is greater than the app's codelab version " + CodeLabProject.kCurrentVersionNum.ToString());
          }
          else {
            while (defaultProfile.CodeLabProjects.Find(p => p.ProjectUUID == project.ProjectUUID) != null) {
              project.ProjectUUID = Guid.NewGuid();
            }

            return project;
          }
        }
        catch (NullReferenceException) {
          DAS.Error("Codelab.OnAppLoadedFromData.NullReferenceException", "NullReferenceException in creating new Code Lab project from external data");
        }
      }
      return null;
    }

    // Used to import project.
    public static bool AddExternalProject(DataPersistence.CodeLabProject project) {

      DataPersistence.PlayerProfile defaultProfile = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile;
      if (defaultProfile == null) {
        DAS.Error("Codelab.AddExternalProject.NullDefaultProfile", "In adding external Code Lab project, defaultProfile is null");
        return false;
      }

      if (project == null) {
        DAS.Error("Codelab.AddExternalProject.NullProject", "In adding external Code Lab project, project is null");
        return false;
      }

      defaultProfile.CodeLabProjects.Add(project);

      ProjectStats stats = new ProjectStats();
      stats.Update(project);
      stats.PostPendingChanges(ProjectStats.EventCategory.loaded_from_file);

      return true;
    }

    public static string RemoveUnsupportedChars(string rawString) {
      string cleanedString = "";
      for (int i = 0; i < rawString.Length; i++) {
        char currentChar = rawString[i];
        if (CozmoInputFilter.IsValidInput(currentChar, allowPunctuation: true, allowDigits: true)) {
          cleanedString += currentChar;
        }
      }
      return cleanedString;
    }

    public static void PushImportError(string errorLocString) {
      _importErrors.Add(errorLocString);
    }
  }
}
